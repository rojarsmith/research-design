// Shared math for Lab 03 and later: 3D vectors, 4×4 matrices, cameras and projections.
//
// This is a classic script, not an ES module, on purpose: index.html must keep working
// when it is opened straight from disk, and browsers block module imports over file://.
// Everything is attached to one namespace object, window.cg.math. A lab picks what it needs:
//
//   const { multiply, transform, perspective } = window.cg.math;
//
// Changing a function here changes every lab that uses it. To experiment, copy the
// function into your lab's app.js first.
//
// Conventions (the full list, and how each one changes in Vulkan and Direct3D 12,
// is in ../../CONVENTIONS.md):
//   - Column vectors: p' = M · p, so in P · V · M · p the model matrix M acts first.
//   - Storage: a 4×4 matrix is a flat array of 16 numbers in column-major order,
//     m[col * 4 + row]. fromRows() lets you write a matrix row by row, as on paper.
//     WebGL's uniformMatrix4fv(location, false, m) reads this layout unchanged.
//   - Right-handed world and view space with +y up; a camera looks down its own −z axis.
//   - perspective() and orthographic() target the OpenGL / WebGL clip volume
//     −w ≤ x, y, z ≤ w, so NDC is the cube [−1, 1]³ with +y up.
//   - ndcToPixel() maps NDC to pixel coordinates with +y down (row 0 at the top).
//     Pixel (x, y) has its center, the sample point, at (x + 0.5, y + 0.5).
//   - Angles are radians in code. The page's sliders show degrees.
(function (cg) {
  const math = {};

  math.radians = (degrees) => (degrees * Math.PI) / 180;

  // ---------------------------------------------------------------------------
  // 3D vectors as plain [x, y, z] arrays.
  // ---------------------------------------------------------------------------
  math.add = (a, b) => [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
  math.sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
  math.scale = (a, s) => [a[0] * s, a[1] * s, a[2] * s];
  math.dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  math.cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
  math.length = (a) => Math.hypot(a[0], a[1], a[2]);
  math.normalize = (a) => math.scale(a, 1 / math.length(a));

  // ---------------------------------------------------------------------------
  // 4×4 matrices, column-major.
  // ---------------------------------------------------------------------------
  // Write 16 numbers row by row, as on paper; get the column-major array back.
  math.fromRows = (r) => [0, 1, 2, 3].flatMap((c) => [r[c], r[4 + c], r[8 + c], r[12 + c]]);
  // Read one entry back by row and column.
  math.at = (m, row, col) => m[col * 4 + row];
  math.identity = () => math.fromRows([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);

  // a · b: applying the result means applying b first, then a.
  math.multiply = (a, b) => {
    const out = new Array(16).fill(0);
    for (let c = 0; c < 4; c++) {
      for (let r = 0; r < 4; r++) {
        let sum = 0;
        for (let k = 0; k < 4; k++) sum += math.at(a, r, k) * math.at(b, k, c);
        out[c * 4 + r] = sum;
      }
    }
    return out;
  };

  // M · v for v = [x, y, z, w]: w = 1 for points, w = 0 for directions.
  math.transform = (m, v) => [0, 1, 2, 3].map((r) =>
    math.at(m, r, 0) * v[0] + math.at(m, r, 1) * v[1] + math.at(m, r, 2) * v[2] + math.at(m, r, 3) * v[3]);

  math.translation = (x, y, z) => math.fromRows([1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1]);
  math.scaling = (x, y, z) => math.fromRows([x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1]);
  // Rotations follow the right-hand rule: positive angles turn counter-clockwise when
  // you look from the positive end of the axis toward the origin.
  math.rotationX = (a) => {
    const c = Math.cos(a);
    const s = Math.sin(a);
    return math.fromRows([1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0, 0, 0, 0, 1]);
  };
  math.rotationY = (a) => {
    const c = Math.cos(a);
    const s = Math.sin(a);
    return math.fromRows([c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1]);
  };
  math.rotationZ = (a) => {
    const c = Math.cos(a);
    const s = Math.sin(a);
    return math.fromRows([c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
  };

  // ---------------------------------------------------------------------------
  // Cameras.
  // ---------------------------------------------------------------------------
  // A point on a sphere around `target`: yaw turns around +y, pitch lifts toward +y.
  // Yaw 0 and pitch 0 put the eye on the +z side of the target.
  math.orbitEye = (yawDegrees, pitchDegrees, distance, target = [0, 0, 0]) => {
    const yaw = math.radians(yawDegrees);
    const pitch = math.radians(pitchDegrees);
    return math.add(target, [
      distance * Math.cos(pitch) * Math.sin(yaw),
      distance * Math.sin(pitch),
      distance * Math.cos(pitch) * Math.cos(yaw),
    ]);
  };

  // The camera's own axes in world coordinates. It looks along `forward`, which is
  // its −z axis, so its +z axis ("back") is −forward.
  math.cameraBasis = (eye, target, worldUp) => {
    const forward = math.normalize(math.sub(target, eye));
    const right = math.normalize(math.cross(forward, worldUp));
    const up = math.cross(right, forward);
    return { eye, right, up, forward };
  };

  // The view matrix undoes the camera's placement in the world. Its first three rows
  // are the camera's right, up and back axes; its last column moves the eye to the origin.
  math.viewMatrix = ({ eye, right, up, forward }) => math.fromRows([
    ...right, -math.dot(right, eye),
    ...up, -math.dot(up, eye),
    ...math.scale(forward, -1), math.dot(forward, eye),
    0, 0, 0, 1,
  ]);

  math.lookAt = (eye, target, worldUp) => math.viewMatrix(math.cameraBasis(eye, target, worldUp));

  // ---------------------------------------------------------------------------
  // Projections into the OpenGL / WebGL clip volume (NDC z in [−1, 1]).
  // In Vulkan and Direct3D the clip volume is 0 ≤ z ≤ w instead, and in Vulkan NDC
  // +y points down; see CONVENTIONS.md before reusing these there.
  // ---------------------------------------------------------------------------
  // Squeezes the view frustum into the clip volume and copies −z_view (the distance in
  // front of the camera) into w, so the divide by w shrinks distant points.
  math.perspective = (fovY, aspect, near, far) => {
    const f = 1 / Math.tan(fovY / 2);
    return math.fromRows([
      f / aspect, 0, 0, 0,
      0, f, 0, 0,
      0, 0, (far + near) / (near - far), (2 * far * near) / (near - far),
      0, 0, -1, 0,
    ]);
  };

  // A box instead of a frustum. w stays 1, so the divide changes nothing.
  math.orthographic = (halfHeight, aspect, near, far) => math.fromRows([
    1 / (halfHeight * aspect), 0, 0, 0,
    0, 1 / halfHeight, 0, 0,
    0, 0, -2 / (far - near), -(far + near) / (far - near),
    0, 0, 0, 1,
  ]);

  // NDC -> pixel coordinates. NDC +y is up; pixel rows grow downward.
  math.ndcToPixel = (ndc, width, height) => [((ndc[0] + 1) / 2) * width, ((1 - ndc[1]) / 2) * height];

  // ---------------------------------------------------------------------------
  // 3×3 matrices for directions and normals, also column-major: m[col * 3 + row].
  // ---------------------------------------------------------------------------
  math.upper3x3 = (m) => [m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]];

  // Normals must stay perpendicular to their surface, which needs the inverse transpose
  // of the 3×3 part. For a 3×3 matrix that is the cofactor matrix divided by the determinant.
  math.normalMatrix = (m4) => {
    const [a, b, c, d, e, f, g, h, i] = math.upper3x3(m4); // columns (a b c), (d e f), (g h i)
    const cofactors = [
      e * i - f * h, f * g - d * i, d * h - e * g,
      c * h - b * i, a * i - c * g, b * g - a * h,
      b * f - c * e, c * d - a * f, a * e - b * d,
    ];
    const det = a * cofactors[0] + d * cofactors[3] + g * cofactors[6];
    return cofactors.map((value) => value / det);
  };

  cg.math = math;
})((window.cg = window.cg || {}));
