**English** | [繁體中文](README.zh-TW.md)

# Senior engineer interview preparation: DSA and the other rounds, without memorizing answers

Research date: 2026-11-15. On 2026-11-20 two earlier studies, HackerRank and LeetCode practice and the interview rounds beyond DSA, were merged into this topic. The parts they shared, the 2025–2026 interview landscape, the time split and the 12-week plan, are in the [Traditional Chinese overview](README.zh-TW.md). The method for each round stays in the two reports.

| Document | Content |
| --- | --- |
| [DSA practice](dsa-practice.md) ([full report](dsa-practice.zh-TW.md)) | Learning-science basis, a per-problem procedure, a trigger-to-pattern table, problem lists and counts, HackerRank vs LeetCode, two reference repos, HackerRank online assessment prep |
| [Rounds beyond DSA](beyond-dsa.md) ([full report](beyond-dsa.zh-TW.md)) | System design framework, capacity estimation, a trigger-to-pattern table and a 14-problem set; 6 to 8 STAR stories mapped to Meta and Amazon signals; LLD/OOD; AI-assisted rounds at Meta, Canva, Google and HackerRank Chakra |
| [cpp-practice/](cpp-practice/README.md) | A 74-problem VS Code C++ project; the `pp` CLI includes timed mock interviews and a readiness report |
| [ai-round-practice/](ai-round-practice/README.md) | 6 original three-phase AI-assisted round scenarios; the `drill` CLI reveals phases, times them and requires a review |
| [lld-practice/](lld-practice/README.md) | 4 original LLD/OOD problems; the `ood` CLI reveals one "what if" at a time and counts the classes, functions and lines you had to change |
| [estimation-practice/](estimation-practice/README.md) | 42 capacity-estimation drills and 3 chains; the `est` CLI checks units, scores by order of magnitude and reports bias |

## Summary

- **Practice the reasoning, not the answer, in every round.**
  - DSA: trigger → pattern → invariant, re-solved from a blank editor.
  - System design: requirement trigger → design pattern → tradeoff.
  - Behavioral: each story's facts, numbers and decision points, not a script.
  - LLD/OOD: requirements → entities → responsibilities → extension points.
  - AI-assisted coding: you decide, the AI carries it out, and you verify every step.
- **Senior candidates need both unaided problem solving and fluency with AI in an unfamiliar codebase.** Big Tech still runs DSA rounds, now with more variants. Meta, Canva and Google have added rounds where AI is allowed or expected, and HackerRank launched the Chakra AI interviewer.
- **System design is the main leveling signal.** The suggested split is system design 35–40%, DSA 25–35%, AI-assisted coding 10–15%, behavioral and LLD 10–15%.
- **One plan for all rounds.** The overview turns that split into 18 to 20 hours a week over 12 weeks, with DSA and the other rounds in one table, a short version for when only 3 to 4 weeks remain, and readiness criteria for each round.

Both reports keep their original section numbers. Sections 2, 6 and 9 of the DSA report and sections 2 and 7 of the rounds report moved to the overview or the other report, and now hold pointers. References such as "section 6.2" in the kits and CLIs therefore still lead to the right place.

The merge moved and combined existing content only; web sources were not rechecked. Claims that rest only on secondary or vendor sources are marked unverified in the full reports.
