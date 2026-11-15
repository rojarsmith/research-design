// Lab 08: the final project's reference demo. Labs 05–07 combined into one small renderer:
// a navigable scene, a textured lit material, a shadow map, MSAA, a render scale, and a
// debug view for every stage, with the numbers you need for the design note on the page.
//
//   CPU, once       meshes and textures -> GPU buffers; shader sources -> programs
//   CPU, per frame  input -> camera; animation -> model matrices; light -> light matrices
//   pass 1 shadow   every object, from the light, into a depth texture
//   pass 2 scene    every object, from the camera: textured, lit, shadowed (1 or 4 samples)
//   pass 3 resolve  4 samples -> 1 per pixel (only with MSAA)
//   pass 4 present  the scaled image -> the canvas, filtered in linear light, encoded once
//
// The file is laid out in the same order. Work through README.md in this folder;
// TODO(exercise X) comments mark where the extensions go.
(function () {
  const canvas = document.querySelector('#l8-screen');
  const W = canvas.width;
  const H = canvas.height;
  const byId = (id) => document.getElementById(id);

  // ---------------------------------------------------------------------------
  // 1. Settings: page controls, read once per frame.
  // ---------------------------------------------------------------------------
  const DEFAULTS = { fov: 60, az: 40, el: 40 };
  const CHOICES = { view: '0', scale: '1', size: '2048' };
  const sliders = Object.fromEntries(Object.keys(DEFAULTS).map((key) => [key, byId(`l8-${key}`)]));
  const selects = Object.fromEntries(Object.keys(CHOICES).map((key) => [key, byId(`l8-${key}`)]));
  const toggles = { shadows: true, texture: true, specular: true, msaa: true, animate: true };

  const {
    add, sub, scale, cross, normalize, multiply, transform, fromRows, identity, translation, rotationX,
    rotationY, scaling, radians, lookAt, perspective, orthographic, normalMatrix,
  } = window.cg.math;
  const { srgbToLinear, byteToLinear } = window.cg.color;
  const meshes = window.cg.mesh;
  const webgl = window.cg.webgl;

  // ---------------------------------------------------------------------------
  // 2. The scene, as data: meshes, materials, and objects that pair them with a model matrix.
  // Colors are sRGB, as a color picker shows them; they are decoded once, at upload.
  // ---------------------------------------------------------------------------
  const MESHES = {
    ground: () => meshes.plane(8),
    box: () => meshes.box(0.5, 0.5, 0.5),
    sphere: () => meshes.sphere(0.7, 32, 64),
    pillar: () => meshes.box(0.18, 0.8, 0.18),
  };
  const MATERIALS = {
    tiles: { texture: 'tiles', color: [1, 1, 1], uvScale: 4, shininess: 24, specular: 0.15 },
    crate: { texture: 'crate', color: [1, 1, 1], uvScale: 1, shininess: 12, specular: 0.08 },
    glaze: { texture: null, color: [0.3, 0.55, 0.85], uvScale: 1, shininess: 96, specular: 0.6 },
    stone: { texture: null, color: [0.78, 0.74, 0.66], uvScale: 1, shininess: 8, specular: 0.05 },
    brass: { texture: null, color: [0.86, 0.66, 0.3], uvScale: 1, shininess: 64, specular: 0.5 },
  };
  const SPINNER_HEIGHT = 2.0;
  // `model(time)` returns the object's model matrix; most objects ignore the time.
  const SCENE = [
    { name: 'ground', mesh: 'ground', material: 'tiles', model: () => identity() },
    { name: 'crate', mesh: 'box', material: 'crate', model: () => multiply(translation(-1.6, 0.5, 0.6), rotationY(radians(18))) },
    { name: 'small crate', mesh: 'box', material: 'crate', model: () => multiply(translation(-1.5, 1.3, 0.5), multiply(rotationY(radians(-12)), scaling(0.6, 0.6, 0.6))) },
    { name: 'sphere', mesh: 'sphere', material: 'glaze', model: () => translation(1.3, 0.7, 0.2) },
    { name: 'pillar', mesh: 'pillar', material: 'stone', model: () => translation(0.1, 0.8, -1.8) },
    {
      name: 'spinner', mesh: 'box', material: 'brass',
      model: (t) => multiply(translation(0.1, SPINNER_HEIGHT, -1.8), multiply(rotationY(t), multiply(rotationX(t * 0.7), scaling(0.4, 0.4, 0.4)))),
    },
  ];
  const TRACE_OBJECT = SCENE.length - 1; // the spinner's corner is traced through every space
  const TRACE_POINT = [0.5, 0.5, 0.5, 1];
  const BACKGROUND = [10, 13, 18].map(byteToLinear);
  const NEAR = 0.1;
  const FAR = 60;

  // Two textures drawn on 2D canvases, so the lab needs no image files. Both carry a red
  // mark in the first row and column of the image, as in Lab 06.
  function makeTileTexture() {
    const image = document.createElement('canvas');
    image.width = image.height = 256;
    const g = image.getContext('2d');
    g.fillStyle = '#5b544c';
    g.fillRect(0, 0, 256, 256);
    for (let y = 0; y < 4; y++) {
      for (let x = 0; x < 4; x++) {
        const shade = 168 + ((x * 37 + y * 61) % 5) * 9; // fixed per-tile variation
        g.fillStyle = `rgb(${shade}, ${shade - 8}, ${shade - 22})`;
        g.fillRect(x * 64 + 3, y * 64 + 3, 58, 58);
      }
    }
    g.fillStyle = '#c8473c';
    g.fillRect(3, 3, 12, 12);
    return image;
  }

  function makeCrateTexture() {
    const image = document.createElement('canvas');
    image.width = image.height = 256;
    const g = image.getContext('2d');
    g.fillStyle = '#a8763f';
    g.fillRect(0, 0, 256, 256);
    g.fillStyle = '#8c5f2f';
    for (let y = 0; y < 256; y += 32) g.fillRect(0, y, 256, 3); // planks
    g.strokeStyle = '#6b4520';
    g.lineWidth = 22;
    g.strokeRect(11, 11, 234, 234); // frame
    g.beginPath();
    g.moveTo(22, 22);
    g.lineTo(234, 234); // brace
    g.stroke();
    g.fillStyle = '#c8473c';
    g.fillRect(0, 0, 12, 12);
    return image;
  }

  // ---------------------------------------------------------------------------
  // 3. The camera: a fly camera. Yaw 0 looks down −z; pitch looks up. W A S D move along the
  // view direction and its right vector, Q / E move down and up, Shift is faster.
  // ---------------------------------------------------------------------------
  const START = { eye: [0.6, 2.2, 6.4], yaw: -4, pitch: -14 };
  const camera = { eye: [...START.eye], yaw: START.yaw, pitch: START.pitch };
  const keys = new Set();
  const SPEED = 2.5; // world units per second

  function cameraAxes() {
    const yaw = radians(camera.yaw);
    const pitch = radians(camera.pitch);
    const forward = [-Math.sin(yaw) * Math.cos(pitch), Math.sin(pitch), -Math.cos(yaw) * Math.cos(pitch)];
    const right = normalize(cross(forward, [0, 1, 0]));
    return { forward, right };
  }

  function updateCamera(dt) {
    const { forward, right } = cameraAxes();
    let move = [0, 0, 0];
    if (keys.has('KeyW')) move = add(move, forward);
    if (keys.has('KeyS')) move = sub(move, forward);
    if (keys.has('KeyD')) move = add(move, right);
    if (keys.has('KeyA')) move = sub(move, right);
    if (keys.has('KeyE')) move = add(move, [0, 1, 0]);
    if (keys.has('KeyQ')) move = sub(move, [0, 1, 0]);
    const speed = SPEED * (keys.has('ShiftLeft') || keys.has('ShiftRight') ? 3 : 1);
    camera.eye = add(camera.eye, scale(move, speed * dt));
  }

  // ---------------------------------------------------------------------------
  // 4. GPU resources. Shaders first: one depth-only pair, one scene pair, and two
  // full-screen passes from ../common/webgl.js.
  // ---------------------------------------------------------------------------
  const DEPTH_VERTEX_SHADER = `#version 300 es
layout(location = 0) in vec3 aPosition;
uniform mat4 uLightViewProjection;
uniform mat4 uModel;
void main() {
  gl_Position = uLightViewProjection * uModel * vec4(aPosition, 1.0);
}
`;
  const DEPTH_FRAGMENT_SHADER = `#version 300 es
precision mediump float;
void main() {}
`;

  const SCENE_VERTEX_SHADER = `#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform mat4 uShadowMatrix;   // world -> the shadow map's (u, v, d), as in Lab 07
uniform mat3 uNormalMatrix;   // inverse transpose of the model matrix's 3 × 3, as in Lab 06

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vShadowCoord;

void main() {
  vec4 world = uModel * vec4(aPosition, 1.0);
  vWorldPos = world.xyz;
  vNormal = uNormalMatrix * aNormal;
  vUV = aUV;
  vShadowCoord = uShadowMatrix * world;
  gl_Position = uViewProjection * world;
}
`;
  const SCENE_FRAGMENT_SHADER = `#version 300 es
precision highp float;
precision highp sampler2DShadow;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vShadowCoord;

uniform sampler2D uAlbedoMap;        // SRGB8_ALPHA8: decoded to linear light before filtering
uniform sampler2DShadow uShadowMap;  // LINEAR compare: each tap blends a 2 × 2 comparison
uniform vec3 uColor;                 // material color, linear light
uniform bool uTextured;
uniform float uUVScale;
uniform float uShininess;
uniform float uSpecular;             // specular strength; 0 turns it off
uniform vec3 uLightDir;              // world space, toward the light
uniform vec3 uEye;
uniform bool uShadows;
uniform float uTexelDepth;           // one shadow-map texel's width / the light's depth range
uniform float uMapSize;
uniform float uNear;
uniform float uFar;
uniform int uView;                   // 0 final, 1 albedo, 2 normals, 3 UVs, 4 depth, 5 shadow

out vec4 outColor;
${webgl.GLSL_COLOR}
// Lab 07's lookup: 3 × 3 bilinear taps reach texels up to 2 texels away, so the slope term
// forgives 3 texels of depth slope.
float shadowFactor(vec3 n) {
  if (!uShadows) return 1.0;
  vec3 p = vShadowCoord.xyz / vShadowCoord.w;
  if (any(lessThan(p, vec3(0.0))) || any(greaterThan(p, vec3(1.0)))) return 1.0;
  float c = clamp(dot(n, uLightDir), 0.05, 1.0);
  float reference = p.z - (0.0005 + 3.0 * uTexelDepth * sqrt(1.0 - c * c) / c);
  float lit = 0.0;
  for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) lit += texture(uShadowMap, vec3(p.xy + vec2(x, y) / uMapSize, reference));
  }
  return lit / 9.0;
}

void main() {
  vec3 n = normalize(vNormal);
  vec2 uv = vUV * uUVScale;
  // TODO(exercise B): perturb n with a normal map here.
  vec3 albedo = uColor * (uTextured ? texture(uAlbedoMap, uv).rgb : vec3(1.0));
  float shadow = shadowFactor(n);

  // Debug views. Albedo is a color, so it goes through the normal encode; the others are
  // measurements, pre-decoded so the final encode shows the number itself.
  if (uView == 1) { outColor = vec4(albedo, 1.0); return; }
  if (uView == 2) { outColor = vec4(srgbToLinear(n * 0.5 + 0.5), 1.0); return; }
  if (uView == 3) { outColor = vec4(srgbToLinear(vec3(fract(uv), 0.0)), 1.0); return; }
  if (uView == 4) {
    float z = uNear * uFar / (uFar - gl_FragCoord.z * (uFar - uNear));  // window depth -> view distance
    outColor = vec4(srgbToLinear(vec3(clamp(1.0 - z / 20.0, 0.0, 1.0))), 1.0);
    return;
  }
  if (uView == 5) { outColor = vec4(srgbToLinear(vec3(shadow)), 1.0); return; }

  // TODO(exercise C): replace Blinn–Phong with Fresnel and energy normalization, or PBR.
  vec3 h = normalize(uLightDir + normalize(uEye - vWorldPos));
  float diffuse = max(dot(n, uLightDir), 0.0);
  float specular = diffuse > 0.0 ? uSpecular * pow(max(dot(n, h), 0.0), uShininess) : 0.0;
  vec3 ambient = albedo * mix(vec3(0.05, 0.04, 0.035), vec3(0.11, 0.13, 0.17), n.y * 0.5 + 0.5);  // ground and sky
  vec3 color = ambient + shadow * (albedo * diffuse + vec3(specular));
  // TODO(exercise G): render HDR values into a float target and tone-map them in the present pass.
  outColor = vec4(min(color, vec3(1.0)), 1.0);  // linear; the SRGB8_ALPHA8 target encodes on write
}
`;

  // Pass 4: bilinear in linear light (the target is sRGB, so sampling decodes before filtering).
  // At 2× this sample point sits on the corner of 4 texels, so bilinear is an exact 2 × 2 average.
  const PRESENT_FRAGMENT_SHADER = `#version 300 es
precision highp float;
in vec2 vUV;
uniform sampler2D uColor;
out vec4 outColor;
${webgl.GLSL_COLOR}
void main() {
  outColor = vec4(linearToSrgb(texture(uColor, vUV).rgb), 1.0);
}
`;
  const DEPTH_VIEW_FRAGMENT_SHADER = `#version 300 es
precision highp float;
in vec2 vUV;
uniform sampler2D uDepth;
out vec4 outColor;
void main() {
  outColor = vec4(vec3(texture(uDepth, vUV).r), 1.0);  // a measurement, written directly
}
`;

  const gl = canvas.getContext('webgl2', { antialias: false, alpha: false, depth: false });
  let gpu = null; // every GPU object lives here, created once in setup()
  let targets = null; // the scene's render targets for the current size and sample count

  function setup() {
    gpu = {
      programs: {
        depth: webgl.program(gl, DEPTH_VERTEX_SHADER, DEPTH_FRAGMENT_SHADER),
        scene: webgl.program(gl, SCENE_VERTEX_SHADER, SCENE_FRAGMENT_SHADER),
        present: webgl.program(gl, webgl.FULLSCREEN_VERTEX_SHADER, PRESENT_FRAGMENT_SHADER),
        depthView: webgl.program(gl, webgl.FULLSCREEN_VERTEX_SHADER, DEPTH_VIEW_FRAGMENT_SHADER),
      },
      meshes: Object.fromEntries(Object.entries(MESHES).map(([name, build]) => [name, webgl.uploadMesh(gl, build())])),
      textures: { tiles: webgl.imageTexture(gl, makeTileTexture()), crate: webgl.imageTexture(gl, makeCrateTexture()) },
      shadowMap: webgl.depthTarget(gl, Number(CHOICES.size)),
      rawSampler: webgl.rawDepthSampler(gl),
      timer: webgl.gpuTimer(gl),
    };
    // Bilinear comparison: each tap blends the 0 / 1 results of the 4 nearest texels.
    gl.bindTexture(gl.TEXTURE_2D, gpu.shadowMap.texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
  }

  // The scene pass draws at the render scale; with MSAA it needs a multisample target plus a
  // 1-sample target to resolve into. Targets are rebuilt only when the size or MSAA changes.
  function sceneTargets(renderScale, msaa) {
    const width = Math.round(W * renderScale);
    const height = Math.round(H * renderScale);
    const key = `${width}x${height}x${msaa ? 4 : 1}`;
    if (targets && targets.key === key) return targets;
    if (targets) {
      targets.single.dispose();
      if (targets.msaa) targets.msaa.dispose();
    }
    targets = {
      key, width, height,
      single: webgl.colorTarget(gl, width, height, 1),
      msaa: msaa ? webgl.colorTarget(gl, width, height, 4) : null,
    };
    return targets;
  }

  // ---------------------------------------------------------------------------
  // 5. Passes.
  // ---------------------------------------------------------------------------
  // A directional light with an orthographic box around the whole scene (Lab 07).
  const LIGHT_DISTANCE = 15;
  const SCENE_RADIUS = 12;
  const LIGHT_BOX = 9;
  const NDC_TO_TEXTURE = fromRows([0.5, 0, 0, 0.5, 0, 0.5, 0, 0.5, 0, 0, 0.5, 0.5, 0, 0, 0, 1]);

  // TODO(exercise F): fit the box to the camera's view and snap it to whole texels.
  function lightFrame() {
    const az = radians(Number(sliders.az.value));
    const el = radians(Number(sliders.el.value));
    const dir = [Math.cos(el) * Math.sin(az), Math.sin(el), Math.cos(el) * Math.cos(az)];
    const up = Math.abs(dir[1]) > 0.99 ? [0, 0, -1] : [0, 1, 0];
    const range = 2 * SCENE_RADIUS;
    // TODO(exercise A): a spot or point light uses perspective() and a position instead.
    const viewProjection = multiply(
      orthographic(LIGHT_BOX, 1, LIGHT_DISTANCE - SCENE_RADIUS, LIGHT_DISTANCE + SCENE_RADIUS),
      lookAt(scale(dir, LIGHT_DISTANCE), [0, 0, 0], up),
    );
    return { dir, range, viewProjection, shadowMatrix: multiply(NDC_TO_TEXTURE, viewProjection) };
  }

  function drawObjects(program, frame, perObject) {
    // TODO(exercise D): skip objects whose bounding sphere is outside the view frustum.
    frame.objects.forEach((object) => {
      const mesh = gpu.meshes[object.mesh];
      if (perObject) perObject(object);
      gl.uniformMatrix4fv(program.uniforms.uModel, false, new Float32Array(object.model));
      gl.bindVertexArray(mesh.vao);
      gl.drawElements(gl.TRIANGLES, mesh.count, gl.UNSIGNED_SHORT, 0);
    });
    gl.bindVertexArray(null);
  }

  // Pass 1: depth from the light. The camera's culling rule applies here too.
  function shadowPass(frame) {
    const map = gpu.shadowMap;
    map.resize(frame.settings.size);
    gl.bindFramebuffer(gl.FRAMEBUFFER, map.framebuffer);
    gl.viewport(0, 0, map.size, map.size);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LESS);
    gl.enable(gl.CULL_FACE);
    gl.cullFace(gl.BACK);
    gl.clear(gl.DEPTH_BUFFER_BIT);
    const program = gpu.programs.depth;
    gl.useProgram(program.program);
    gl.uniformMatrix4fv(program.uniforms.uLightViewProjection, false, new Float32Array(frame.light.viewProjection));
    drawObjects(program, frame);
  }

  // Pass 2: the camera's view into the scene target, one draw per object.
  function scenePass(frame, target) {
    const s = frame.settings;
    gl.bindFramebuffer(gl.FRAMEBUFFER, target.framebuffer);
    gl.viewport(0, 0, target.width, target.height);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LESS);
    gl.enable(gl.CULL_FACE);
    gl.cullFace(gl.BACK);
    gl.clearColor(...BACKGROUND, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    const program = gpu.programs.scene;
    const u = program.uniforms;
    gl.useProgram(program.program);
    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_2D, gpu.shadowMap.texture);
    gl.uniform1i(u.uShadowMap, 1);
    gl.uniform1i(u.uAlbedoMap, 0);
    gl.uniformMatrix4fv(u.uViewProjection, false, new Float32Array(frame.viewProjection));
    gl.uniformMatrix4fv(u.uShadowMatrix, false, new Float32Array(frame.light.shadowMatrix));
    gl.uniform3fv(u.uLightDir, frame.light.dir);
    gl.uniform3fv(u.uEye, camera.eye);
    gl.uniform1i(u.uShadows, s.shadows ? 1 : 0);
    gl.uniform1f(u.uTexelDepth, (2 * LIGHT_BOX) / s.size / frame.light.range);
    gl.uniform1f(u.uMapSize, s.size);
    gl.uniform1f(u.uNear, NEAR);
    gl.uniform1f(u.uFar, FAR);
    gl.uniform1i(u.uView, s.view);
    drawObjects(program, frame, (object) => {
      const material = MATERIALS[object.material];
      const textured = Boolean(material.texture) && s.texture;
      // Untextured materials keep a texture bound anyway: an empty unit makes browsers warn.
      gl.activeTexture(gl.TEXTURE0);
      gl.bindTexture(gl.TEXTURE_2D, gpu.textures[material.texture || 'tiles']);
      gl.uniformMatrix3fv(u.uNormalMatrix, false, new Float32Array(normalMatrix(object.model)));
      gl.uniform3fv(u.uColor, material.color.map(srgbToLinear));
      gl.uniform1i(u.uTextured, textured ? 1 : 0);
      gl.uniform1f(u.uUVScale, material.uvScale);
      gl.uniform1f(u.uShininess, material.shininess);
      gl.uniform1f(u.uSpecular, s.specular ? material.specular : 0);
    });
    gl.disable(gl.CULL_FACE);
    gl.disable(gl.DEPTH_TEST);
  }

  // Pass 4: into the canvas, bilinear in linear light, encoded once.
  function presentPass(source) {
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.viewport(0, 0, W, H);
    const program = gpu.programs.present;
    gl.useProgram(program.program);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, source.texture);
    gl.uniform1i(program.uniforms.uColor, 0);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
  }

  // Debug view "Light's view": the shadow map itself, in a square in the middle.
  function lightViewPass() {
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.viewport(0, 0, W, H);
    gl.clearColor(10 / 255, 13 / 255, 18 / 255, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.viewport((W - H) / 2, 0, H, H);
    const program = gpu.programs.depthView;
    gl.useProgram(program.program);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, gpu.shadowMap.texture);
    gl.bindSampler(0, gpu.rawSampler);
    gl.uniform1i(program.uniforms.uDepth, 0);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bindSampler(0, null);
  }

  // ---------------------------------------------------------------------------
  // 6. One frame: gather the frame's data, run the passes, report what happened.
  // ---------------------------------------------------------------------------
  let time = 0;
  let frameRequest = 0;
  let lastTime = 0;
  let frameMs = 0;
  const fmt = (v) => (Math.abs(v) < 5e-4 ? 0 : v).toFixed(2).padStart(6);
  const vec = (v, n = v.length) => `(${v.slice(0, n).map(fmt).join(',')})`;
  const mib = (bytes) => `${(bytes / 1048576).toFixed(1)} MiB`;

  function readSettings() {
    return {
      view: Number(selects.view.value),
      scale: Number(selects.scale.value),
      size: Number(selects.size.value),
      fov: Number(sliders.fov.value),
      ...toggles,
    };
  }

  function buildFrame() {
    const settings = readSettings();
    const { forward } = cameraAxes();
    const view = lookAt(camera.eye, add(camera.eye, forward), [0, 1, 0]);
    const projection = perspective(radians(settings.fov), W / H, NEAR, FAR);
    return {
      settings,
      view,
      projection,
      viewProjection: multiply(projection, view),
      light: lightFrame(),
      objects: SCENE.map((object) => ({ ...object, model: object.model(time) })),
    };
  }

  // The numbers for the design note: one vertex through every space, and into the shadow map.
  function showTrace(frame) {
    const object = frame.objects[TRACE_OBJECT];
    const world = transform(object.model, TRACE_POINT);
    const viewPos = transform(frame.view, world);
    const clip = transform(frame.projection, viewPos);
    const ndc = clip.map((c) => c / clip[3]);
    const pixel = [((ndc[0] + 1) / 2) * W, ((1 - ndc[1]) / 2) * H, (ndc[2] + 1) / 2];
    const shadow = transform(frame.light.shadowMatrix, world);
    const inside = clip[3] > 0 && ndc.slice(0, 3).every((c) => Math.abs(c) <= 1);
    byId('l8-trace').textContent = [
      `object   ${vec(TRACE_POINT, 3)}   corner of the ${object.name}`,
      `world    ${vec(world, 3)}   M · object`,
      `view     ${vec(viewPos, 3)}   V · world`,
      `clip     ${vec(clip)}   P · view`,
      `NDC      ${vec(ndc, 3)}   clip ÷ w${inside ? '' : '   (outside the view volume)'}`,
      `pixel    (${pixel[0].toFixed(1)}, ${pixel[1].toFixed(1)}) depth ${pixel[2].toFixed(4)}   row 0 at the top`,
      `shadow   ${vec(shadow, 3)}   B · P_light · V_light · world = (u, v, d)`,
    ].join('\n');
  }

  function showReadouts(frame, target) {
    const s = frame.settings;
    const { forward } = cameraAxes();
    byId('l8-camera').textContent =
      `eye     ${vec(camera.eye)}\nforward ${vec(forward)}\nyaw ${camera.yaw.toFixed(0)}° · pitch ${camera.pitch.toFixed(0)}°`;
    const draws = frame.objects.length;
    const triangles = frame.objects.reduce((sum, o) => sum + gpu.meshes[o.mesh].triangles, 0);
    byId('l8-draws').textContent = `${draws} draws · ${triangles.toLocaleString()} triangles per pass`;
    const size = `${target.width} × ${target.height}`;
    const lines = s.view === 6
      ? [`1 shadow   ${s.size}² depth · ${draws} draws`, `- light's view: the shadow map, drawn to the canvas`]
      : [
        `1 shadow   ${s.size}² depth · ${draws} draws`,
        `2 scene    ${size} · ${s.msaa ? 'MSAA 4×' : '1 sample'} · ${draws} draws`,
        s.msaa ? `3 resolve  blitFramebuffer · 4 samples → 1` : '3 resolve  (skipped: no MSAA)',
        `4 present  ${size} → ${W} × ${H} · linear filter · sRGB encode`,
      ];
    byId('l8-passes').textContent = lines.join('\n');
    const targetBytes = target.single.bytes + (target.msaa ? target.msaa.bytes : 0);
    const textureBytes = Object.keys(gpu.textures).length * 256 * 256 * 4 * (4 / 3);
    byId('l8-memory').textContent =
      `shadow map ${mib(s.size * s.size * 4)}\nscene targets ${mib(targetBytes)}\ntextures ${mib(textureBytes)}`;
    if (gpu.timer) {
      const t = gpu.timer.poll();
      const ms = (v) => (v === undefined ? '…' : `${v.toFixed(3)} ms`);
      byId('l8-gpu').textContent = `shadow          ${ms(t.shadow)}\nscene + resolve ${ms(t.scene)}\npresent         ${ms(t.present)}`;
    }
  }

  function render() {
    const started = performance.now();
    Object.values(sliders).forEach((el) => { el.nextElementSibling.value = el.value; });
    const frame = buildFrame();
    showTrace(frame);
    if (!gl) return;
    const s = frame.settings;
    const timer = gpu.timer;
    const target = sceneTargets(s.scale, s.msaa);

    if (timer) timer.begin('shadow');
    shadowPass(frame);
    if (timer) timer.end();
    if (s.view === 6) {
      lightViewPass();
    } else {
      if (timer) timer.begin('scene');
      scenePass(frame, target.msaa || target.single);
      if (target.msaa) webgl.resolve(gl, target.msaa, target.single);
      if (timer) timer.end();
      if (timer) timer.begin('present');
      presentPass(target.single);
      if (timer) timer.end();
    }
    showReadouts(frame, target);
    frameMs = frameMs * 0.9 + (performance.now() - started) * 0.1;
    byId('l8-frame').textContent = `${frameMs.toFixed(2)} ms`;
  }

  function tick(now) {
    const dt = lastTime ? Math.min(0.1, (now - lastTime) / 1000) : 0;
    lastTime = now;
    if (toggles.animate) time += dt;
    updateCamera(dt);
    render();
    frameRequest = requestAnimationFrame(tick);
  }

  // ---------------------------------------------------------------------------
  // 7. Start-up and controls.
  // ---------------------------------------------------------------------------
  if (gl) {
    setup();
    byId('l8-gpu-info').textContent = webgl.describeGpu(gl);
    byId('l8-gpu').textContent = gpu.timer ? '…' : 'EXT_disjoint_timer_query_webgl2: —';
  } else {
    byId('l8-no-webgl').hidden = false;
    byId('l8-gpu-info').textContent = 'WebGL 2: —';
  }

  Object.values(sliders).forEach((el) => el.addEventListener('input', render));
  Object.values(selects).forEach((el) => el.addEventListener('change', render));
  document.querySelectorAll('[data-l8-toggle]').forEach((button) => {
    const key = button.dataset.l8Toggle;
    button.setAttribute('aria-pressed', String(toggles[key]));
    button.addEventListener('click', () => {
      toggles[key] = !toggles[key];
      button.setAttribute('aria-pressed', String(toggles[key]));
      render();
    });
  });
  byId('l8-reset').addEventListener('click', () => {
    Object.assign(camera, { eye: [...START.eye], yaw: START.yaw, pitch: START.pitch });
    Object.entries(DEFAULTS).forEach(([key, value]) => { sliders[key].value = value; });
    Object.entries(CHOICES).forEach(([key, value]) => { selects[key].value = value; });
    render();
  });

  // Keys move the camera only while the canvas has focus, so the page still scrolls normally.
  const MOVE_KEYS = ['KeyW', 'KeyA', 'KeyS', 'KeyD', 'KeyQ', 'KeyE', 'ShiftLeft', 'ShiftRight'];
  canvas.addEventListener('keydown', (event) => {
    if (!MOVE_KEYS.includes(event.code)) return;
    keys.add(event.code);
    event.preventDefault();
  });
  canvas.addEventListener('keyup', (event) => keys.delete(event.code));
  canvas.addEventListener('blur', () => keys.clear());

  let drag = null;
  canvas.addEventListener('pointerdown', (event) => {
    canvas.focus({ preventScroll: true });
    drag = { x: event.clientX, y: event.clientY };
    canvas.setPointerCapture(event.pointerId);
  });
  canvas.addEventListener('pointermove', (event) => {
    if (!drag) return;
    camera.yaw -= (event.clientX - drag.x) * 0.3;
    camera.yaw = ((camera.yaw + 540) % 360) - 180;
    camera.pitch = Math.max(-85, Math.min(85, camera.pitch - (event.clientY - drag.y) * 0.3));
    drag = { x: event.clientX, y: event.clientY };
    render();
  });
  canvas.addEventListener('pointerup', () => { drag = null; });

  window.rendererLab = {
    init: render,
    start() {
      if (frameRequest) return;
      lastTime = 0;
      frameRequest = requestAnimationFrame(tick);
    },
    stop() {
      cancelAnimationFrame(frameRequest);
      frameRequest = 0;
      keys.clear();
    },
  };
})();
