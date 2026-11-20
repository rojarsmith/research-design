**English** | [繁體中文](beyond-dsa.zh-TW.md) · [Topic overview](README.md)

# Preparing for the non-algorithm interview rounds as a senior engineer in 2026, without memorizing answers

Research date: 2026-11-15. The full report is in [beyond-dsa.zh-TW.md](beyond-dsa.zh-TW.md); this page is a short overview.

This report is the non-DSA part of [senior engineer interview preparation](README.md) and follows the [DSA practice report](dsa-practice.md). The 2026 interview landscape (section 2) and the time split and 12-week plan (section 7) moved to the topic overview, where they are combined with the DSA material.

## Summary

- **The rule against memorizing answers applies to every round. What you practice changes from round to round.**
  - System design: practice the chain requirement trigger → design pattern → tradeoff, not a reference diagram for each question.
  - Behavioral: learn each story's facts, numbers and decision points, not a script.
  - LLD/OOD: practice requirements → entities → responsibilities → extension points.
  - AI-assisted coding: you make the decisions, the AI carries them out, and you verify every step.
  - Interviewers tell memorization from understanding by changing constraints and probing tradeoffs, so practice with deliberate variants.
- **System design is the main leveling signal.**
  - Use one framework, based on Hello Interview's:
    - requirements, about 5 minutes
    - core entities, about 2 minutes
    - API, about 5 minutes
    - optional data flow, about 5 minutes
    - high-level design, 10 to 15 minutes
    - deep dives, about 10 minutes
  - Do capacity estimation only when the number changes the design. The [estimation-practice](estimation-practice/README.md) kit drills it with 42 timed drills and 3 chains.
  - Senior candidates should find what makes the system hard and go deep in about two places. Staff candidates should lead almost the whole session, make decisions instead of listing options, and cut complexity that isn't needed.
  - Hello Interview names over-engineering as one of the most common reasons for down-leveling.
  - The report has a trigger → pattern → tradeoff table and a 14-problem set that covers every pattern. You don't need to work through all 32 common questions.
- **Behavioral: prepare 6 to 8 stories drawn from 3 to 5 projects.**
  - Map them in a matrix against the target company's signals, such as Meta's eight focus areas or Amazon's 16 Leadership Principles. Have at least two stories for each signal.
  - For senior roles, scope decides the level: whether a story affected your team, the org, or several teams. A story that is fine for a junior candidate can count against a senior one.
  - Say "I", not "we". Keep notes on facts and decisions rather than a script, and practice with randomly drawn questions and follow-up probes.
- **LLD/OOD is not universal, so ask the recruiter first.**
  - Hello Interview's format is about 35 minutes in five steps.
  - It is graded on how you split responsibilities, where state lives and how easily the design extends, not on how many patterns you can name.
  - The report maps common problems to what they test, such as state machines, strategy, observer, booking with concurrency and ledgers.
  - The [lld-practice/](lld-practice/README.md) kit turns this into a practice loop: 4 original problems, a design note before any code, 35- or 90-minute timing, "what if" variants revealed one at a time, and a count of the existing classes, functions and lines each variant forced you to change.
- **AI-assisted coding rounds are now real rounds at major companies.**
  - Meta (since October 2025): 60 minutes in CoderPad, fixing a bug, then building a feature, then optimizing in a multi-file codebase. Most details come from candidate reports.
  - Canva (since June 2025): replaced CS Fundamentals screening for backend and frontend roles.
  - Google (piloting in the second half of 2026): junior and mid-level roles on select US teams read, debug and optimize existing code with Gemini.
  - HackerRank Chakra (generally available on 2026-10-05): an AI interviewer that grades "AI fluency" from your activity in the IDE.
  - They all grade the same things: breaking down the problem, staying in control of the AI, verifying its output, and communicating. Handing everything to the AI loses points, and so does never using it. Chakra gives no AI-fluency grade when there are too few AI interactions.
- **Time:** system design 35–40%, DSA 25–35%, AI-assisted 10–15%, behavioral and LLD 10–15%. The [topic overview](README.zh-TW.md) gives a 12-week plan at 18 to 20 hours a week with DSA and the other rounds in one table, a short version for when only 3 to 4 weeks remain, and readiness criteria for each round. The LLD column of the 12-week plan can use [lld-practice](lld-practice/README.md): `parking-garage` in weeks 3–4, `expense-approval` and `seat-holds` in weeks 5–6, `auction-house` in weeks 7–8, redoing one in `machine` mode if the target company runs machine coding.

## Contents of the full report

| Section | Content |
| --- | --- |
| 1 | Conclusions |
| 2 | Moved to the topic overview: what each round looks like in 2026 and how much it weighs for senior roles |
| 3 | System design: expectations by level and why candidates get down-leveled, a reusable framework, capacity estimation (powers of two, latency, availability, a worked example), a trigger-to-pattern table, a 14-problem set, practice without memorizing (review card, variant drills, mocks), Meta's Product Architecture vs System Design, resources; a capacity-estimation drill kit (see below) |
| 4 | Behavioral and leadership: Meta's eight focus areas and scope by level, Amazon's 16 Leadership Principles, Google's reported change, STAR/STAR(R)/CARL, steps to prepare 6 to 8 stories, story slots and a matrix, follow-up probes and common mistakes |
| 5 | LLD/OOD: who asks it and in what format, a five-step framework, grading by level, problems mapped to what they test, patterns and principles, practice |
| 6 | AI-assisted coding: comparison of Meta, Canva, Google and HackerRank Chakra; a shared workflow; anti-patterns; a weekly three-phase drill and the practice kit (see below) |
| 7 | Moved to the topic overview: time split in hours per week, the 12-week plan, a 3-to-4-week short version, readiness criteria |
| 8–9 | Verification gaps and references |

## Practice kit for AI-assisted rounds

[ai-round-practice/](ai-round-practice/README.md) is a VS Code workspace for the weekly drill. It needs only VS Code, the C/C++ extension and WinLibs GCC, the same as the DSA report's [cpp-practice](cpp-practice/README.md).

- **Scenarios:** 6 original multi-file C++ codebases. Each runs in three phases: fix a bug, implement a feature, then optimize against large generated data.
- **The `drill` CLI:**
  - reveals one phase at a time and times each phase
  - opens each session in its own folder and window, so the AI assistant sees only that codebase
  - supports `chat`, `open` and `none` modes
  - records a session only after you fill in a review that covers the report's replay checks
- **Verified** on Windows 11 with g++ 16.2.0:
  - Reference solutions use under a third of each time limit.
  - Every slower approach that was tried fails at least one large test.

## Practice kit for capacity estimation

[estimation-practice/](estimation-practice/README.md) turns section 3.3 into timed drills and needs only WinLibs GCC.

- 42 original drills and 3 chains that walk one system's numbers from users to QPS, storage, bandwidth and servers.
- The `est` CLI asks for assumptions before answers and rejects answers without a valid unit. It counts an answer within 3x of the reference as a hit and records an attempt only after you name the step where you diverged. It schedules missed drills for review and reports systematic over- or under-estimation by quantity type.
- Verified on Windows 11 with g++ 16.2.0: an independent Python script re-derived every reference answer from the givens, and all agree with exact arithmetic within 1.25x.

## Practice kit for LLD/OOD rounds

[lld-practice/](lld-practice/README.md) turns section 5.6 into a practice loop and needs only VS Code, the C/C++ extension and WinLibs GCC.

- 4 original problems, each on a different row of the section 5.4 table: a parking garage (strategy), expense approval (state machine), an auction house (observer) and seat holds (booking with concurrency).
- The `ood` CLI locks building and testing until a short design note is written, times a 35-minute design round or a 90-minute machine-coding round, reveals "what if" variants one at a time, and after each variant counts the existing classes, functions and lines you had to change.
- Verified on Windows 11 with g++ 16.2.0: the starting skeletons fail every test, a flexible and a deliberately rigid reference design (not shipped) both pass every test, and the rigid one changed more existing functions in 9 of 11 variants and more existing lines in 10 of 11.

Web sources were checked on 2026-11-15, and GitHub star counts come from the GitHub API on that date. Claims that rest only on secondary or vendor sources are marked unverified in the full report. That includes most details of Meta's AI-enabled round, the scope of Google's pilot beyond what Google confirmed, HackerRank's own figures for Chakra, and which companies run LLD rounds.
