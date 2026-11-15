**English** | [繁體中文](README.zh-TW.md)

# C++ practice project for VS Code on Windows 11

This project turns the recommendations in the [research report](../README.md) into a practice loop you can follow every day. The full guide is in [README.zh-TW.md](README.zh-TW.md); this page is a short overview.

- **74 problems over 10 weeks:** every one of the 30 patterns in the report's trigger table has at least one problem, and 24 have two or more. The frequently tested patterns (two pointers, sliding window, binary search, stacks, linked lists, trees, backtracking, graphs, dynamic programming) have 3 to 14. Four are HackerRank problems where you parse STDIN yourself, and 42 overlap with the Blind 75. Weeks 11 and 12 are left for reviews, interleaving, company-tagged problems and mock interviews.
- **Tests for every problem, written for this project:** examples and edge cases, and a stress test against a brute-force oracle that prints the smallest counterexample it finds. 60 of the 74 also have a timing test at the problem's maximum input size. When that size is too small to separate the complexities, the timing test uses a larger input and its name says so.
- **The `pp` CLI:** written in C++. It times each attempt, gives hints in layers, requires a review card, schedules spaced reviews, picks interleaved problems and reports weak patterns.
- **You install three things:** VS Code, the C/C++ extension and WinLibs GCC (`winget install --id BrechtSanders.WinLibs.POSIX.UCRT --exact`). No Python, CMake, make or PowerShell scripts.

## Learning points built into the workflow

1. **Re-solve from a blank file.** `pp start` archives your last attempt and hides it from Explorer and search.
2. **Think before you code.** Fill in the card fields first: constraints to complexity, brute force, bottleneck, trigger.
3. **Debug from the smallest counterexample instead of guessing.** Stress tests grow the input size, so the first failure is small.
4. **Time-box.** After 20 minutes `pp` reminds you to take a hint. `pp hint` reveals one layer at a time.
5. **Record results honestly.** Using a hint turns "pass" into "hint" automatically. Reviews are spaced 1 → 3 → 7 → 14 → 30 days; a hint halves the interval, and reading the solution means you redo the problem the next day.
6. **The card records the insight, not the code.** `pp done` refuses to record until the `[觸發]` (trigger) and `[關鍵洞見]` (key insight) fields are filled in.
7. **A timing failure means the complexity is wrong.** O(n²) solutions fail the timing tests, and a test still running after 10 s is stopped with a TIMEOUT that points to exponential time or an endless loop.
8. **Learn in blocks, then interleave.** After a pattern block, use `pp mix`, which hides the pattern.
9. **Fixed daily load, reviews first.** `pp today` gives up to 2 due reviews and 1 new problem, and no new problem when more than 3 reviews are due.
10. **Practice in exam-like conditions.** AI inline completion is off in this workspace, and the four HackerRank problems make you parse input yourself, one of them with several test cases in one input.

C++-specific points are in [docs/cpp-points.zh-TW.md](docs/cpp-points.zh-TW.md). Why this GCC build was chosen over the other WinLibs 16.2.0 downloads is in [docs/toolchain.zh-TW.md](docs/toolchain.zh-TW.md).

## Daily loop

| Step | VS Code | Terminal |
| --- | --- | --- |
| What to do today | Task `pp: 今日練習` | `.\build\pp.exe today` |
| Start or redo a problem | Open any file in the problem folder, run task `pp: 開始/重做這題` | `.\build\pp.exe start 739` |
| Test | Ctrl+Shift+B | `.\build\pp.exe test 739` |
| Debug | F5 | |
| Hint | Task `pp: 提示` | `.\build\pp.exe hint 739` |
| Record the result | Task `pp: 記錄結果` | `.\build\pp.exe done 739 pass` |

The F5 debug build (`-O0 -g -D_GLIBCXX_DEBUG`) skips the timing tests (names starting with `perf`) and shows them as `[ SKIP ]`. It does not enforce time limits anyway, and `_GLIBCXX_DEBUG` checks the whole heap on every `priority_queue` push and pop, so a large input can run for minutes. To step through a timing test in the debugger, set `args` to `["perf"]` in `.vscode/launch.json`.

Verified on Windows 11 with WinLibs g++ 16.2.0: all 74 stubs fail their tests, and reference solutions (not shipped) pass every test, including the timing tests. In the F5 debug build, the reference solutions to all 70 function-style problems pass with the timing tests skipped, each in under a second.
