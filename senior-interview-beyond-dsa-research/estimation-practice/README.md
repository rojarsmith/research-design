**English** | [繁體中文](README.zh-TW.md)

# Capacity estimation practice kit (Windows 11, needs only g++)

This kit turns the capacity-estimation principles in section 3.3 of the [research report](../README.md) into deliberate practice: write your assumptions first, give every number a unit, count reads and writes separately, take the peak as 2 to 3 times the average, use round numbers, and aim for the right order of magnitude.

- **42 original single drills** and **3 chain drills** of 5 steps each. The scenarios and numbers are practice assumptions written for this project. They are not real data from any company and are not copied from a question bank or answer key.
- **The `est` CLI:** one C++ file, built the same way as `pp` in [cpp-practice](../../coding-interview-practice-research/cpp-practice/README.md) and `drill` in [ai-round-practice](../ai-round-practice/README.md). Its messages are in Traditional Chinese.
- **You only need WinLibs GCC.** VS Code is optional: if you use it, the tasks are set up; if not, run `est` in a terminal.

## 1. Read this first: how the kit enforces the method

| # | Report principle (sections 3.2 and 3.3) | What the kit does |
| --- | --- | --- |
| 1 | **Write down assumptions** | Before answering you must write at least one assumption that contains a number. They are saved to `progress/journal.md`. |
| 2 | **Give every number a unit** | An answer with no unit, the wrong kind of unit (for example GB when the question asks for bandwidth), or a case that hides whether you mean bits or bytes (`1.2 tb`) is sent back with the reason. The number of rejections is recorded. |
| 3 | **Aim for the order of magnitude, not precision** | An answer within 3x of the reference is a hit (10x for drills where you assume a key size yourself). Section 4 gives the reasons. |
| 4 | **Count read and write QPS separately** | Many drills ask for both reads and writes, or for requests and records, and one side is deliberately larger. |
| 5 | **Peak is about 2 to 3 times the average** | Some drills give a traffic distribution and ask you to derive the peak; others ask you to pick the factor and say why. |
| 6 | **Tie each estimate to a design decision** | Every reference ends with what the number means for the design: shard or not, fits in memory or not, CDN or not. |
| 7 | **Estimation takes 0 to 5 minutes** | Each drill has a 5-minute limit. A correct answer over time is recorded as `slow`, and its review interval is halved. |
| 8 | **Do it yourself first, then compare** (section 3.6) | The reference derivation appears only after you answer. VS Code hides the drill files by default. |
| 9 | **Name where you got stuck** (the review card in section 3.6) | After the derivation is shown, you name the step where your estimate diverged and write one sentence. Nothing is recorded until you do. |
| 10 | **Follow the framework's numbers in order** (section 3.2) | Chain drills: users → QPS → storage → bandwidth → servers, one system at a time. |
| 11 | **Interleaving and spaced review** (the method of the earlier DSA report) | Without a drill id, due drills come first, then new ones, and never two of the same quantity type in a row. A missed drill comes back the next day. |
| 12 | **Find systematic bias** | `est stats` averages the log ratio per quantity type and tells you whether you systematically over- or under-estimate, or are just noisy. |

## 2. Install and build

1. Install WinLibs GCC. Skip this if you already did for cpp-practice or ai-round-practice.

   ```bash
   winget install --id BrechtSanders.WinLibs.POSIX.UCRT --exact
   ```

2. **With VS Code:** use File → Open Folder on `estimation-practice`. The first `est:` task you run builds `build/est.exe` (about 10 seconds).
3. **Without VS Code:** in the `estimation-practice` folder, run:

   ```bash
   g++ -std=c++20 -O2 -static -o build/est.exe tools/est.cpp
   build\est.exe --self-test
   ```

## 3. A session

| What | VS Code task | Terminal |
| --- | --- | --- |
| See what to do today | `est: 今日練習` | `est today` |
| Three interleaved drills (about 15 minutes) | `est: 交錯練習 3 題` (Ctrl+Shift+B) | `est drill` |
| A given drill or quantity type | `est: 練指定的題目或題型` | `est drill shortlink-qps`, `est drill bandwidth` |
| Due reviews | `est: 到期複習` | `est review` |
| A chain (about 15 minutes) | `est: 串連練習` | `est chain`, `est chain photo-app` |
| Bias report | `est: 偏差統計` | `est stats` |
| Drill list and status | `est: 題庫清單` | `est list` |
| Only list which drills would be picked | | `est drill -n 5 --plan` |

Once a task starts, type in the terminal panel. Each drill goes like this:

1. **Read the drill.** It lists the givens and what to estimate, for example "peak read QPS (rate; 3x allowed)". The clock starts.
2. **Write your assumptions,** one per line, ending with an empty line. For example `一天約 10^5 秒` (a day is about 10^5 s), `尖峰 = 平均 × 3`, `每筆 500 B`.
3. **Answer** each quantity with a number and a unit, for example `15k QPS`. If it is sent back, fix it as the message says.
4. **Read the result and the reference derivation.** Each answer shows its ratio and log10(yours / reference), followed by the reference's assumptions, the numbered derivation steps, and what the number means for the design.
5. **Name where you diverged.** Enter the number of the derivation step (0 means the assumptions themselves differ) and one sentence on why. The sentence is required after a miss or an overtime attempt; when everything hit you may enter `-` to skip.
6. **Recorded and scheduled.** Hit: 3 → 7 → 14 → 30 → 60 days. Overtime: the interval is halved. Miss: tomorrow. A hit at an interval of 60 days or more drops the drill from the due list.

**A chain** starts with the assumptions shared by the whole chain, then goes step by step. After each step the reference answer is shown and the next step continues from it, so each step measures only that step's estimate and one mistake does not spoil the rest. At the end you name the step where you first diverged, then the line of that step's derivation.

## 4. Scoring rules and why

**A hit is** |log10(your answer / reference)| ≤ log10(allowed factor).

| Allowed | Where | Why |
| --- | --- | --- |
| **3x** (about half an order of magnitude, log10 ≈ 0.48) | Drills that give every key number (63 questions, plus the 16 in chains) | The only legitimate differences then come from rounding: 10^5 or 86,400 seconds per day (16%), a peak factor of 2 or 3 (1.5x), and keeping 1 or 2 significant figures. References differ from exact arithmetic by at most 1.25x (section 9). A miss beyond 3x is usually structural: a missing factor (replicas, retention, fan-out), bits versus bytes (8x), or seconds per day (10^4 versus 10^5). And 3x is enough to change a design decision: one machine or three, fits in one machine's memory or not. |
| **10x** (one order of magnitude) | Drills where you assume a key size yourself (currently 2: `shortlink-storage` and `shortlink-cache`; the drill says so) | Reasonable assumptions differ by several times, for example 200 B or 2 KB per record. Section 3.3 asks for the order of magnitude, and the conclusion (how many machines, fits in memory or not) does not change within that range. |

- **Units are required.** An answer with a missing or wrong kind of unit is not scored; it is sent back.
- **Assumptions come first.** At least one line with a number before you can answer.
- **5 minutes per drill** (section 3.2: 0 to 5 minutes for estimation). `--minutes` changes it; chain steps default to 3 minutes each.
- **Bias uses the log ratio.** Overestimating by 2x and underestimating by 2x have log values of the same size and opposite sign, so they cancel in the average, and large numbers do not dominate. How `est stats` reads it: no verdict before 3 attempts; a mean |log| below 0.15 (about 1.4x) means no clear bias; above that, with at least two thirds of answers on the same side, it reports systematic over- or under-estimation.

## 5. Writing answers

| Kind | Accepted | Sent back |
| --- | --- | --- |
| Numbers | `3e5`, `300k`, `2.5M`, `1.5bn`, `86,400`, `2^30`, `3×10^5`, `2000萬`, `1.5億`, `2千萬`; may start with `~`, `≈` or `約` | `300k` (no unit), `3,00` (bad thousands separator) |
| Rates | `15k QPS`, `200 /s`, `3k req/s`, `5億 次/天`, `12 req/min`, `3k 次每秒` | `3000 requests` (no "per second"), `40 MB/kg` |
| Data | `1.2 TB`, `500 GB`, `64 KiB`, `8 Tb` (bits, converted to 1 TB) | `1.2 tb`, `3 gb` (bits or bytes?), `40 mB` (lowercase m means milli) |
| Bandwidth | `40 MB/s`, `320 Mbps`, `10 Gb/s`, `2 TB/day` | `40 mb/s`, `13.5 GB` (a total where a rate is asked) |
| Time | `500 µs`, `500 us`, `150 ms`, `2.5 s`, `22 min`, `1.8 h`, `3 天` | `5m` (write min for minutes) |
| Counts | `43 台`, `200 個分片`, `200 partitions`, `34 GPU servers`, `1億 人`, `2.5M users` | `2.5B users` (B means bytes; write 1bn or 10億 for a billion) |

Prefixes are decimal (1 KB = 1,000 B), matching the report's "2^10 ≈ a thousand" estimation habit; KiB and MiB are binary. `est parse <text>` shows how a notation will be read.

## 6. The drill bank

| Quantity type | Drills | Questions | Drills |
| --- | --- | --- | --- |
| QPS (average and peak, reads and writes) | 9 | 16 | URL shortener, evening feed peak, chat messages, driver locations, flash sale, price tracking, post search, online chess, rate-limiter counters |
| Storage (N years, replicas, retention) | 7 | 12 | URL shortener over 10 years, photos over 5 years, chat kept 30 days, metrics for a year, ad-click logs, document snapshots, web crawler |
| Bandwidth | 6 | 8 | video egress and origin, video upload, image CDN, cross-region replication, central logging, file sync |
| Memory (caches, indexes) | 6 | 7 | short-link cache, login sessions, geo index, latest driver locations, top-K counters, feed cache |
| Servers and partitions | 6 | 10 | API servers (with an AZ failure), Kafka partitions, database shards, judge machines, WebSocket gateways, LLM GPU servers |
| Fan-out | 4 | 5 | feed fan-out (with a celebrity), group messages, live comments, breaking-news push |
| Time and latency | 4 | 7 | 99.95% downtime budget, memory versus disk scan, cross-continent serial calls, rebuilding a storage node |
| **Total** | **42** | **65** | |

"Drills" counts each drill under its main quantity type; "Questions" counts every question by its own type, so one drill can practise two quantities (for example storage and bandwidth).

**Chains** (5 steps each: users → QPS → storage → bandwidth → servers; 16 questions in all):

| Chain | System |
| --- | --- |
| `photo-app` | a photo-sharing app (Instagram-like) |
| `chat-app` | a chat app (WhatsApp-like) |
| `ride-app` | the driver-location service of a ride-hailing app (Uber-like) |

The drills cover 24 of the 32 problems in section 3.5: Bitly, Dropbox, Yelp, Ticketmaster, Instagram, FB News Feed, LeetCode, WhatsApp, Distributed Cache, Rate Limiter, YouTube, FB Live Comments, Price Tracking Service, Notification System, YouTube Top K, Uber, Google Docs, Web Crawler, Ad Click Aggregator, FB Post Search, Metrics Monitoring, Online Chess, ChatGPT and Flash Sale. A few are general (API servers, availability, the latency table). The other 8 (Local Delivery Service, Tinder, Strava, Online Auction, Job Scheduler, News Aggregator, Robinhood, Payment System) can be added in the format of section 10.

**Per-machine capacity is always given.** Section 3.3 warns against memorizing a "single-machine limit", so what one server, Redis node or GPU server handles is always stated as a load-test result or a drill assumption. In an interview, say what per-machine capacity you are assuming.

## 7. Reference numbers

These are the only facts the drills rely on; every other number is a practice assumption stated in the drill.

| Number | Value | Source | Date |
| --- | --- | --- | --- |
| Seconds in a day | 86,400 ≈ 10^5 | Research report, section 3.3 | report research date 2026-11-15 |
| Daily volume to average QPS | 1 million/day ≈ 12, 100 million ≈ 1,200, 1 billion ≈ 12,000 | Research report, section 3.3 (derived from 86,400) | 2026-11-15 |
| Seconds in a 30-day month | 2,592,000 ≈ 2.5×10^6 | This kit, from 30 × 86,400 | 2026-11-16 |
| Seconds in a 365-day year | 31,536,000 ≈ 3×10^7 | This kit, from 365 × 86,400 | 2026-11-16 |
| Peak and average | peak is about 2 to 3 times the average | Research report, section 3.3 | 2026-11-15 |
| Powers of two | 2^10 ≈ a thousand, 2^20 ≈ a million, 2^30 ≈ a billion, 2^40 ≈ a trillion, 2^50 ≈ a quadrillion | Research report, section 3.3 | 2026-11-15 |
| Bits and bytes | 1 B = 8 bits | definition | |
| Read 1 MB sequentially from memory | 250 µs | Jeff Dean's 2010 list via ByteByteGo (report section 3.3) | checked 2026-11-15 |
| Read 1 MB sequentially from disk | 30 ms | same | checked 2026-11-15 |
| Round trip within a datacenter | 500 µs | same | checked 2026-11-15 |
| Round trip California to the Netherlands | 150 ms | same | checked 2026-11-15 |
| Monthly downtime at 99.9% | 43.8 min | Research report, section 3.3 (derived by the report from the formula) | 2026-11-15 |
| Monthly downtime at 99.99% | 4.38 min | ByteByteGo (report section 3.3) | checked 2026-11-15 |

The latency figures are 2010 orders of magnitude, and ByteByteGo notes some are out of date. Use them only to compare, not to predict how fast modern hardware is (report section 8). ByteByteGo page: https://bytebytego.com/courses/system-design-interview/back-of-the-envelope-estimation

## 8. Fitting it into the 12-week plan

The "System design" column of the report's section 7.2 schedules reading about estimation (section 3.3) in weeks 1–2. A suggested schedule for the kit, taking 30 to 45 minutes a week out of the roughly 7 hours of system design in section 7.1:

| Weeks | Practice |
| --- | --- |
| 1–2 | After reading section 3.3, run `est drill` twice a week (3 drills, about 15 minutes each time) until you have tried all 7 quantity types |
| 3–8 | Before each full system design practice, run `est review` (due drills, 5 to 10 minutes); one `est chain` a week |
| 9–12 | Only `est review` and chains; check `est stats` the day before a mock interview to make sure no type shows a systematic bias |

**Readiness** (this kit's suggestion, not part of the report's section 7.4): at least 3 attempts per quantity type with a hit rate of 80% or more; no type flagged as systematically biased by `est stats`; a chain finished within 15 minutes with no missed step.

You may remember the numbers when you redo a drill. So in a review, the point is not the number but whether you can rewrite the assumptions and derivation within 5 minutes without looking. Interviewers also change the constraints (the variant drills in section 3.6), for example "writes grow 10x", and you should be able to change that one step of your derivation.

## 9. Verified

Everything below was actually run on Windows 11 with WinLibs g++ 16.2.0 and Python 3.14.

- **Unit-parser tests:** all 89 cases in `est --self-test` pass: 63 accepted notations, 15 notations that must be rejected, 6 wrong-kind-of-unit cases, and 5 formatting checks.
- **Bank check:** `est --self-test` also checks the 42 drills and 3 chains (15 steps): every question has a reference answer of the right kind of unit, every derivation step has a known type, and chain steps are numbered in order.
- **Every reference answer re-derived by a second method:** `tools/verify_bank.py` (Python standard library only, sharing no code with `est.cpp`) has its own unit parser and bank reader. It recomputes **all 81 questions** (65 in the 42 drills and 16 in the 15 chain steps) from the givens with exact values (86,400 seconds per day, 365-day years). Every reference is within 1.35x of the exact value; the largest gap is 1.25x (`video-upload` ingress: reference 100 Gbps, exact 125 Gbps). It also confirms that each reference answer's text appears in its derivation, and that each chain step's "previous answer" given matches that earlier step's reference.
- **The two parsers cross-checked:** for all 279 givens and answers in the bank (plus 22 extra notations), `est` and the Python parser agree on the kind of unit and the value; both reject the same 8 bad notations.
- **The full loop was run** (input piped in, progress written to a scratch folder): a correct answer (recorded as hit, review in 3 days); a missing unit, a wrong kind of unit and an ambiguous case, each sent back, followed by an 8x bits-versus-bytes miss (recorded as miss, review tomorrow); the 5 steps of the `photo-app` chain (step 4 deliberately missing a factor of 10, recorded as miss); refusing to answer without assumptions; refusing to record a miss without a reason, exiting non-zero; a correct answer over time (recorded as slow); the `est stats` bias report; with `EST_TODAY` set to the next day, `est today` listing the due drills and a correct `est review` moving the interval to 3 days; and `est drill --plan` picking 7 different quantity types for the first 7 drills. Most runs piped input from Git Bash; one drill was also completed from Windows PowerShell 5.1 (with `$OutputEncoding` set to UTF-8) with Chinese assumptions, and the Chinese in the record was correct.
- **The VS Code build command:** with `build/est.exe` deleted, the command of the task "est: 確認工具已建置", run with `cmd.exe`, builds `est.exe` again.

**Not verified:**

- Typing answers interactively in a VS Code terminal panel. On a Windows console the tool reads input with `ReadConsoleW` so Chinese input does not depend on the code page, but only piped input was tested, not typing in a real console or a VS Code task.
- Linux and macOS: the code handles them, but it was not compiled or run there.
- The scenario numbers in the drills (user counts, record sizes, per-machine capacity) are practice assumptions and were not checked against any real system.

## 10. Adding drills

The bank lives in `drills/*.txt`, plain UTF-8 text. VS Code hides that folder by default; to edit it, set `"drills"` in `.vscode/settings.json` to `false` for a while.

```
== drill <id>
title: drill name
kind: the matching problem in section 3.5
type: storage                          (optional: the quantity type it is filed under; default is the first question's)
scenario: the situation (may repeat; lines are joined)
given: key = quantity | label           (shown before you answer)
ask: key | quantity type | unit kind | label [| allowed factor]
answer: key = quantity                  (the reference, in round numbers)
assume: an assumption of the reference
calc: step type | one step of the derivation   (one line must contain the answer's exact text)
design: what the number means for the design
```

- Quantity types: `users`, `qps`, `storage`, `bandwidth`, `memory`, `servers`, `fanout`, `time`.
- Unit kinds: `count`, `rate`, `bytes`, `byterate`, `time`.
- Step types (`est stats` uses them to show which kind of step you tend to get wrong): `assume` (the assumption itself), `unit` (unit or time conversion), `peak` (peak factor), `mix` (read/write ratio or a share), `size` (size of one item), `mult` (multiplying by a count or a duration), `copy` (replicas, indexes and overhead), `cap` (per-machine capacity and headroom), `fan` (fan-out factor).
- A chain is written as `== chain <id>` with the shared scenario, and each step as `== step <chain-id> <n>`.

After editing, run `est --self-test`. If you have Python, add the drill's formula to `FORMULAS` in `tools/verify_bank.py` and run:

```bash
python tools/verify_bank.py --exe build/est.exe
```

Learners do not need Python; the script is only for whoever maintains the bank.

## 11. Layout

```
estimation-practice/
├─ .vscode/               tasks; hides drills/ (it holds the references), build/ and progress/
├─ tools/
│  ├─ est.cpp             the CLI (compiled the first time you run a task)
│  └─ verify_bank.py      for maintainers: re-derives every reference and cross-checks the two parsers
├─ drills/                the bank: 10-qps, 20-storage, 30-bandwidth, 40-memory, 50-servers,
│                          60-fanout, 70-time, 90-chains
├─ progress/              your records (created automatically, not under version control)
│  ├─ attempts.tsv        per question: answer, reference, log ratio, the step where you diverged
│  ├─ reviews.tsv         per attempt: result and next review date
│  └─ journal.md          your assumptions and reasons
└─ build/                 build output (safe to delete at any time)
```

## 12. Troubleshooting

| Symptom | Fix |
| --- | --- |
| `g++` not found | Reopen the terminal or VS Code after installing WinLibs. Check with `where g++`. |
| Chinese shows as garbage | Run `chcp 65001` first. |
| Chinese turns into question marks when piping answers from PowerShell | The pipe encoding is `$OutputEncoding`. Set it to `[Text.Encoding]::UTF8`, or type the answers interactively. |
| Start over | Delete the `progress/` folder. |
| Changed `tools/est.cpp` | Run the task "est: 重新建置工具". |
| Keep records somewhere else | Set the environment variable `EST_PROGRESS` to that folder. |
