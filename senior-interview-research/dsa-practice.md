**English** | [繁體中文](dsa-practice.zh-TW.md) · [Topic overview](README.md)

# Efficient HackerRank and LeetCode practice for senior engineers, without memorizing answers

Research date: 2026-11-15. The full report is in [dsa-practice.zh-TW.md](dsa-practice.zh-TW.md); this page is a short overview. It is the DSA part of [senior engineer interview preparation](README.md).

## Summary

- **Practice patterns, not answers.** Train the chain trigger → pattern → invariant, not problem → code. When you review, re-solve from a blank editor instead of rereading old code.
  - Dunlosky et al. (2013) rated only two study techniques high utility: practice testing and spaced practice.
  - Interleaving trains the skill interviews actually test: picking the right technique for an unfamiliar problem.
- **You need fewer problems than you think.** Finish one pattern-ordered list of 150 to 250 problems, mostly mediums. Grind 169 or NeetCode 150/250 both work. Then add hard problems and mock interviews.
  - interviewing.io found that the total number of problems solved correlates with interview percentile at only 0.27.
  - Candidates with five or more mock interviews were about twice as likely to pass Facebook's interview.
- **Use each platform for what it is good at.** Learn the content on LeetCode. Use HackerRank to rehearse the format of employer assessments: time limits, hidden test cases, parsing STDIN yourself, proctoring and AI-assist modes.
- **Senior candidates have to prepare for two kinds of round.**
  - Big Tech still runs DSA rounds. None of the interviewers interviewing.io surveyed said their company had dropped them. Questions are shifting toward variants, and lightly modified questions are enough to trip up candidates who memorized answers.
  - Meta, Canva, Shopify and Google have added rounds where AI tools are allowed or expected, and Google is bringing back in-person interviews.
  - So you need both unaided whiteboard problem solving and fluency working with AI in an unfamiliar codebase. Memorized answers fail in both.
  - DSA is roughly a third of senior prep. System design is the highest-leverage part.
- **The two reference repos are better for comparing after you solve a problem than as a curriculum.**
  - Java-aid/Hackerrank-Solutions has 292 Java files, mostly 30 Days of Code, Java language exercises and basic programming. It has only 2 Interview Preparation Kit problems. It is useful for HackerRank STDIN handling and Java idioms.
  - withaarzoo/LeetCode-Solutions has 511 problems in five languages, and 416 of them have a full write-up that includes an Intuition section. It is a daily-challenge log: it covers only 7 of the Blind 75, 179 of its problems are numbered 3000 or above, and it has no license.
- **Plan:** 8 to 12 weeks at 60 to 90 minutes of DSA a day.
  - Use the same per-problem procedure every time, with a 15 to 20 minute limit before taking a hint.
  - Keep a review card that records the insight, not the code.
  - Schedule reviews with SM-2 or FSRS.
  - In the second half, switch to interleaved and timed sets, and add mocks and practice for AI-assisted rounds.

## Contents of the full report

| Section | Content |
| --- | --- |
| 2 | Moved to the [topic overview](README.zh-TW.md): the 2025–2026 interview landscape, now combined with what each non-DSA round looks like |
| 3 | HackerRank vs LeetCode: tests, I/O, scoring, Proctor Mode, AI modes, prep resources |
| 4 | Method: learning-science basis, the per-problem procedure, a constraint-to-complexity table, a trigger-to-pattern table with LeetCode and HackerRank examples, a review card template, spacing and interleaving |
| 5 | Problem lists and how many problems to do |
| 6 | Moved to the topic overview: the time split and a 12-week plan that puts DSA next to the other rounds |
| 7 | Evaluation of the two repos and rules for using solution repos |
| 8 | HackerRank online assessment prep: fast-input templates in Java and Python, edge-case checklist, proctoring |
| 9 | Expanded in [Rounds beyond DSA](beyond-dsa.md); the mock interview advice moved to the overview's readiness criteria |
| 10 | The VS Code C++ practice project (see below) |
| 11–12 | Verification gaps and references |

## C++ practice project

[cpp-practice/](cpp-practice/README.md) puts the method into practice as a project you open in VS Code. It needs only VS Code, the C/C++ extension and WinLibs GCC.

- **Problems:** 74 over 10 weeks, leaving weeks 11 and 12 for reviews and mock interviews. Every one of the 30 patterns in the trigger table has at least one problem, 24 have two or more, and the frequently tested patterns have 3 to 14. Four are HackerRank STDIN problems, and 42 overlap with the Blind 75.
- **Tests:** examples and edge cases and a stress test against a brute-force oracle for each problem, plus a timing test for the 60 problems where the complexity matters.
- **The `pp` CLI** enforces the method:
  - re-solve from a blank file
  - time-boxed, layered hints
  - a required review card
  - spaced reviews at 1, 3, 7, 14 and 30 days
  - interleaving with the pattern hidden
  - a report of weak patterns
  - timed mock interviews (`pp mock`) that record nothing until you score a self-review against what interviewers assess, and a readiness report (`pp ready`) against the standard in sections 4.2 and 9

It was verified on Windows 11 with g++ 16.2.0:
- All 74 stubs fail their tests.
- Reference solutions pass every test.
- 14 solutions deliberately written with the wrong complexity fail only the timing tests.

Web sources were checked on 2026-11-15. Claims that rest only on secondary or vendor sources are marked unverified in the full report. The correctness of the repos' solutions was not checked problem by problem.
