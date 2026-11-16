**English** | [繁體中文](README.zh-TW.md)

# LLD/OOD and machine-coding practice kit for VS Code on Windows 11

This kit turns section 5 of the [research report](../README.md) into a practice loop: the five-step framework in 5.2, what is graded in 5.3, and the practice method in 5.6. The full guide is in [README.zh-TW.md](README.zh-TW.md); this page is a short overview.

- **4 original problems**, written for this project rather than taken from any question bank. Each covers a different row of the report's problem table (5.4). Each has a core requirement and 2 or 3 "what if" variants, revealed one at a time, each with its own tests:

  | Problem | Row in 5.4 | Variants |
  | --- | --- | --- |
  | `parking-garage` | swappable algorithms and pricing (Strategy) | pricing plans, an allocation policy, EV charging |
  | `expense-approval` | state machine | a finance-review state, delegation, notifications |
  | `auction-house` | event notification (Observer) | notification preferences, anti-sniping, buy-it-now |
  | `seat-holds` | booking and concurrency | a per-customer limit, a per-seat waitlist |

- **The `ood` CLI**, written in C++.
  - It locks building and testing until you have written a short design note.
  - It times the design step, the core and each variant: 35 minutes in `design` mode (the interview format), 90 in `machine` mode (Flipkart-style machine coding).
  - It reveals one variant at a time and asks you to predict which classes will change.
  - **After each variant it reports how many existing classes, existing functions and lines you had to change.** Section 5.6 uses that number as the signal of how well responsibilities were split. Line totals come from `git diff` when git is available, otherwise from the tool's own snapshot diff.
- **You install three things:** VS Code, the C/C++ extension and WinLibs GCC, the same as [cpp-practice](../../coding-interview-practice-research/cpp-practice/README.md) and [ai-round-practice](../ai-round-practice/README.md).

## How it enforces the method

- **Design first.** `ood start` creates `DESIGN.md` with five fields: requirements, out of scope, entities, classes and responsibilities, extension points. `ood design` refuses to stop the design clock until they are filled in, and `ood test` refuses to build before that.
- **Tests check behaviour through one small public interface** (for example `Garage` in `src/garage.hpp`), never your class names, so different designs pass. When a variant adds a method you have not written yet, its tests fail with the expected signature instead of breaking the build.
- **Variants accumulate.** Earlier tests stay, and `ood next` needs all of them to pass (or `--skip`).
- **Prediction before change.** Each revealed variant adds a prediction field to `DESIGN.md`; tests stay locked until it is written, and the report prints it next to the classes you actually changed.
- **The change report** separates existing code from new code. It counts the existing classes and functions you modified and the code lines changed inside them, plus new classes and total lines. Adding classes is how a good design extends, so the "existing" numbers are the signal.
- **Concurrency tests are deterministic in what they check.** They assert invariants that hold for every interleaving, such as no seat in two live holds, all-or-nothing holds and unique ids. All threads start from one latch, rounds repeat 30 to 100 times, and a 20-second watchdog reports deadlocks.
- **`ood done` writes `REVIEW.md`** with the times and change counts filled in, and records the session only once four fields are written. Then it shows the problem's debrief: design choices, patterns that fit, measured change counts for a flexible and a rigid reference design, and strong versus weak designs at junior, mid-level and senior level (section 5.3).
- **No AI.** Inline completion is off in session folders, and `AGENTS.md`, `CLAUDE.md` and `.github/copilot-instructions.md` ask assistants not to help.

## A session

| Step | VS Code | Terminal |
| --- | --- | --- |
| Start | In the kit window, run task `ood: 開始新的練習` and pick a problem and mode | `.\build\ood.exe start parking-garage --mode design` |
| Design done | Task `ood: 設計完成，開始寫程式` | `ood design` |
| Test | Ctrl+Shift+B in the session window | `ood test` |
| Changes so far | Task `ood: 目前改了多少` | `ood diff` |
| Next variant | Task `ood: 下一個變體` | `ood next`, `ood next --skip` |
| Finish | Task `ood: 結束並複盤`, twice | `ood done` |

## Verified

Verified on Windows 11 with WinLibs g++ 16.2.0. Reference solutions are not shipped.

- Each problem's blank skeleton fails every test of every phase: 0/22, 0/30, 0/25 and 0/19.
- Each problem has two reference designs, and both pass every test: one with responsibilities split for change, and one deliberately rigid (rules repeated in every method, types checked by string, expiry checked separately in each method). Each version fails all of the next variant's new tests.
- Change counts (existing classes / existing functions / code lines changed inside existing classes and functions), flexible vs rigid:
  - parking-garage: 1/0/+1−0 vs 1/1/+12−0; 1/0/+1−0 vs 1/1/+13−6; 4/3/+11−4 vs 3/5/+39−26
  - expense-approval: 2/2/+6−1 vs 2/4/+26−11; 1/2/+17−7 vs 2/3/+19−6; 2/2/+6−0 vs 1/5/+17−0
  - auction-house: 2/1/+15−1 vs 1/2/+12−7; 3/2/+6−1 vs 1/1/+5−0; 2/0/+5−0 vs 2/1/+5−13
  - seat-holds: 1/1/+4−0 vs 1/1/+8−0; 1/1/+7−1 vs 2/5/+18−2

  The rigid design changed more existing functions in 9 of 11 variants and more existing lines in 10. The exception is auction-house variant 2, where both designs keep the bid rule in one function. The class count alone does not separate the designs reliably, which is why the report shows all three numbers.
- Line totals from `git diff --minimal` equal the tool's own snapshot diff for the same code. One full session ran with git removed from PATH and used the snapshot diff.
- Concurrency:
  - Both correct seat-holds designs: 20 of 20 runs passed (0.6 to 3.8 s per run) on the five thread tests.
  - Three deliberately broken versions, 10 runs each: no locking failed all three core thread tests in 10 of 10 runs; per-seat locks taken in request order were stopped as a deadlock every time; a limit checked in a separate critical section failed every time.
- The full `ood` loop ran twice per problem: `design` mode with git, and `machine` mode without git, with `OOD_MINUTE` shortening the clock. Every gate refused as intended: design note incomplete, prediction missing, review incomplete.
- Not verified:
  - VS Code tasks and F5 debugging inside VS Code; the tool never opens windows, and every command the tasks call was run in a terminal.
  - Linux and macOS.
  - The change analysis on designs other than the eight references; it is a heuristic.

Problem statements and tests are in English, as in a real interview. The tool's messages and the debriefs are in Traditional Chinese.
