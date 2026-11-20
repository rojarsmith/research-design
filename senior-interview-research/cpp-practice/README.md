**English** | [繁體中文](README.zh-TW.md)

# C++ practice project for VS Code on Windows 11

This project turns the recommendations in the [research report](../dsa-practice.md) into a practice loop you can follow every day. The full guide is in [README.zh-TW.md](README.zh-TW.md); this page is a short overview.

- **74 problems over 10 weeks:** every one of the 30 patterns in the report's trigger table has at least one problem, and 24 have two or more. The frequently tested patterns (two pointers, sliding window, binary search, stacks, linked lists, trees, backtracking, graphs, dynamic programming) have 3 to 14. Four are HackerRank problems where you parse STDIN yourself, and 42 overlap with the Blind 75. Weeks 11 and 12 are left for reviews, interleaving, company-tagged problems and mock interviews (`pp mock`).
- **Tests for every problem, written for this project:** examples and edge cases, and a stress test against a brute-force oracle that prints the smallest counterexample it finds. 60 of the 74 also have a timing test at the problem's maximum input size. When that size is too small to separate the complexities, the timing test uses a larger input and its name says so.
- **The `pp` CLI:** written in C++. It times each attempt, gives hints in layers, requires a review card, schedules spaced reviews, picks interleaved problems and reports weak patterns. It also runs timed mock interviews and reports readiness against the research report's standard.
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
11. **Time your mock interviews and judge readiness over several of them.** The report's standard is that you solve most unseen Mediums in 20 to 25 minutes and pass several mocks in a row, not just one; it recommends at least 5 mocks, recorded and reviewed. `pp mock` runs one clock, reveals one problem at a time with the pattern and level hidden, gives no hints, and records nothing until you score a self-review. `pp ready` checks the standard.

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

Each week, also run `pp: 交錯抽題` (interleaving with the pattern hidden) and `pp: 弱項統計` (weak patterns). From week 5, add one mock interview a week; `pp today` reminds you when a week has passed without one.

The F5 debug build (`-O0 -g -D_GLIBCXX_DEBUG`) skips the timing tests (names starting with `perf`) and shows them as `[ SKIP ]`. It does not enforce time limits anyway, and `_GLIBCXX_DEBUG` checks the whole heap on every `priority_queue` push and pop, so a large input can run for minutes. To step through a timing test in the debugger, set `args` to `["perf"]` in `.vscode/launch.json`.

## Mock interviews

| Step | VS Code | Terminal |
| --- | --- | --- |
| Start, with a countdown in the panel | Task `pp: 模擬面試` | `.\build\pp.exe mock`; for a countdown, run `.\build\pp.exe mock clock` in a second terminal |
| Solve and test | Edit the solution file that opens, Ctrl+Shift+B | `.\build\pp.exe test <problem>` |
| Next problem | Task `pp: 模擬面試下一題` | `.\build\pp.exe mock next` |
| Stop the clock and open the review form | Task `pp: 結束模擬面試` | `.\build\pp.exe mock end` |
| Record, after filling in the form | Run `pp: 結束模擬面試` again | `.\build\pp.exe mock end` |
| Readiness | Task `pp: 準備度` | `.\build\pp.exe ready` |

`.\build\pp.exe mock status` shows where the session stands; running `pp: 模擬面試` again does the same and resumes the countdown.

**How a session runs.**

- The default is 2 problems in 45 minutes on one clock. `pp mock 3 60` changes both (1 to 4 problems, 5 to 180 minutes).
- Problems are revealed one at a time, the way an interviewer gives the next question. Each problem's time runs from its reveal. The pattern and the level are hidden, as with `pp mix`.
- Selection prefers problems you have never attempted in a pattern you have already practiced (problems added with `pp new` for week 11 onward count too), then problems untouched for at least a week, older ones more likely. Patterns differ within a session. The first problem is a Medium; the others get a level that fits the time per problem: Easy only below 15 minutes each, Hard possible only at 30 minutes or more. Problems come easiest first.
- Each problem starts from a blank file, with the previous attempt archived as `pp start` does.
- There are no hints: `pp hint` is disabled. During a mock, `pp start` on another problem and `pp done` on the mock's problems are refused.
- Test as usual. A problem's result is its last test run before time runs out. A run started before the deadline counts even if compiling ends after it. Once you move to the next problem, the previous result is fixed.
- The practice-mode rule "after 20 minutes, take a hint" becomes checkpoints scaled to the session. They appear in `pp test`, `pp mock status` and the countdown:
  - A problem has used its share of the time without passing (22 minutes for 2 problems in 45): write a correct brute force for partial credit and say where you are stuck, or move on.
  - 5 minutes left: stop making big changes, run an extreme test, and say what is still missing (report section 8).
  - Time is up: later passes do not count; run `pp mock end`.

**The review form.** The first `pp mock end` stops the clock and writes `progress/mocks/<session>.txt`. It holds the five things interviewers score. After each tag, write a score from 1 to 4 and one sentence of evidence, for example `[測試] 2 只跑了範例`:

| Tag | What it covers |
| --- | --- |
| `[澄清]` (clarify) | Restated the problem and asked about input ranges and edge cases before coding |
| `[思路]` (approach) | Stated the brute force and its complexity, then the optimization and why this pattern, before coding |
| `[正確]` (correctness) | Code was close to right the first time and clearly structured, not fixed by rerunning tests until they passed |
| `[測試]` (testing) | Wrote own edge cases and traced the code by hand, finding bugs before running the tests |
| `[溝通]` (communication) | Talked while coding and said where and why when stuck |

- Scores: 4 is strong, 3 is acceptable, 2 has gaps an interviewer would notice, 1 is missing. Full-width digits work too.
- `[下次改進]` (next time) is required: the one thing to change in the next mock. `pp mock` shows it again when the next session starts.
- `[夥伴或錄影]` (partner or recording) is optional: who played the interviewer, or where the recording is.
- As with `pp done`, nothing is recorded until the required fields are filled in.
- When you practice alone, still talk out loud. Record video or audio if you can, and watch it before scoring.

**Records and the review schedule.** Run `pp mock end` again after filling in the form:

- `progress/mocks.tsv` gets one row per problem: result, minutes and rubric scores.
- A session **passes** when every problem passed its tests in time and no rubric item scored 1.
- Results also go into the spaced-review schedule in `reviews.tsv`, with `mock` in a seventh column. A passed problem moves up the ladder like `pp done pass`.
- A failed problem is recorded as `fail` and is **due the same day**, so `pp today` and `pp due` list it. Work it through untimed first (hints are allowed then), then record it with `pp done`.
- `pp mock abort` drops an interrupted session without recording it or touching the schedule.

**Readiness.** `pp ready` checks three things against the report; the last line of `pp stats` summarizes them.

| Check | Threshold | Source in the report |
| --- | --- | --- |
| Count | At least 5 mocks | Section 1: candidates with 5 or more mocks were about twice as likely to pass Facebook's interview (interviewing.io). Section 6.2 (weeks 9 to 12) and section 9 both ask for at least 5. |
| Consistency | The last 3 mocks passed in a row | Section 4.2: several consecutive mocks passed, not just one; only about 20% to 25% of people perform consistently. Section 9: judge readiness over several mocks. The report gives no number; this project uses 3. |
| Medium speed | At least 70% of the Mediums in the last 5 mocks passed within 25 minutes | Section 4.2: most unseen Mediums solved in 20 to 25 minutes (NeetCode's benchmark). "Most" is read as 70%, against the upper end of the range. |

`pp ready` also lists:

- The last 5 sessions.
- The average of each rubric item, and the weakest one below 3.
- How many sessions had a partner or a recording.
- How many of the Mediums were unseen before the session. Problems you have solved before go faster, so the speed check is only meaningful with unseen ones. In weeks 11 and 12, add company-tagged or new-list problems with `pp new` and use them in mocks.

**Testing `pp` itself.** Set `PP_MOCK_MINUTE` (1 to 60) to make a session minute that many seconds. For example, with `PP_MOCK_MINUTE=2` a 45-minute session ends in 90 seconds. The value is stored in the session when `pp mock` starts, so later commands do not need it. Do not use it for practice.

## Verification

Verified on Windows 11 with WinLibs g++ 16.2.0: all 74 stubs fail their tests, and reference solutions (not shipped) pass every test, including the timing tests. In the F5 debug build, the reference solutions to all 70 function-style problems pass with the timing tests skipped, each in under a second.

`pp mock` and `pp ready` were run end to end in a terminal in the same environment, with `PP_MOCK_MINUTE` shortening the clock:

- Passing and failing sessions, including one with a HackerRank problem.
- Refusal to record while the review form is incomplete or a score is outside 1 to 4.
- Tests after the deadline not counting, and the checkpoints of the countdown (`pp mock clock`) run in a terminal.
- Failed problems falling due the same day.
- `pp ready` reporting the standard met after 5 sessions with 3 consecutive passes.
- On progress files made by the previous version, `today`, `start`, `test`, `hint`, `done`, `debug`, `due`, `list` and `stats` print the same output as before, except one added mock-interview reminder line in `pp today`. The previous `pp` also reads the files after mock results are added.

The VS Code tasks and the countdown panel were not exercised inside VS Code.
