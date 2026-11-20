**English** | [繁體中文](README.zh-TW.md)

# AI-assisted coding round practice kit for VS Code on Windows 11

This kit turns sections 6.6 and 6.7 of the [research report](../beyond-dsa.md) into a practice loop. The full guide is in [README.zh-TW.md](README.zh-TW.md); this page is a short overview.

It follows the three-phase structure reported for Meta's AI-enabled round. In a multi-file codebase you did not write, you:

1. fix a bug,
2. implement a feature,
3. optimize it against much larger data.

- **6 original scenarios**, written for this project rather than taken from any company's question bank. Each is a small C++ project with a README, 4 to 6 modules, a command-line app, existing tests, and three phases of tasks and tests:
  - `build-planner`: a CI build-manifest tool
  - `log-alerts`: an on-call log tool
  - `room-booking`: a meeting-room tool
  - `api-gateway`: the routing layer of an API gateway
  - `netlist-check`: a circuit-board connectivity checker
  - `team-match`: an internship team-matching tool

  The phase 2 and 3 work needs different data structures in each: a heap-driven simulation, sliding windows, ordered sets, a trie, union-find, and stable matching with per-team heaps.
- **The `drill` CLI**, written in C++. It reveals one phase at a time and times each phase. It only closes a phase when that phase's tests pass, unless you pass `--skip`, and only records a session once you have written the review.
- **You install three things:** VS Code, the C/C++ extension and WinLibs GCC, the same as [cpp-practice](../cpp-practice/README.md). You also need the AI assistant you want to practice with. Git is used to show your diff if you have it.

## How it enforces the method

- **Each session gets its own folder and git repository,** opened in a new window. The AI assistant sees only that codebase. Later phases and the debrief stay outside the folder.
- **Three modes:**
  - `chat`: the AI answers in chat only and you type every change, like Meta's round
  - `open`: any tool is allowed, including agents, like Canva and HackerRank Chakra
  - `none`: no AI, to measure your unaided baseline

  `AGENTS.md`, `CLAUDE.md` and `.github/copilot-instructions.md` tell the assistant the rules for the mode you picked.
- **Phase 2 asks you to state your data structures and complexity before you prompt.**
- **Phase 3 needs measurement, not guessing.** Each large test stresses a different shape of data. In four scenarios the slow part is in code you did not write; in the other two, existing code is slow only if you choose to reuse it.
- **A watchdog stops any large test at 3x its limit.** Each large test runs in its own process, so a stack overflow is reported instead of hiding the other tests.
- **`drill done` writes `REVIEW.md` and will not record the session until four fields are filled in.** Those fields cover the replay checks from the report: did you state the plan before prompting, did you run and verify each AI output, and how fast did you catch the AI's mistakes. Only then does it show the scenario's debrief.

## A session

| Step | VS Code | Terminal |
| --- | --- | --- |
| Start | In the kit window, run task `drill: 開始新的練習` and pick a scenario and mode | `.\build\drill.exe start build-planner --mode chat` |
| Test | Ctrl+Shift+B in the session window | `drill test` |
| Next phase | Task `drill: 下一階段` (or the skip task when time is up) | `drill next`, `drill next --skip` |
| Time per phase | Task `drill: 狀態` | `drill status` |
| Finish | Task `drill: 結束並複盤`, twice: once to stop the clock and create `REVIEW.md`, again after you fill it in | `drill done` |

Suggested time: phase 1 about 15 minutes including reading, phase 2 about 25, phase 3 about 15, then 5 to wrap up and discuss.

## Verified

The kit was verified on Windows 11 with WinLibs g++ 16.2.0. Reference solutions are not shipped.

- In each scenario's starting codebase, the hidden bug fails 1 or 2 phase 1 tests, and every phase 2 test fails.
- Reference solutions pass every test. They run the large tests in 0.08 to 0.46 seconds, against limits of 1 to 1.5 seconds.
- Expected values for phase 2 were cross-checked with two or three independent implementations per scenario. For the three newer scenarios, the phase 3 expected values were also reproduced by an independent implementation.
- Slower approaches fail at least one large test each, by timeout, stack overflow or 1.3x to 2.3x the limit.
- The full `drill` loop was run end to end, including incremental rebuilds and the review check: once when the kit was built, and once for each of the three newer scenarios, in `chat`, `open` and `none` mode.

Task statements are in English, as in a real interview. The tool's messages and the debriefs are in Traditional Chinese.
