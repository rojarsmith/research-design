// Shared meshes for Lab 07 and later. Every mesh uses Lab 06's vertex layout:
//
//   | x  y  z | nx ny nz | u  v |      8 floats = 32 bytes per vertex
//     0         12         24          <- byte offset of each attribute
//
// Triangles are counter-clockwise seen from outside, so they are front faces under
// gl.frontFace(gl.CCW), and indices are 16-bit. Lab 06 builds its sphere and cube inline in
// its own app.js so you can read them there first; these are the same constructions with the
// sizes as parameters.
//
// A classic script on window.cg, like math.js, so index.html keeps working over file://.
(function (cg) {
  const mesh = {};
  mesh.FLOATS_PER_VERTEX = 8;
  mesh.STRIDE = 32;

  // A UV sphere around the origin. On a sphere the unit normal is the position divided by
  // the radius. u runs once around, v from the north pole (0) to the south pole (1).
  mesh.sphere = (radius = 1, stacks = 24, slices = 48) => {
    const vertices = [];
    const indices = [];
    for (let i = 0; i <= stacks; i++) {
      const theta = (Math.PI * i) / stacks;
      for (let j = 0; j <= slices; j++) {
        const phi = (2 * Math.PI * j) / slices;
        const n = [Math.sin(theta) * Math.cos(phi), Math.cos(theta), -Math.sin(theta) * Math.sin(phi)];
        vertices.push(n[0] * radius, n[1] * radius, n[2] * radius, ...n, j / slices, i / stacks);
      }
    }
    for (let i = 0; i < stacks; i++) {
      for (let j = 0; j < slices; j++) {
        const a = i * (slices + 1) + j;
        const b = a + slices + 1;
        indices.push(a, b, b + 1, a, b + 1, a + 1);
      }
    }
    return { vertices: new Float32Array(vertices), indices: new Uint16Array(indices) };
  };

  // An axis-aligned box centered on the origin with half extents hx, hy, hz. Four vertices
  // per face, so every face has its own flat normal and a full 0..1 UV square
  // (v = 0 is the first texture row, as in Lab 06).
  mesh.box = (hx, hy, hz) => {
    const corners = [
      [-hx, -hy, -hz], [hx, -hy, -hz], [hx, hy, -hz], [-hx, hy, -hz],
      [-hx, -hy, hz], [hx, -hy, hz], [hx, hy, hz], [-hx, hy, hz],
    ];
    const faces = [[4, 5, 6, 7], [1, 0, 3, 2], [5, 1, 2, 6], [0, 4, 7, 3], [7, 6, 2, 3], [0, 1, 5, 4]];
    const normals = [[0, 0, 1], [0, 0, -1], [1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0]];
    const uvs = [[0, 1], [1, 1], [1, 0], [0, 0]];
    const vertices = [];
    const indices = [];
    faces.forEach((face, f) => {
      face.forEach((corner, k) => vertices.push(...corners[corner], ...normals[f], ...uvs[k]));
      indices.push(f * 4, f * 4 + 1, f * 4 + 2, f * 4, f * 4 + 2, f * 4 + 3);
    });
    return { vertices: new Float32Array(vertices), indices: new Uint16Array(indices) };
  };

  // A square in the plane y = 0 facing +y, from −half to +half in x and z. It has no back
  // side: seen from below it is a back face and gets culled. UVs repeat `repeat` times;
  // v = 0 is at −z, the far edge for a camera on the +z side.
  mesh.plane = (half, repeat = 1) => {
    const vertices = new Float32Array([
      -half, 0, half, 0, 1, 0, 0, repeat,
      half, 0, half, 0, 1, 0, repeat, repeat,
      half, 0, -half, 0, 1, 0, repeat, 0,
      -half, 0, -half, 0, 1, 0, 0, 0,
    ]);
    return { vertices, indices: new Uint16Array([0, 1, 2, 0, 2, 3]) };
  };

  cg.mesh = mesh;
})((window.cg = window.cg || {}));
