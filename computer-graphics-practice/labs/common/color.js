// Shared color helpers: the sRGB transfer function (IEC 61966-2-1) and 8-bit conversions.
// A classic script on the window.cg namespace, like math.js, so it works over file://.
//
// The course rule from Lab 02 on: colors are stored and displayed sRGB-encoded, but every
// mix (interpolation, lighting, blending, filtering) happens in linear light. Decode on the
// way in, do the math, encode once on the way out. See ../../CONVENTIONS.md.
(function (cg) {
  const color = {};

  // Encoded sRGB value in [0, 1] -> linear light in [0, 1].
  color.srgbToLinear = (c) => (c <= 0.04045 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4);
  // Linear light in [0, 1] -> encoded sRGB value in [0, 1].
  color.linearToSrgb = (c) => (c <= 0.0031308 ? c * 12.92 : 1.055 * c ** (1 / 2.4) - 0.055);

  const clamp01 = (v) => Math.min(1, Math.max(0, v));
  // An 8-bit sRGB channel (0–255, may be fractional) -> linear light.
  color.byteToLinear = (b) => color.srgbToLinear(clamp01(b / 255));
  // Linear light -> the nearest 8-bit sRGB channel.
  color.linearToByte = (v) => Math.round(color.linearToSrgb(clamp01(v)) * 255);

  cg.color = color;
})((window.cg = window.cg || {}));
