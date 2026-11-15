// Lab 02: 2D transforms with homogeneous coordinates, plus a wireframe debug overlay.
// Lab 01's rasterizer is reused unchanged. Everything new happens before it:
//
//   object space --(model matrix M)--> world space --(viewport matrix V)--> pixel space
//
// Work through README.md in this folder. TODO(exercise X) comments mark code you write.
(function () {
  const canvas = document.querySelector('#l2-screen');
  const ctx = canvas.getContext('2d', { alpha: false });
  const image = ctx.createImageData(canvas.width, canvas.height);
  const W = canvas.width;
  const H = canvas.height;
  const byId = (id) => document.getElementById(id);
  const sliders = ['tx', 'ty', 'rot', 'sx', 'sy'].map((name) => byId(`l2-${name}`));
  const DEFAULTS = ['-1', '-1.5', '0', '1', '1'];
  const orderSelect = byId('l2-order');
  const show = { fill: true, wire: true, verts: true, axes: true, ghost: true, grid: true };
  let mouse = null; // last mouse position in pixel coordinates

  // ---------------------------------------------------------------------------
  // 3×3 matrices for column vectors: p' = M · p, with p = [x, y, w].
  // Stored as rows, m[row][col], so the code reads like the math on paper.
  // w = 1 marks a point (translation applies); w = 0 marks a direction (it does not).
  // ---------------------------------------------------------------------------
  function multiply(a, b) {
    const out = [[0, 0, 0], [0, 0, 0], [0, 0, 0]];
    for (let r = 0; r < 3; r++) {
      for (let c = 0; c < 3; c++) {
        out[r][c] = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c];
      }
    }
    return out;
  }
  const identity = () => [[1, 0, 0], [0, 1, 0], [0, 0, 1]];
  const translate = (tx, ty) => [[1, 0, tx], [0, 1, ty], [0, 0, 1]];
  const scale = (sx, sy) => [[sx, 0, 0], [0, sy, 0], [0, 0, 1]];
  function rotate(radians) {
    const c = Math.cos(radians);
    const s = Math.sin(radians);
    return [[c, -s, 0], [s, c, 0], [0, 0, 1]];
  }
  function apply(m, p) {
    return [0, 1, 2].map((r) => m[r][0] * p[0] + m[r][1] * p[1] + m[r][2] * p[2]);
  }
  // Determinant of the linear (upper-left 2×2) part: how much the transform scales
  // signed area. A negative value means the transform mirrors.
  const det2 = (m) => m[0][0] * m[1][1] - m[0][1] * m[1][0];

  // TODO(exercise C): return the inverse of an affine matrix (bottom row 0 0 1),
  // or null when det2(m) is close to zero. The mouse readout uses it to show
  // object-space coordinates; until you write it, that readout says TODO.
  function invert(m) {
    return null;
  }

  // World units -> pixels. The world origin sits at the canvas center and world +y
  // points up, but pixel rows grow downward, so the viewport scales y by a negative number.
  const PIXELS_PER_UNIT = 40;
  const viewport = multiply(translate(W / 2, H / 2), scale(PIXELS_PER_UNIT, -PIXELS_PER_UNIT));
  // Written out by hand. Once invert() works, check that invert(viewport) agrees.
  const pixelToWorld = (px, py) => [(px - W / 2) / PIXELS_PER_UNIT, (H / 2 - py) / PIXELS_PER_UNIT, 1];

  // ---------------------------------------------------------------------------
  // The object: an "F" made of three rectangles (six triangles), in object space.
  // Its origin (0, 0) is the bottom-left corner of the stem, not its center.
  // An F has no mirror symmetry, so a flip or a swapped axis is easy to spot.
  // ---------------------------------------------------------------------------
  const rects = [[0, 0, 0.6, 3], [0.6, 2.4, 2, 3], [0.6, 1.2, 1.5, 1.8]]; // [x0, y0, x1, y1]
  const triangles = rects.flatMap(([x0, y0, x1, y1]) => [
    [[x0, y0], [x1, y0], [x1, y1]], // counter-clockwise in object space (+y up)
    [[x0, y0], [x1, y1], [x0, y1]],
  ]);
  const PINK = [255, 103, 128];
  const TEAL = [86, 225, 192];
  const BLUE = [105, 155, 255];
  // Color comes from object-space position, so it travels with the object.
  function objectColor([x, y]) {
    const u = x / 2;
    const v = y / 3;
    return [0, 1, 2].map((i) => PINK[i] * (1 - u) * (1 - v) + TEAL[i] * u * (1 - v) + BLUE[i] * v);
  }

  // The composed model matrix. "TRS" means M = T · R · S: read right to left,
  // so S touches the point first, then R, then T.
  function modelMatrix() {
    const [tx, ty, degrees, sx, sy] = sliders.map((el) => Number(el.value));
    const parts = { T: translate(tx, ty), R: rotate((degrees * Math.PI) / 180), S: scale(sx, sy) };
    return orderSelect.value.split('').reduce((m, key) => multiply(m, parts[key]), identity());
  }

  // ---------------------------------------------------------------------------
  // Framebuffer helpers. Everything below works in pixel coordinates only.
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

  // DDA line: step one pixel at a time along the longer axis.
  // TODO(exercise D): write an integer-only Bresenham version and compare the pixels.
  function drawLine(a, b, rgb) {
    const dx = b[0] - a[0];
    const dy = b[1] - a[1];
    const steps = Math.ceil(Math.max(Math.abs(dx), Math.abs(dy)));
    if (steps > 4 * (W + H)) return; // far off-screen; clipping lines is a later topic
    for (let i = 0; i <= steps; i++) {
      const t = steps === 0 ? 0 : i / steps;
      plot(Math.floor(a[0] + dx * t), Math.floor(a[1] + dy * t), rgb);
    }
  }

  function drawMarker(p, rgb, radius = 2) {
    const cx = Math.floor(p[0]);
    const cy = Math.floor(p[1]);
    for (let y = cy - radius; y <= cy + radius; y++) {
      for (let x = cx - radius; x <= cx + radius; x++) plot(x, y, rgb);
    }
  }

  // Lab 01's edge function and triangle fill, unchanged apart from [x, y] arrays.
  function edge(a, b, p) {
    return (p[0] - a[0]) * (b[1] - a[1]) - (p[1] - a[1]) * (b[0] - a[0]);
  }

  function fillTriangle(v, colors) {
    const area = edge(v[0], v[1], v[2]);
    if (Math.abs(area) < 1e-5) return 0;
    const minX = Math.max(0, Math.floor(Math.min(v[0][0], v[1][0], v[2][0])));
    const maxX = Math.min(W - 1, Math.ceil(Math.max(v[0][0], v[1][0], v[2][0])));
    const minY = Math.max(0, Math.floor(Math.min(v[0][1], v[1][1], v[2][1])));
    const maxY = Math.min(H - 1, Math.ceil(Math.max(v[0][1], v[1][1], v[2][1])));
    let covered = 0;
    for (let y = minY; y <= maxY; y++) {
      for (let x = minX; x <= maxX; x++) {
        const p = [x + 0.5, y + 0.5];
        const w0 = edge(v[1], v[2], p) / area;
        const w1 = edge(v[2], v[0], p) / area;
        const w2 = edge(v[0], v[1], p) / area;
        if (w0 >= -1e-8 && w1 >= -1e-8 && w2 >= -1e-8) {
          const index = (y * W + x) * 4;
          for (let c = 0; c < 3; c++) {
            image.data[index + c] = Math.round(w0 * colors[0][c] + w1 * colors[1][c] + w2 * colors[2][c]);
          }
          covered++;
        }
      }
    }
    return covered;
  }

  // Standard signed area: positive when a -> b -> c turns counter-clockwise in a +y-up frame.
  const signedArea = (a, b, c) => ((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])) / 2;

  function drawGrid() {
    const minor = [27, 36, 44];
    for (let x = -6; x <= 6; x++) drawLine(apply(viewport, [x, -5, 1]), apply(viewport, [x, 5, 1]), minor);
    for (let y = -4; y <= 4; y++) drawLine(apply(viewport, [-7, y, 1]), apply(viewport, [7, y, 1]), minor);
    drawLine(apply(viewport, [-7, 0, 1]), apply(viewport, [7, 0, 1]), [92, 52, 58]); // world x axis
    drawLine(apply(viewport, [0, -5, 1]), apply(viewport, [0, 5, 1]), [50, 88, 64]); // world y axis
  }

  // ---------------------------------------------------------------------------
  // One frame: transform, rasterize, then draw the debug overlay on top.
  // ---------------------------------------------------------------------------
  function draw() {
    sliders.forEach((el) => { el.nextElementSibling.value = el.value; });
    const model = modelMatrix();
    clear([10, 13, 18]);
    if (show.grid) drawGrid();

    const toPixel = (p) => apply(viewport, p); // world (w = 1) -> pixel
    const worldTris = triangles.map((tri) => tri.map(([x, y]) => apply(model, [x, y, 1])));
    const pixelTris = worldTris.map((tri) => tri.map(toPixel));
    const areas = { object: 0, world: 0, pixel: 0 };
    triangles.forEach((tri, i) => {
      areas.object += signedArea(...tri);
      areas.world += signedArea(...worldTris[i]);
      areas.pixel += signedArea(...pixelTris[i]);
    });

    let covered = 0;
    if (show.fill) {
      pixelTris.forEach((tri, i) => { covered += fillTriangle(tri, triangles[i].map(objectColor)); });
    }
    if (show.ghost) {
      // The same object drawn with M = identity: object coordinates read as world coordinates.
      for (const tri of triangles) {
        const p = tri.map(([x, y]) => toPixel([x, y, 1]));
        for (let e = 0; e < 3; e++) drawLine(p[e], p[(e + 1) % 3], [74, 86, 98]);
      }
    }
    if (show.wire) {
      for (const p of pixelTris) {
        for (let e = 0; e < 3; e++) drawLine(p[e], p[(e + 1) % 3], [236, 242, 247]);
      }
    }
    if (show.axes) {
      // Local axes are directions (w = 0), so translation does not move their tips.
      const origin = apply(model, [0, 0, 1]);
      const xAxis = apply(model, [1, 0, 0]);
      const yAxis = apply(model, [0, 1, 0]);
      const tip = (dir) => toPixel([origin[0] + dir[0], origin[1] + dir[1], 1]);
      drawLine(toPixel(origin), tip(xAxis), [255, 96, 96]);
      drawLine(toPixel(origin), tip(yAxis), [110, 232, 120]);
      drawMarker(tip(xAxis), [255, 96, 96], 1);
      drawMarker(tip(yAxis), [110, 232, 120], 1);
      drawMarker(toPixel(origin), [255, 179, 105], 3);
    }
    if (show.verts) {
      for (const p of pixelTris.flat()) drawMarker(p, [255, 255, 255], 1);
    }
    ctx.putImageData(image, 0, 0);

    byId('l2-matrix').textContent = model
      .map((row) => row.map((v) => (Math.abs(v) < 5e-3 ? 0 : v).toFixed(2).padStart(6)).join(' '))
      .join('\n');
    byId('l2-applied').textContent = orderSelect.value.split('').reverse().join(' → ');
    byId('l2-det').textContent = det2(model).toFixed(2);
    byId('l2-area').textContent = `${areas.object.toFixed(2)} → ${areas.world.toFixed(2)} → ${areas.pixel.toFixed(0)}`;
    byId('l2-covered').textContent = covered.toLocaleString();
    updateMouse(model);
  }

  function updateMouse(model = modelMatrix()) {
    const fmt = (p) => `(${p[0].toFixed(2)}, ${p[1].toFixed(2)})`;
    if (!mouse) {
      byId('l2-mouse').textContent = '—';
      return;
    }
    const world = pixelToWorld(mouse[0], mouse[1]);
    const inverse = invert(model);
    const object = inverse ? fmt(apply(inverse, world)) : 'TODO (C)';
    byId('l2-mouse').textContent =
      `px    ${fmt(mouse)}\nworld ${fmt(world)}\nobj   ${object}`;
  }

  canvas.addEventListener('mousemove', (event) => {
    const rect = canvas.getBoundingClientRect();
    mouse = [
      ((event.clientX - rect.left) * W) / rect.width,
      ((event.clientY - rect.top) * H) / rect.height,
    ];
    updateMouse();
  });
  canvas.addEventListener('mouseleave', () => { mouse = null; updateMouse(); });
  sliders.forEach((el) => el.addEventListener('input', draw));
  orderSelect.addEventListener('change', draw);
  document.querySelectorAll('[data-l2-toggle]').forEach((button) => {
    const key = button.dataset.l2Toggle;
    button.setAttribute('aria-pressed', String(show[key]));
    button.addEventListener('click', () => {
      show[key] = !show[key];
      button.setAttribute('aria-pressed', String(show[key]));
      draw();
    });
  });
  byId('l2-reset').addEventListener('click', () => {
    DEFAULTS.forEach((value, i) => { sliders[i].value = value; });
    orderSelect.value = 'TRS';
    draw();
  });

  window.transformLab = { init: draw };
})();
