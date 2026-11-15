// Lab 03: the first 3D image. Still a CPU software renderer; the new parts are
// 4×4 matrices, a camera, a projection, and the perspective divide.
//
//   object --M--> world --V--> view --P--> clip --(÷ w)--> NDC --(viewport)--> pixel
//
// Conventions used here (every renderer has to choose; write yours down):
//   - column vectors, p' = M · p, stored column-major: m[col * 4 + row].
//     That is the layout WebGL expects, so Lab 05 can upload these arrays unchanged.
//   - right-handed world with +y up; the camera looks down its own −z axis.
//   - OpenGL / WebGL clip volume: −w ≤ x, y, z ≤ w, so NDC is the cube [−1, 1]³.
//
// Work through README.md in this folder. TODO(exercise X) comments mark code you write.
(function () {
  const canvas = document.querySelector('#l3-screen');
  const ctx = canvas.getContext('2d', { alpha: false });
  const image = ctx.createImageData(canvas.width, canvas.height);
  const W = canvas.width;
  const H = canvas.height;
  const byId = (id) => document.getElementById(id);
  const DEFAULTS = { yaw: 30, pitch: 20, dist: 6, fov: 60, near: 0.5, far: 12 };
  const sliders = Object.fromEntries(Object.keys(DEFAULTS).map((key) => [key, byId(`l3-${key}`)]));
  const projectionSelect = byId('l3-projection');
  const modeSelect = byId('l3-mode');
  const viewSelect = byId('l3-view');
  const traceSelect = byId('l3-trace');
  const spinButton = byId('l3-spin');
  const SPIN_SPEED = 0.6; // radians per second
  let spinning = true;
  let angle = 0; // model rotation about +y, in radians
  let frameRequest = 0; // the animation loop runs only while this lab is visible
  let lastTime = 0;
  let frameMs = 0;

  // ---------------------------------------------------------------------------
  // Small vector helpers for 3D points and directions.
  // ---------------------------------------------------------------------------
  const add = (a, b) => [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
  const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
  const scale = (a, s) => [a[0] * s, a[1] * s, a[2] * s];
  const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
  const normalize = (a) => scale(a, 1 / Math.hypot(a[0], a[1], a[2]));
  const radians = (degrees) => (degrees * Math.PI) / 180;

  // ---------------------------------------------------------------------------
  // 4×4 matrices. fromRows lets you *write* a matrix row by row, as on paper,
  // and stores it column-major. at(m, row, col) reads one entry back.
  // ---------------------------------------------------------------------------
  const fromRows = (r) => [0, 1, 2, 3].flatMap((c) => [r[c], r[4 + c], r[8 + c], r[12 + c]]);
  const at = (m, row, col) => m[col * 4 + row];

  function multiply(a, b) {
    const out = new Array(16).fill(0);
    for (let c = 0; c < 4; c++) {
      for (let r = 0; r < 4; r++) {
        let sum = 0;
        for (let k = 0; k < 4; k++) sum += at(a, r, k) * at(b, k, c);
        out[c * 4 + r] = sum;
      }
    }
    return out;
  }

  // v = [x, y, z, w]; w = 1 for points, 0 for directions (as in Lab 02).
  function transform(m, v) {
    return [0, 1, 2, 3].map((r) => at(m, r, 0) * v[0] + at(m, r, 1) * v[1] + at(m, r, 2) * v[2] + at(m, r, 3) * v[3]);
  }

  function rotationY(a) {
    const c = Math.cos(a);
    const s = Math.sin(a);
    return fromRows([c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1]);
  }

  // A camera is placed in the world like any object. The view matrix undoes that
  // placement: its first three rows are the camera's right, up and back axes, written
  // in world coordinates, and its last column moves the eye to the origin.
  function lookAt(eye, target, worldUp) {
    const forward = normalize(sub(target, eye));
    const right = normalize(cross(forward, worldUp));
    const up = cross(right, forward);
    const view = fromRows([
      ...right, -dot(right, eye),
      ...up, -dot(up, eye),
      ...scale(forward, -1), dot(forward, eye),
      0, 0, 0, 1,
    ]);
    return { view, eye, right, up, forward };
  }

  // Perspective: squeezes the view frustum into the clip volume and copies −z_view
  // into w, so the later divide by w shrinks distant points.
  function perspective(fovY, aspect, near, far) {
    const f = 1 / Math.tan(fovY / 2);
    return fromRows([
      f / aspect, 0, 0, 0,
      0, f, 0, 0,
      0, 0, (far + near) / (near - far), (2 * far * near) / (near - far),
      0, 0, -1, 0,
    ]);
  }

  // Orthographic: a box instead of a frustum. w stays 1, so the divide changes nothing.
  function orthographic(halfHeight, aspect, near, far) {
    return fromRows([
      1 / (halfHeight * aspect), 0, 0, 0,
      0, 1 / halfHeight, 0, 0,
      0, 0, -2 / (far - near), -(far + near) / (far - near),
      0, 0, 0, 1,
    ]);
  }

  // NDC -> pixel. NDC +y points up and pixel rows grow down, as in Lab 02.
  const ndcToPixel = (ndc) => [((ndc[0] + 1) / 2) * W, ((1 - ndc[1]) / 2) * H];

  // ---------------------------------------------------------------------------
  // The mesh: a cube from (−1, −1, −1) to (1, 1, 1), built from 12 triangles that
  // index into 8 shared positions. Each triangle is counter-clockwise when seen
  // from outside the cube; Lab 04 depends on that.
  // ---------------------------------------------------------------------------
  const CUBE = {
    positions: [
      [-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
      [-1, -1, 1], [1, -1, 1], [1, 1, 1], [-1, 1, 1],
    ],
    triangles: [
      [4, 5, 6], [4, 6, 7], // +z
      [1, 0, 3], [1, 3, 2], // −z
      [5, 1, 2], [5, 2, 6], // +x
      [0, 4, 7], [0, 7, 3], // −x
      [7, 6, 2], [7, 2, 3], // +y
      [0, 1, 5], [0, 5, 4], // −y
    ],
    faceColors: [
      [255, 103, 128], [86, 225, 192], [105, 155, 255],
      [255, 179, 105], [190, 140, 255], [240, 220, 110],
    ],
  };
  // A ground grid one unit below the cube gives the eye something to converge.
  const GRID = [];
  for (let i = -3; i <= 3; i++) {
    GRID.push([[i, -1.5, -3], [i, -1.5, 3]], [[-3, -1.5, i], [3, -1.5, i]]);
  }

  // ---------------------------------------------------------------------------
  // Cameras.
  // ---------------------------------------------------------------------------
  function mainCamera() {
    const yaw = radians(Number(sliders.yaw.value));
    const pitch = radians(Number(sliders.pitch.value));
    const dist = Number(sliders.dist.value);
    const eye = [
      dist * Math.cos(pitch) * Math.sin(yaw),
      dist * Math.sin(pitch),
      dist * Math.cos(pitch) * Math.cos(yaw),
    ];
    const camera = lookAt(eye, [0, 0, 0], [0, 1, 0]);
    camera.fovY = radians(Number(sliders.fov.value));
    camera.aspect = W / H;
    camera.near = Number(sliders.near.value);
    camera.far = Number(sliders.far.value);
    camera.orthographic = projectionSelect.value === 'ortho';
    camera.projection = camera.orthographic
      // Size the box so the target looks the same size as with perspective.
      ? orthographic(dist * Math.tan(camera.fovY / 2), camera.aspect, camera.near, camera.far)
      : perspective(camera.fovY, camera.aspect, camera.near, camera.far);
    return camera;
  }

  // A second camera beside and above the main one, aimed between the cube and the
  // main camera, so you can see both the scene and the camera that renders it.
  function observerCamera(main) {
    const yaw = radians(Number(sliders.yaw.value) + 75);
    const pitch = radians(35);
    const dist = Number(sliders.dist.value) + 9;
    const target = scale(main.eye, 0.5);
    const offset = [dist * Math.cos(pitch) * Math.sin(yaw), dist * Math.sin(pitch), dist * Math.cos(pitch) * Math.cos(yaw)];
    const camera = lookAt(add(target, offset), target, [0, 1, 0]);
    camera.projection = perspective(radians(50), W / H, 0.5, 200);
    return camera;
  }

  // TODO(exercise D): return the 8 world-space corners of the main camera's view
  // volume, ordered near [bottom-left, bottom-right, top-right, top-left], then far
  // in the same order. Use camera.eye/right/up/forward, fovY, aspect, near and far.
  // The observer view draws the frustum as soon as this returns 8 points.
  function frustumCorners(camera) {
    return [];
  }

  // ---------------------------------------------------------------------------
  // Framebuffer helpers (pixel coordinates only).
  // ---------------------------------------------------------------------------
  function clear(rgb) {
    for (let i = 0; i < image.data.length; i += 4) {
      image.data[i] = rgb[0];
      image.data[i + 1] = rgb[1];
      image.data[i + 2] = rgb[2];
      image.data[i + 3] = 255;
    }
  }

  function plot(x, y, rgb) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    const index = (y * W + x) * 4;
    image.data[index] = rgb[0];
    image.data[index + 1] = rgb[1];
    image.data[index + 2] = rgb[2];
  }

  // Clip the segment to the canvas rectangle (Liang–Barsky), then step with a DDA.
  function drawLine(a, b, rgb) {
    const dx = b[0] - a[0];
    const dy = b[1] - a[1];
    let t0 = 0;
    let t1 = 1;
    for (const [p, q] of [[-dx, a[0]], [dx, W - 1e-3 - a[0]], [-dy, a[1]], [dy, H - 1e-3 - a[1]]]) {
      if (p === 0) {
        if (q < 0) return;
      } else if (p < 0) {
        t0 = Math.max(t0, q / p);
      } else {
        t1 = Math.min(t1, q / p);
      }
      if (t0 > t1) return;
    }
    const x0 = a[0] + dx * t0;
    const y0 = a[1] + dy * t0;
    const steps = Math.ceil(Math.max(Math.abs(dx), Math.abs(dy)) * (t1 - t0));
    for (let i = 0; i <= steps; i++) {
      const t = steps === 0 ? 0 : i / steps;
      plot(Math.floor(x0 + dx * (t1 - t0) * t), Math.floor(y0 + dy * (t1 - t0) * t), rgb);
    }
  }

  function drawMarker(p, rgb, radius) {
    for (let y = Math.floor(p[1]) - radius; y <= Math.floor(p[1]) + radius; y++) {
      for (let x = Math.floor(p[0]) - radius; x <= Math.floor(p[0]) + radius; x++) plot(x, y, rgb);
    }
  }

  // Lab 01's edge-function fill, with one flat color per triangle.
  function edge(a, b, p) {
    return (p[0] - a[0]) * (b[1] - a[1]) - (p[1] - a[1]) * (b[0] - a[0]);
  }

  function fillTriangle(v, rgb) {
    const area = edge(v[0], v[1], v[2]);
    if (Math.abs(area) < 1e-5) return;
    const minX = Math.max(0, Math.floor(Math.min(v[0][0], v[1][0], v[2][0])));
    const maxX = Math.min(W - 1, Math.ceil(Math.max(v[0][0], v[1][0], v[2][0])));
    const minY = Math.max(0, Math.floor(Math.min(v[0][1], v[1][1], v[2][1])));
    const maxY = Math.min(H - 1, Math.ceil(Math.max(v[0][1], v[1][1], v[2][1])));
    for (let y = minY; y <= maxY; y++) {
      for (let x = minX; x <= maxX; x++) {
        const p = [x + 0.5, y + 0.5];
        const w0 = edge(v[1], v[2], p) / area;
        const w1 = edge(v[2], v[0], p) / area;
        const w2 = edge(v[0], v[1], p) / area;
        if (w0 >= -1e-8 && w1 >= -1e-8 && w2 >= -1e-8) plot(x, y, rgb);
      }
    }
  }

  // ---------------------------------------------------------------------------
  // The pipeline.
  // ---------------------------------------------------------------------------
  // A point is in front of the near plane when z ≥ −w in clip space. For a
  // perspective camera this also rejects points behind the eye, where w < 0.
  const inFrontOfNear = (clip) => clip[2] >= -clip[3];
  const clipToPixel = (clip) => ndcToPixel([clip[0] / clip[3], clip[1] / clip[3]]);

  // Debug lines only: keep the part of a segment in front of the near plane.
  // Lab 04 does the same job properly for triangles.
  function drawSegment3D(viewProj, a, b, rgb) {
    let p = transform(viewProj, [...a, 1]);
    let q = transform(viewProj, [...b, 1]);
    const dp = p[2] + p[3];
    const dq = q[2] + q[3];
    if (dp < 0 && dq < 0) return;
    if (dp < 0 || dq < 0) {
      const t = dp / (dp - dq);
      const cut = p.map((value, i) => value + (q[i] - value) * t);
      if (dp < 0) p = cut;
      else q = cut;
    }
    drawLine(clipToPixel(p), clipToPixel(q), rgb);
  }

  function drawScene(camera, model) {
    const viewProj = multiply(camera.projection, camera.view);
    for (const [a, b] of GRID) drawSegment3D(viewProj, a, b, [40, 54, 64]);

    const mvp = multiply(viewProj, model);
    const clip = CUBE.positions.map((p) => transform(mvp, [...p, 1]));
    const pixel = clip.map(clipToPixel);
    const mode = modeSelect.value;
    // No depth test and no culling yet: triangles are simply filled in array order.
    // TODO(exercise B): sort the triangles back to front first (painter's algorithm).
    const order = CUBE.triangles.map((_, i) => i);
    const drawnTriangles = [];
    let skipped = 0;
    for (const i of order) {
      const tri = CUBE.triangles[i];
      // Without clipping, a triangle that crosses the near plane is simply skipped.
      if (!tri.every((k) => inFrontOfNear(clip[k]))) {
        skipped++;
        continue;
      }
      if (mode !== 'wire') fillTriangle(tri.map((k) => pixel[k]), CUBE.faceColors[i >> 1]);
      drawnTriangles.push(tri);
    }
    if (mode !== 'fill') {
      for (const tri of drawnTriangles) {
        for (let e = 0; e < 3; e++) drawLine(pixel[tri[e]], pixel[tri[(e + 1) % 3]], [236, 242, 247]);
      }
    }
    const traced = Number(traceSelect.value);
    if (inFrontOfNear(clip[traced])) drawMarker(pixel[traced], [255, 255, 255], 3);
    return { drawn: drawnTriangles.length, skipped };
  }

  // The observer sees the main camera as an object: its eye, its axes and (once you
  // write frustumCorners) its view volume.
  function drawCameraGizmo(observer, main) {
    const viewProj = multiply(observer.projection, observer.view);
    const eye = main.eye;
    drawSegment3D(viewProj, eye, [0, 0, 0], [70, 84, 96]);
    drawSegment3D(viewProj, eye, add(eye, scale(main.right, 1.5)), [255, 96, 96]);
    drawSegment3D(viewProj, eye, add(eye, scale(main.up, 1.5)), [110, 232, 120]);
    drawSegment3D(viewProj, eye, add(eye, scale(main.forward, -1.5)), [110, 150, 255]);
    const corners = frustumCorners(main);
    if (corners.length === 8) {
      for (let i = 0; i < 4; i++) {
        drawSegment3D(viewProj, corners[i], corners[(i + 1) % 4], [255, 179, 105]);
        drawSegment3D(viewProj, corners[4 + i], corners[4 + ((i + 1) % 4)], [255, 179, 105]);
        drawSegment3D(viewProj, corners[i], corners[4 + i], [255, 179, 105]);
      }
    }
    const clip = transform(viewProj, [...eye, 1]);
    if (inFrontOfNear(clip)) drawMarker(clipToPixel(clip), [255, 255, 255], 2);
    return corners.length === 8;
  }

  // ---------------------------------------------------------------------------
  // Readouts.
  // ---------------------------------------------------------------------------
  const num = (v) => (Math.abs(v) < 5e-4 ? 0 : v).toFixed(2).padStart(6);
  const vec = (v) => `(${v.map(num).join(',')})`;
  const rows = (m) => [0, 1, 2, 3].map((r) => [0, 1, 2, 3].map((c) => num(at(m, r, c))).join(' ')).join('\n');

  function updateTrace(camera, model) {
    const k = Number(traceSelect.value);
    const object = [...CUBE.positions[k], 1];
    const world = transform(model, object);
    const view = transform(camera.view, world);
    const clip = transform(camera.projection, view);
    const ndc = clip.slice(0, 3).map((v) => v / clip[3]);
    const inside = ['x', 'y', 'z'].map((axis, i) => `${axis} ${Math.abs(clip[i]) <= clip[3] ? '✓' : '✗'}`);
    byId('l3-t-object').textContent = vec(object.slice(0, 3));
    byId('l3-t-world').textContent = vec(world.slice(0, 3));
    byId('l3-t-view').textContent = vec(view.slice(0, 3));
    byId('l3-t-clip').textContent = vec(clip);
    byId('l3-t-ndc').textContent = vec(ndc);
    byId('l3-t-pixel').textContent = vec(ndcToPixel(ndc));
    byId('l3-t-inside').textContent = inside.join('   ');
  }

  function render() {
    const started = performance.now();
    Object.values(sliders).forEach((el) => { el.nextElementSibling.value = el.value; });
    const camera = mainCamera();
    const model = rotationY(angle);
    clear([10, 13, 18]);
    let stats;
    let frustumDone = false;
    if (viewSelect.value === 'observer') {
      const observer = observerCamera(camera);
      stats = drawScene(observer, model);
      frustumDone = drawCameraGizmo(observer, camera);
    } else {
      stats = drawScene(camera, model);
    }
    ctx.putImageData(image, 0, 0);
    updateTrace(camera, model);
    byId('l3-view-matrix').textContent = rows(camera.view);
    byId('l3-proj-matrix').textContent = rows(camera.projection);
    byId('l3-tris').textContent = `${stats.drawn} / ${stats.skipped}`;
    byId('l3-frustum').textContent = viewSelect.value !== 'observer' ? '—' : frustumDone ? '8 / 8' : 'TODO (D)';
    frameMs = frameMs * 0.9 + (performance.now() - started) * 0.1;
    byId('l3-frame').textContent = `${frameMs.toFixed(2)} ms`;
  }

  function frame(time) {
    const dt = lastTime ? Math.min(0.1, (time - lastTime) / 1000) : 0;
    lastTime = time;
    if (spinning) angle += dt * SPIN_SPEED;
    render();
    frameRequest = requestAnimationFrame(frame);
  }

  // ---------------------------------------------------------------------------
  // Controls: sliders, selects, and drag-to-orbit on the canvas.
  // ---------------------------------------------------------------------------
  Object.values(sliders).forEach((el) => el.addEventListener('input', render));
  [projectionSelect, modeSelect, viewSelect, traceSelect].forEach((el) => el.addEventListener('change', render));
  spinButton.setAttribute('aria-pressed', 'true');
  spinButton.addEventListener('click', () => {
    spinning = !spinning;
    spinButton.setAttribute('aria-pressed', String(spinning));
  });
  byId('l3-reset').addEventListener('click', () => {
    Object.entries(DEFAULTS).forEach(([key, value]) => { sliders[key].value = value; });
    projectionSelect.value = 'persp';
    angle = 0;
    render();
  });
  let drag = null;
  canvas.addEventListener('pointerdown', (event) => {
    drag = { x: event.clientX, y: event.clientY };
    canvas.setPointerCapture(event.pointerId);
  });
  canvas.addEventListener('pointermove', (event) => {
    if (!drag) return;
    const yaw = Number(sliders.yaw.value) - (event.clientX - drag.x) * 0.5;
    sliders.yaw.value = String(((yaw + 540) % 360) - 180); // wrap into [−180, 180)
    sliders.pitch.value = String(Number(sliders.pitch.value) + (event.clientY - drag.y) * 0.5);
    drag = { x: event.clientX, y: event.clientY };
    render();
  });
  canvas.addEventListener('pointerup', () => { drag = null; });

  window.cameraLab = {
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
