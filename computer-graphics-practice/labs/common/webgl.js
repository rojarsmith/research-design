// Shared WebGL 2 helpers for Lab 07 and later: programs, meshes, textures, render targets
// and a GPU timer. Labs 05 and 06 write the first two out in full in their own app.js; read
// them there first.
//
// A classic script on window.cg.webgl, like math.js, so index.html keeps working over file://.
//
// Color rule (../../CONVENTIONS.md): color render targets here are SRGB8_ALPHA8. The GPU
// encodes linear values to sRGB when a fragment is written into them and decodes them back
// to linear light when the texture is sampled, so shaders that draw into them output linear
// light. The last pass, into the canvas, encodes once with linearToSrgb.
(function (cg) {
  const webgl = {};

  // The sRGB transfer functions from ../common/color.js as GLSL, pasted into shaders that need them.
  webgl.GLSL_COLOR = `
vec3 srgbToLinear(vec3 c) {
  return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}
vec3 linearToSrgb(vec3 c) {
  return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}
`;

  // One triangle that covers the whole viewport, built from gl_VertexID, so a full-screen
  // pass needs no vertex buffer: drawArrays(TRIANGLES, 0, 3). vUV runs 0..1 across the viewport.
  webgl.FULLSCREEN_VERTEX_SHADER = `#version 300 es
out vec2 vUV;
void main() {
  vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));  // (0, 0), (2, 0), (0, 2)
  vUV = p;
  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
`;

  function compile(gl, type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      const log = gl.getShaderInfoLog(shader);
      gl.deleteShader(shader);
      throw new Error(log);
    }
    return shader;
  }

  // Compile both stages, link them, and look up every active uniform by name, so a lab can
  // write gl.uniform1f(uniforms.uBias, 0.002) without listing the names twice.
  webgl.program = (gl, vertexSource, fragmentSource) => {
    const vs = compile(gl, gl.VERTEX_SHADER, vertexSource);
    const fs = compile(gl, gl.FRAGMENT_SHADER, fragmentSource);
    const program = gl.createProgram();
    gl.attachShader(program, vs);
    gl.attachShader(program, fs);
    gl.linkProgram(program);
    gl.deleteShader(vs);
    gl.deleteShader(fs);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(program));
    const uniforms = {};
    const count = gl.getProgramParameter(program, gl.ACTIVE_UNIFORMS);
    for (let i = 0; i < count; i++) {
      const name = gl.getActiveUniform(program, i).name.replace(/\[0\]$/, '');
      uniforms[name] = gl.getUniformLocation(program, name);
    }
    return { program, uniforms };
  };

  // Upload a mesh from ./mesh.js once: one interleaved vertex buffer, one index buffer, and a
  // vertex array object that remembers the layout (location 0 position, 1 normal, 2 uv).
  webgl.uploadMesh = (gl, { vertices, indices }) => {
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
    return { vao, count: indices.length, vertices: vertices.length / 8, triangles: indices.length / 3 };
  };

  // An 8-bit image (a canvas or a loaded <img>) as a repeating texture with a full mip chain
  // and trilinear filtering. srgb: true stores it as SRGB8_ALPHA8, so the GPU decodes it to
  // linear light before filtering, as in Lab 06.
  webgl.imageTexture = (gl, image, { srgb = true } = {}) => {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texImage2D(gl.TEXTURE_2D, 0, srgb ? gl.SRGB8_ALPHA8 : gl.RGBA8, gl.RGBA, gl.UNSIGNED_BYTE, image);
    gl.generateMipmap(gl.TEXTURE_2D);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    return texture;
  };

  function checkFramebuffer(gl, what) {
    const status = gl.checkFramebufferStatus(gl.FRAMEBUFFER);
    if (status !== gl.FRAMEBUFFER_COMPLETE) throw new Error(`${what}: framebuffer incomplete (0x${status.toString(16)})`);
  }

  // The render target of a shadow pass: a square depth texture attached to a framebuffer
  // that has no color attachment at all. The texture compares instead of returning depth
  // (TEXTURE_COMPARE_MODE), so shaders read it through a sampler2DShadow:
  //   texture(map, vec3(uv, reference)) is 1.0 where reference ≤ stored depth, else 0.0.
  // A sampler object with the compare mode switched off reads the raw depth (debug views).
  webgl.depthTarget = (gl, size) => {
    const texture = gl.createTexture();
    const framebuffer = gl.createFramebuffer();
    const target = { texture, framebuffer, size: 0 };
    // Re-allocate when the resolution changes. 24-bit depth is usually stored in 4 bytes.
    target.resize = (next) => {
      if (next === target.size) return;
      gl.bindTexture(gl.TEXTURE_2D, texture);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.DEPTH_COMPONENT24, next, next, 0, gl.DEPTH_COMPONENT, gl.UNSIGNED_INT, null);
      target.size = next;
    };
    target.resize(size);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_COMPARE_MODE, gl.COMPARE_REF_TO_TEXTURE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_COMPARE_FUNC, gl.LEQUAL);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.TEXTURE_2D, texture, 0);
    checkFramebuffer(gl, 'shadow map');
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    return target;
  };

  // A sampler object that reads a depth texture as plain numbers: no comparison, nearest texel.
  webgl.rawDepthSampler = (gl) => {
    const sampler = gl.createSampler();
    gl.samplerParameteri(sampler, gl.TEXTURE_COMPARE_MODE, gl.NONE);
    gl.samplerParameteri(sampler, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.samplerParameteri(sampler, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.samplerParameteri(sampler, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.samplerParameteri(sampler, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    return sampler;
  };

  // The render target of a camera pass, with a 24-bit depth buffer.
  //   samples = 1: an SRGB8_ALPHA8 texture that the next pass can sample (bilinear, clamped).
  //   samples > 1: multisample renderbuffers (MSAA). They cannot be sampled; copy them into a
  //                1-sample target with webgl.resolve first.
  webgl.colorTarget = (gl, width, height, samples = 1) => {
    const count = Math.min(samples, gl.getParameter(gl.MAX_SAMPLES));
    const framebuffer = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);
    let texture = null;
    let colorBuffer = null;
    if (count > 1) {
      colorBuffer = gl.createRenderbuffer();
      gl.bindRenderbuffer(gl.RENDERBUFFER, colorBuffer);
      gl.renderbufferStorageMultisample(gl.RENDERBUFFER, count, gl.SRGB8_ALPHA8, width, height);
      gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.RENDERBUFFER, colorBuffer);
    } else {
      texture = gl.createTexture();
      gl.bindTexture(gl.TEXTURE_2D, texture);
      gl.texStorage2D(gl.TEXTURE_2D, 1, gl.SRGB8_ALPHA8, width, height);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
      gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);
    }
    const depthBuffer = gl.createRenderbuffer();
    gl.bindRenderbuffer(gl.RENDERBUFFER, depthBuffer);
    if (count > 1) gl.renderbufferStorageMultisample(gl.RENDERBUFFER, count, gl.DEPTH_COMPONENT24, width, height);
    else gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT24, width, height);
    gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, depthBuffer);
    checkFramebuffer(gl, `color target ${width} × ${height} × ${count}`);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    return {
      framebuffer, texture, width, height, samples: Math.max(1, count),
      // Bytes of GPU memory: 4 for color plus 4 for depth, per sample.
      bytes: width * height * Math.max(1, count) * 8,
      dispose() {
        gl.deleteFramebuffer(framebuffer);
        if (texture) gl.deleteTexture(texture);
        if (colorBuffer) gl.deleteRenderbuffer(colorBuffer);
        gl.deleteRenderbuffer(depthBuffer);
      },
    };
  };

  // Copy the color of `from` into the 1-sample target `to` of the same size. For an MSAA
  // source this is the resolve: the GPU averages each pixel's samples into one value.
  webgl.resolve = (gl, from, to) => {
    gl.bindFramebuffer(gl.READ_FRAMEBUFFER, from.framebuffer);
    gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, to.framebuffer);
    gl.blitFramebuffer(0, 0, from.width, from.height, 0, 0, to.width, to.height, gl.COLOR_BUFFER_BIT, gl.NEAREST);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
  };

  // GPU time per pass from EXT_disjoint_timer_query_webgl2, when the browser exposes it.
  // Results arrive a few frames late, so begin/end only queue queries; poll() collects the
  // finished ones and returns smoothed milliseconds per label. Returns null when unavailable:
  // then the only honest cost measurements are controlled frame-time comparisons.
  webgl.gpuTimer = (gl) => {
    const ext = gl.getExtension('EXT_disjoint_timer_query_webgl2');
    if (!ext) return null;
    const pending = [];
    const results = {};
    let active = null;
    return {
      begin(label) {
        if (active) return; // only one TIME_ELAPSED query may run at a time
        active = { query: gl.createQuery(), label };
        gl.beginQuery(ext.TIME_ELAPSED_EXT, active.query);
      },
      end() {
        if (!active) return;
        gl.endQuery(ext.TIME_ELAPSED_EXT);
        pending.push(active);
        active = null;
      },
      poll() {
        const disjoint = gl.getParameter(ext.GPU_DISJOINT_EXT); // true: the clock jumped, discard
        while (pending.length && gl.getQueryParameter(pending[0].query, gl.QUERY_RESULT_AVAILABLE)) {
          const { query, label } = pending.shift();
          const ms = gl.getQueryParameter(query, gl.QUERY_RESULT) / 1e6;
          gl.deleteQuery(query);
          if (!disjoint) results[label] = results[label] === undefined ? ms : results[label] * 0.9 + ms * 0.1;
        }
        while (pending.length > 60) gl.deleteQuery(pending.shift().query);
        return results;
      },
    };
  };

  // Which adapter runs the shaders. Many browsers hide the name unless asked for the unmasked one.
  webgl.describeGpu = (gl) => {
    let renderer = gl.getParameter(gl.RENDERER);
    const info = gl.getExtension('WEBGL_debug_renderer_info');
    if (info && /^WebKit|^Mozilla/.test(renderer)) renderer = gl.getParameter(info.UNMASKED_RENDERER_WEBGL);
    return `${renderer}\n${gl.getParameter(gl.VERSION)}`;
  };

  cg.webgl = webgl;
})((window.cg = window.cg || {}));
