// Lab 06: materials, light and texture. Lab 05's WebGL 2 pipeline, with three new inputs:
//
//   normals  -> Lambert diffuse  max(N·L, 0)  and Blinn–Phong specular  max(N·H, 0)^shininess
//   UVs      -> a sampled texture with selectable filtering and mipmaps
//   color    -> light is added up in linear units; the screen expects sRGB-encoded values
//
// Lighting happens in world space: positions go through the model matrix, normals through
// the normal matrix (the inverse transpose of the model matrix's 3×3 part).
//
// Work through README.md in this folder. TODO(exercise X) comments mark code you write.
(function () {
  const canvas = document.querySelector('#l6-screen');
  const W = canvas.width;
  const H = canvas.height;
  const byId = (id) => document.getElementById(id);
  const DEFAULTS = { yaw: 20, squash: 1, az: 40, el: 35, shine: 48, repeat: 1 };
  const sliders = Object.fromEntries(Object.keys(DEFAULTS).map((key) => [key, byId(`l6-${key}`)]));
  const selects = Object.fromEntries(['mesh', 'shading', 'normals', 'color', 'filter', 'view'].map((k) => [k, byId(`l6-${k}`)]));
  const toggles = { spin: true, diffuse: true, specular: true, texture: false };
  const SPIN_SPEED = 0.4;
  const EYE = [0, 1.1, 4.2];
  let angle = 0;
  let frameRequest = 0;
  let lastTime = 0;
  let frameMs = 0;

  // ---------------------------------------------------------------------------
  // Shared math from ../common/math.js, including the 3×3 normal matrix
  // (the inverse transpose of the model matrix's upper-left 3×3).
  // ---------------------------------------------------------------------------
  const {
    scale, multiply, rotationY, scaling, radians, lookAt, perspective, upper3x3, normalMatrix,
  } = window.cg.math;

  // ---------------------------------------------------------------------------
  // Meshes with normals and UVs: 8 floats per vertex (position, normal, uv), 32 bytes.
  // ---------------------------------------------------------------------------
  function makeSphere(stacks, slices) {
    const vertices = [];
    const indices = [];
    for (let i = 0; i <= stacks; i++) {
      const theta = (Math.PI * i) / stacks; // 0 at the north pole
      for (let j = 0; j <= slices; j++) {
        const phi = (2 * Math.PI * j) / slices;
        const p = [Math.sin(theta) * Math.cos(phi), Math.cos(theta), -Math.sin(theta) * Math.sin(phi)];
        vertices.push(...p, ...p, j / slices, i / stacks); // on a unit sphere the normal is the position
      }
    }
    for (let i = 0; i < stacks; i++) {
      for (let j = 0; j < slices; j++) {
        const a = i * (slices + 1) + j;
        const b = a + slices + 1;
        indices.push(a, b, b + 1, a, b + 1, a + 1); // counter-clockwise seen from outside
      }
    }
    return { vertices: new Float32Array(vertices), indices: new Uint16Array(indices) };
  }

  function makeCube() {
    const corners = [
      [-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
      [-1, -1, 1], [1, -1, 1], [1, 1, 1], [-1, 1, 1],
    ];
    const faces = [[4, 5, 6, 7], [1, 0, 3, 2], [5, 1, 2, 6], [0, 4, 7, 3], [7, 6, 2, 3], [0, 1, 5, 4]];
    const normals = [[0, 0, 1], [0, 0, -1], [1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0]];
    const uvs = [[0, 1], [1, 1], [1, 0], [0, 0]]; // v = 0 is the first texture row (see exercise F)
    const vertices = [];
    const indices = [];
    faces.forEach((face, f) => {
      face.forEach((corner, k) => vertices.push(...scale(corners[corner], 0.8), ...normals[f], ...uvs[k]));
      indices.push(f * 4, f * 4 + 1, f * 4 + 2, f * 4, f * 4 + 2, f * 4 + 3);
    });
    return { vertices: new Float32Array(vertices), indices: new Uint16Array(indices) };
  }

  // ---------------------------------------------------------------------------
  // Shaders. Both stages can light: the vertex shader computes per-vertex factors for
  // Gouraud shading, the fragment shader recomputes them per sample for Phong shading.
  // ---------------------------------------------------------------------------
  const VERTEX_SHADER = `#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform mat3 uNormalMatrix;
uniform vec3 uLightDir;   // world space, unit length, pointing toward the light
uniform vec3 uEye;        // camera position in world space
uniform float uShininess;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec2 vGouraud;        // diffuse and specular factors computed per vertex

void main() {
  vec4 world = uModel * vec4(aPosition, 1.0);
  vWorldPos = world.xyz;
  vNormal = uNormalMatrix * aNormal;
  vUV = aUV;
  vec3 n = normalize(vNormal);
  vec3 h = normalize(uLightDir + normalize(uEye - world.xyz));
  float diffuse = max(dot(n, uLightDir), 0.0);
  vGouraud = vec2(diffuse, diffuse > 0.0 ? pow(max(dot(n, h), 0.0), uShininess) : 0.0);
  gl_Position = uViewProjection * world;
}
`;
  const FRAGMENT_SHADER = `#version 300 es
precision highp float;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec2 vGouraud;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uEye;
uniform float uShininess;
uniform float uRepeat;
uniform bool uPerFragment;
uniform bool uDiffuse;
uniform bool uSpecular;
uniform bool uTextured;
uniform bool uLinear;      // true: light in linear units and encode to sRGB at the end
uniform int uView;         // 0 shaded, 1 normals, 2 UVs, 3 N·L

out vec4 outColor;

vec3 srgbToLinear(vec3 c) {
  return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}
vec3 linearToSrgb(vec3 c) {
  return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

void main() {
  vec3 n = normalize(vNormal);  // interpolation shortens normals; renormalize per sample
  if (uView == 1) { outColor = vec4(n * 0.5 + 0.5, 1.0); return; }
  if (uView == 2) { outColor = vec4(fract(vUV * uRepeat), 0.0, 1.0); return; }

  // Albedo, the surface's base color. The constant is written in sRGB, the way a color
  // picker shows it. In linear mode the sRGB texture is decoded by the hardware.
  vec3 base = vec3(0.85, 0.42, 0.28);
  vec3 albedo = uTextured ? texture(uTexture, vUV * uRepeat).rgb : (uLinear ? srgbToLinear(base) : base);

  float diffuse = vGouraud.x;
  float specular = vGouraud.y;
  if (uPerFragment) {
    vec3 h = normalize(uLightDir + normalize(uEye - vWorldPos));
    diffuse = max(dot(n, uLightDir), 0.0);
    specular = diffuse > 0.0 ? pow(max(dot(n, h), 0.0), uShininess) : 0.0;
  }
  if (uView == 3) { outColor = vec4(vec3(diffuse), 1.0); return; }

  // TODO(exercise A): replace the directional light with a point light and 1 / d² falloff.
  // TODO(exercise B): scale the specular term with a Schlick Fresnel factor.
  vec3 color = albedo * (0.06 + (uDiffuse ? diffuse : 0.0)) + (uSpecular ? vec3(0.5 * specular) : vec3(0.0));
  color = clamp(color, 0.0, 1.0);
  outColor = vec4(uLinear ? linearToSrgb(color) : color, 1.0);
}
`;

  // ---------------------------------------------------------------------------
  // WebGL setup, once.
  // ---------------------------------------------------------------------------
  const gl = canvas.getContext('webgl2', { antialias: false, alpha: false });
  const meshes = {};
  const textures = {};
  let program = null;
  let uniforms = {};

  function compileShader(type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(shader));
    return shader;
  }

  function buildProgram() {
    program = gl.createProgram();
    gl.attachShader(program, compileShader(gl.VERTEX_SHADER, VERTEX_SHADER));
    gl.attachShader(program, compileShader(gl.FRAGMENT_SHADER, FRAGMENT_SHADER));
    gl.linkProgram(program);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(program));
    const names = ['uModel', 'uViewProjection', 'uNormalMatrix', 'uLightDir', 'uEye', 'uShininess', 'uTexture',
      'uRepeat', 'uPerFragment', 'uDiffuse', 'uSpecular', 'uTextured', 'uLinear', 'uView'];
    uniforms = Object.fromEntries(names.map((name) => [name, gl.getUniformLocation(program, name)]));
  }

  function uploadMesh({ vertices, indices }) {
    const vao = gl.createVertexArray();
    gl.bindVertexArray(vao);
    gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
    gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
    [[0, 3, 0], [1, 3, 12], [2, 2, 24]].forEach(([location, size, offset]) => {
      gl.enableVertexAttribArray(location);
      gl.vertexAttribPointer(location, size, gl.FLOAT, false, 32, offset);
    });
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, gl.createBuffer());
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, indices, gl.STATIC_DRAW);
    gl.bindVertexArray(null);
    return { vao, count: indices.length, vertices: vertices.length / 8 };
  }

  // A 256 × 256 checkerboard drawn on a 2D canvas, so the lab needs no image files.
  // The red cell marks the first row and column of the image.
  function makeTextureImage() {
    const image = document.createElement('canvas');
    image.width = image.height = 256;
    const g = image.getContext('2d');
    for (let y = 0; y < 8; y++) {
      for (let x = 0; x < 8; x++) {
        g.fillStyle = (x + y) % 2 ? '#2a2f38' : '#ece4d6';
        g.fillRect(x * 32, y * 32, 32, 32);
      }
    }
    g.fillStyle = '#e8483c';
    g.fillRect(0, 0, 32, 32);
    return image;
  }

  // Upload the same pixels twice: once as sRGB (the GPU decodes to linear before
  // filtering) and once as plain RGBA8 (the shader sees the encoded numbers as-is).
  // TODO(exercise E): build the mip chain yourself instead of calling generateMipmap.
  function uploadTexture(image, internalFormat) {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texImage2D(gl.TEXTURE_2D, 0, internalFormat, gl.RGBA, gl.UNSIGNED_BYTE, image);
    gl.generateMipmap(gl.TEXTURE_2D);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
    return texture;
  }

  function setFilter(texture, mode) {
    gl.bindTexture(gl.TEXTURE_2D, texture);
    const min = { nearest: gl.NEAREST, linear: gl.LINEAR, trilinear: gl.LINEAR_MIPMAP_LINEAR }[mode];
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, min);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, mode === 'nearest' ? gl.NEAREST : gl.LINEAR);
  }

  // ---------------------------------------------------------------------------
  // One frame.
  // ---------------------------------------------------------------------------
  const fmt = (v) => (Math.abs(v) < 5e-4 ? 0 : v).toFixed(2).padStart(6);

  function render() {
    const started = performance.now();
    Object.values(sliders).forEach((el) => { el.nextElementSibling.value = el.value; });
    const az = radians(Number(sliders.az.value));
    const el = radians(Number(sliders.el.value));
    const lightDir = [Math.cos(el) * Math.sin(az), Math.sin(el), Math.cos(el) * Math.cos(az)];
    const model = multiply(rotationY(radians(Number(sliders.yaw.value)) + angle), scaling(1, Number(sliders.squash.value), 1));
    const normals = selects.normals.value === 'inverse' ? normalMatrix(model) : upper3x3(model);
    byId('l6-light').textContent = `(${lightDir.map(fmt).join(',')})`;
    byId('l6-normal-matrix').textContent = [0, 1, 2].map((r) => [0, 1, 2].map((c) => fmt(normals[c * 3 + r])).join(' ')).join('\n');
    if (!gl) return;

    const mesh = meshes[selects.mesh.value];
    const linear = selects.color.value === 'linear';
    const texture = linear ? textures.srgb : textures.raw;
    setFilter(texture, selects.filter.value);

    gl.viewport(0, 0, W, H);
    gl.clearColor(10 / 255, 13 / 255, 18 / 255, 1);
    gl.enable(gl.DEPTH_TEST);
    gl.enable(gl.CULL_FACE);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.useProgram(program);
    const viewProjection = multiply(perspective(radians(45), W / H, 0.1, 50), lookAt(EYE, [0, 0, 0], [0, 1, 0]));
    gl.uniformMatrix4fv(uniforms.uModel, false, new Float32Array(model));
    gl.uniformMatrix4fv(uniforms.uViewProjection, false, new Float32Array(viewProjection));
    gl.uniformMatrix3fv(uniforms.uNormalMatrix, false, new Float32Array(normals));
    gl.uniform3fv(uniforms.uLightDir, lightDir);
    gl.uniform3fv(uniforms.uEye, EYE);
    gl.uniform1f(uniforms.uShininess, Number(sliders.shine.value));
    gl.uniform1f(uniforms.uRepeat, Number(sliders.repeat.value));
    gl.uniform1i(uniforms.uPerFragment, selects.shading.value === 'fragment' ? 1 : 0);
    gl.uniform1i(uniforms.uDiffuse, toggles.diffuse ? 1 : 0);
    gl.uniform1i(uniforms.uSpecular, toggles.specular ? 1 : 0);
    gl.uniform1i(uniforms.uTextured, toggles.texture ? 1 : 0);
    gl.uniform1i(uniforms.uLinear, linear ? 1 : 0);
    gl.uniform1i(uniforms.uView, Number(selects.view.value));
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.uniform1i(uniforms.uTexture, 0);
    gl.bindVertexArray(mesh.vao);
    gl.drawElements(gl.TRIANGLES, mesh.count, gl.UNSIGNED_SHORT, 0);
    gl.bindVertexArray(null);

    byId('l6-mesh-info').textContent = `V ${mesh.vertices.toLocaleString()} · T ${(mesh.count / 3).toLocaleString()}`;
    frameMs = frameMs * 0.9 + (performance.now() - started) * 0.1;
    byId('l6-frame').textContent = `${frameMs.toFixed(2)} ms`;
  }

  function frame(time) {
    const dt = lastTime ? Math.min(0.1, (time - lastTime) / 1000) : 0;
    lastTime = time;
    if (toggles.spin) angle += dt * SPIN_SPEED;
    render();
    frameRequest = requestAnimationFrame(frame);
  }

  // ---------------------------------------------------------------------------
  // Start-up and controls.
  // ---------------------------------------------------------------------------
  if (gl) {
    buildProgram();
    meshes.sphere = uploadMesh(makeSphere(24, 48));
    meshes.cube = uploadMesh(makeCube());
    const image = makeTextureImage();
    textures.srgb = uploadTexture(image, gl.SRGB8_ALPHA8);
    textures.raw = uploadTexture(image, gl.RGBA8);
    byId('l6-texture-info').textContent = `256 × 256 · ${Math.log2(256) + 1} mip levels`;
  } else {
    byId('l6-no-webgl').hidden = false;
  }

  Object.values(sliders).forEach((el) => el.addEventListener('input', render));
  Object.values(selects).forEach((el) => el.addEventListener('change', render));
  document.querySelectorAll('[data-l6-toggle]').forEach((button) => {
    const key = button.dataset.l6Toggle;
    button.setAttribute('aria-pressed', String(toggles[key]));
    button.addEventListener('click', () => {
      toggles[key] = !toggles[key];
      button.setAttribute('aria-pressed', String(toggles[key]));
      render();
    });
  });
  byId('l6-reset').addEventListener('click', () => {
    Object.entries(DEFAULTS).forEach(([key, value]) => { sliders[key].value = value; });
    const choices = { mesh: 'sphere', shading: 'fragment', normals: 'inverse', color: 'linear', filter: 'trilinear', view: '0' };
    Object.entries(choices).forEach(([key, value]) => { selects[key].value = value; });
    angle = 0;
    render();
  });

  window.materialLab = {
    init: render,
    start() {
      if (frameRequest) return;
      lastTime = 0;
      frameRequest = requestAnimationFrame(frame);
    },
    stop() {
      cancelAnimationFrame(frameRequest);
      frameRequest = 0;
    },
  };
})();
