**English overview** | [繁體中文完整研究](README.zh-TW.md)

# WebAssembly research

Research date: 2026-10-21 (Asia/Taipei).

This topic studies WebAssembly independently of any vendor: its execution model, C/C++ migration with Emscripten, JavaScript integration, deployment, performance, security, and WASI components outside browsers.

The complete report is in Traditional Chinese. Start with the [main report](README.zh-TW.md), then follow the four research notes and the annotated source index linked there. The requested Claire Chang article is summarized and checked against current toolchain documentation, with retrieval limitations recorded.

The main engineering conclusion is to evaluate a bounded computation or existing library first, measuring total startup and integration costs. Wasm execution alone does not move work off the UI thread. Browser deployment and WASI deployments require different host interfaces. These are research conclusions, not benchmark results from this machine.

The [Emscripten guide](research/02-emscripten-guide.zh-TW.md) includes an original, minimal C-to-browser recipe. No SDK was installed and no C/Wasm compilation was performed during this research. See the [source and verification record](research/05-sources.zh-TW.md).
