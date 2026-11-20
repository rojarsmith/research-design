"""Independent check of the drill bank, for maintainers. Learners do not need Python.

It does not share code with est.cpp. It has its own quantity parser and its own bank reader, and
for every question it recomputes the answer from the stated givens with exact arithmetic
(86,400 s per day, 365-day years, 30-day months) instead of the round numbers the reference
derivations use. A reference answer passes when it is within ROUNDING of the exact value.

It also checks that:
  - each reference answer's text appears in one of its derivation lines,
  - each chain step's "previous answer" given equals that earlier step's reference answer,
  - est.exe parses every given and answer in the bank (and a list of extra notations) to the same
    dimension and value as this script's parser (pass --exe build/est.exe).

Usage: python tools/verify_bank.py [--exe build/est.exe]   (run from estimation-practice/)
"""

import math
import re
import subprocess
import sys
from pathlib import Path

DAY = 86400.0
MONTH = 30 * DAY
YEAR = 365 * DAY
ROUNDING = 1.35  # round numbers (10^5 s per day, 1-2 significant figures) may differ this much

# ---------------------------------------------------------------- quantity parser

TIME = {
    "ns": 1e-9, "us": 1e-6, "µs": 1e-6, "μs": 1e-6, "ms": 1e-3, "s": 1, "sec": 1, "secs": 1,
    "second": 1, "seconds": 1, "min": 60, "mins": 60, "minute": 60, "minutes": 60, "h": 3600,
    "hr": 3600, "hrs": 3600, "hour": 3600, "hours": 3600, "d": DAY, "day": DAY, "days": DAY,
    "wk": 7 * DAY, "week": 7 * DAY, "weeks": 7 * DAY, "mo": MONTH, "month": MONTH, "months": MONTH,
    "y": YEAR, "yr": YEAR, "yrs": YEAR, "year": YEAR, "years": YEAR,
    "秒": 1, "秒鐘": 1, "毫秒": 1e-3, "微秒": 1e-6, "奈秒": 1e-9, "納秒": 1e-9, "分": 60, "分鐘": 60,
    "小時": 3600, "時": 3600, "天": DAY, "日": DAY, "週": 7 * DAY, "月": MONTH, "個月": MONTH, "年": YEAR,
}
PREFIX = {"k": 1, "m": 2, "g": 3, "t": 4, "p": 5, "e": 6}
NOUNS_EN = set("""server servers machine machines node nodes host hosts instance instances box boxes vm vms
pod pods container containers partition partitions shard shards replica replicas worker workers gpu gpus
core cores cpu cpus disk disks broker brokers consumer consumers connection connections conn conns user users
people person dau mau driver drivers rider riders device devices request requests req reqs query queries
call calls op ops operation operations read reads write writes msg msgs message messages event events point
points sample samples update updates delivery deliveries push pushes send sends notification notifications
item items key keys row rows record records entry entries token tokens photo photos image images video videos
post posts file files link links url urls page pages click clicks view views submission submissions comment
comments game games move moves doc docs document documents order orders ticket tickets block blocks chunk
chunks object objects counter counters insert inserts timeline timelines copy copies unit units job jobs
task tasks""".split())
ZH_MEASURE = "台臺個个部片份次人則筆張支條封篇名位組顆塊項件輪"
ZH_NOUNS = ["伺服器", "機器", "節點", "分片", "分區", "副本", "連線", "訊息", "請求", "使用者", "用戶", "工作者",
            "執行個體", "貼文", "照片", "網頁", "裝置", "司機", "推播", "寫入", "讀取", "事件", "點"]
MAGNITUDE = [("bn", 1e9), ("k", 1e3), ("K", 1e3), ("M", 1e6), ("十", 10), ("百", 100), ("千", 1e3),
             ("萬", 1e4), ("万", 1e4), ("億", 1e8), ("亿", 1e8)]

NUM_RE = re.compile(r"(\d{1,3}(?:,\d{3})+|\d+)(\.\d+)?(?:([eE][+-]?\d{1,3})|\^(-?\d{1,3}))?"
                    r"(?:\s*(?:×|x|X|\*)\s*10\^(-?\d{1,3}))?")


class ParseError(Exception):
    pass


def data_unit(u):
    """Bytes per unit for a data unit, or None. Raises ParseError for ambiguous case."""
    if u in ("B", "位元組", "字節") or u.lower() in ("byte", "bytes"):
        return 1.0
    if u in ("b", "位元") or u.lower() in ("bit", "bits"):
        return 0.125
    m = re.fullmatch(r"([kKmMgGtTpPeE])(i?)(B|b|bit|bits|byte|bytes)", u, flags=0)
    if not m:
        m2 = re.fullmatch(r"([kKmMgGtTpPeE])(i?)(?i:(bits?|bytes?))", u)
        if not m2:
            return None
        m = m2
    p, binary, rest = m.group(1), m.group(2), m.group(3)
    power = PREFIX[p.lower()]
    factor = (1024.0 if binary else 1000.0) ** power
    is_bit = rest == "b" or rest.lower().startswith("bit")
    if p == "m":
        raise ParseError("lowercase m")
    if rest == "b" and p.islower():
        raise ParseError("ambiguous")
    return factor * (0.125 if is_bit else 1.0)


def count_unit(u):
    u = u.strip()
    if not u:
        return False
    if u.isascii():
        words = u.lower().split()
        if not 1 <= len(words) <= 3:
            return False
        for w in words[:-1]:
            if w in ("k", "m", "b", "bn", "g", "t") or w[0].isdigit():
                return False
        return words[-1] in NOUNS_EN and all(re.fullmatch(r"[a-z0-9-]+", w) for w in words)
    return u[0] in ZH_MEASURE or any(u.endswith(n) for n in ZH_NOUNS)


def unit_value(u, v):
    """Returns (dimension, value in base units) or raises ParseError."""
    u = u.strip()
    if not u:
        raise ParseError("missing unit")
    u = u.replace("每", "/").replace(" per ", "/")
    lu = u.lower()
    if u == "%":
        return "ratio", v / 100
    if u in ("倍", "×") or lu == "x":
        return "ratio", v
    if lu in ("qps", "rps", "tps"):
        return "rate", v
    m = re.fullmatch(r"([kmgtp]?)bps", lu)
    if m:
        f = {"": 1, "k": 1e3, "m": 1e6, "g": 1e9, "t": 1e12, "p": 1e15}[m.group(1)]
        return "byterate", v * f / 8
    if "/" in u:
        left, right = (s.strip() for s in u.split("/", 1))
        sec = TIME.get(right.lower())
        if sec is None:
            raise ParseError("not a time unit")
        if left == "" or count_unit(left):
            return "rate", v / sec
        b = data_unit(left)
        if b is not None:
            return "byterate", v * b / sec
        raise ParseError("bad left side")
    b = data_unit(u)
    if b is not None:
        return "bytes", v * b
    if lu in TIME:
        return "time", v * TIME[lu]
    if count_unit(u):
        return "count", v
    raise ParseError("unknown unit " + u)


def parse(text):
    s = text.strip()
    for approx in ("~", "≈", "大約", "約"):
        if s.startswith(approx):
            s = s[len(approx):].strip()
    m = NUM_RE.match(s)
    if not m:
        raise ParseError("no number")
    v = float(m.group(1).replace(",", "") + (m.group(2) or ""))
    if m.group(3):
        v *= 10.0 ** int(m.group(3)[1:])
    if m.group(4):
        v = v ** int(m.group(4))
    if m.group(5):
        v *= 10.0 ** int(m.group(5))
    rest = s[m.end():]
    try:
        return unit_value(rest, v)
    except ParseError as first:
        err = first
    r = rest.strip()
    mult, ascii_used = 1.0, False
    for _ in range(3):
        for word, f in MAGNITUDE:
            if not r.startswith(word):
                continue
            if word in ("k", "K", "M"):
                if ascii_used:
                    continue
                after = r[len(word):]
                if after and after[0] != " ":
                    try:
                        if data_unit(r) is not None or r.lower().endswith("bps"):
                            continue
                    except ParseError:
                        continue
                ascii_used = True
            r = r[len(word):].strip()
            mult *= f
            break
        else:
            break
        try:
            return unit_value(r, v * mult)
        except ParseError as e:
            err = e
    raise err


# ---------------------------------------------------------------- bank reader


def read_bank(folder):
    items = []
    for path in sorted(Path(folder).glob("*.txt")):
        for n, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
            t = line.strip()
            if not t or t.startswith("#"):
                continue
            if t.startswith("== "):
                parts = t[3:].split()
                item = {"rec": parts[0], "given": {}, "given_text": {}, "ask": [], "answer": {},
                        "answer_text": {}, "calc": [], "where": f"{path.name}:{n}"}
                if parts[0] == "step":
                    item["chain"], item["order"] = parts[1], int(parts[2])
                    item["id"] = f"{parts[1]}#{parts[2]}"
                else:
                    item["id"] = parts[1]
                items.append(item)
                continue
            key, _, val = t.partition(":")
            key, val = key.strip(), val.strip()
            it = items[-1]
            if key == "given":
                k, _, rest = val.partition("=")
                q, _, _label = rest.partition("|")
                it["given_text"][k.strip()] = q.strip()
                it["given"][k.strip()] = parse(q.strip())
            elif key == "ask":
                f = [x.strip() for x in val.split("|")]
                it["ask"].append({"key": f[0], "type": f[1], "dim": f[2],
                                  "tol": float(f[4]) if len(f) > 4 else 3.0})
            elif key == "answer":
                k, _, q = val.partition("=")
                it["answer_text"][k.strip()] = q.strip()
                it["answer"][k.strip()] = parse(q.strip())
            elif key == "calc":
                it["calc"].append(val.partition("|")[2].strip())
    return items


class G:
    """Given values by name, in base units: g.dau, g.links, ..."""

    def __init__(self, values):
        self.__dict__.update({k: v[1] for k, v in values.items()})


# ---------------------------------------------------------------- formulas, written from the givens
# Constants that are not givens come from each drill's "assume:" lines and are named here.

FORMULAS = {
    "shortlink-qps": {
        "write_avg": lambda g: g.links,
        "read_peak": lambda g: g.links * g.reads * 3,  # assume: peak = 3x average
    },
    "feed-evening-peak": {
        "avg": lambda g: g.dau * g.opens / DAY,
        "evening": lambda g: g.dau * g.opens * g.share / (3 * 3600),
    },
    "chat-send-qps": {"peak": lambda g: g.dau * g.msgs / DAY * 2},  # assume: peak = 2x
    "driver-location-qps": {
        "points": lambda g: g.online / g.every,
        "requests": lambda g: g.online / g.batch,
    },
    "flash-sale-qps": {"buy": lambda g: g.waiting * g.tries / g.window},
    "price-tracker-crawl": {"fetch": lambda g: g.products / g.period},
    "search-read-write": {
        "search_peak": lambda g: g.dau * g.searchers * g.searches / DAY * 3,
        "index_avg": lambda g: g.posts,
    },
    "chess-moves": {"moves": lambda g: g.games / g.interval},
    "rate-limiter-redis": {
        "ops": lambda g: g.peak * g.rules,
        "nodes": lambda g: math.ceil(g.peak * g.rules / (g.node * g.target)),
    },
    "shortlink-storage": {"total": lambda g: g.links * g.years * 500 * g.copies},  # assume: 500 B per record
    "photo-storage": {
        "daily": lambda g: g.uploads * DAY * (g.original + g.thumbs),
        "total": lambda g: g.uploads * g.years * (g.original + g.thumbs),
    },
    "chat-retention": {"total": lambda g: g.msgs * g.keep * g.size * g.copies},
    "metrics-storage": {
        "points": lambda g: g.hosts * g.metrics / g.interval,
        "year": lambda g: g.hosts * g.metrics / g.interval * YEAR * g.point,
    },
    "ad-click-logs": {"total": lambda g: g.clicks * g.keep * g.size},
    "doc-snapshots": {
        "daily": lambda g: g.docs * DAY * (g.ops * g.op + g.ops / 100 * g.snap),
        "total": lambda g: g.docs * g.years * (g.ops * g.op + g.ops / 100 * g.snap),
    },
    "crawler-storage": {
        "storage": lambda g: g.pages * g.html / g.ratio * g.versions,
        "download": lambda g: g.pages * g.html / g.period,
    },
    "video-egress": {
        "egress": lambda g: g.viewers * g.bitrate,
        "origin": lambda g: g.viewers * g.bitrate * (1 - g.hit),
    },
    "video-upload": {
        "ingress": lambda g: g.videos * g.length * g.bitrate,
        "daily": lambda g: g.videos * DAY * g.length * g.bitrate,
    },
    "photo-cdn-egress": {"peak": lambda g: g.views * g.size * 3},  # assume: peak = 3x
    "cross-region-replication": {"bw": lambda g: g.writes * g.size * g.regions},
    "log-shipping": {
        "ingest": lambda g: g.servers * g.per,
        "total": lambda g: g.servers * g.per * g.keep * g.copies,
    },
    "dropbox-sync": {"upload": lambda g: g.dau * g.changes * g.delta / DAY},
    "shortlink-cache": {"mem": lambda g: g.active * 200},  # assume: 200 B per entry
    "session-store": {"mem": lambda g: g.sessions * g.size * g.overhead * g.copies},
    "geo-index-memory": {"mem": lambda g: g.places * g.entry * g.overhead},
    "driver-location-memory": {
        "mem": lambda g: g.drivers * g.latest,
        "track": lambda g: g.drivers * (g.hours / g.every) * g.point,
    },
    "topk-counters": {
        "exact": lambda g: g.videos * g.entry,
        "sketch": lambda g: g.rows * g.width * g.counter,
    },
    "feed-cache": {
        "mem": lambda g: g.users * g.ids * g.id,
        "nodes": lambda g: math.ceil(g.users * g.ids * g.id / g.node) * g.copies,
    },
    "api-servers": {
        "base": lambda g: g.peak / (g.per * g.target),
        "az": lambda g: g.peak / (g.per * g.target) * 3 / 2,  # assume: 3 AZs, survive losing 1
    },
    "kafka-partitions": {"parts": lambda g: g.peak / g.per * g.growth},
    "metadata-shards": {"shards": lambda g: max(g.data / g.shard_size, g.writes / g.shard_writes)},
    "judge-workers": {
        "rate": lambda g: g.users * g.subs * g.last / 600,  # 10 minutes
        "machines": lambda g: g.users * g.subs * g.last / 600 * g.cpu / (g.cores * g.target),
    },
    "ws-gateways": {"gateways": lambda g: g.conns / (g.per * g.target)},
    "llm-gpus": {
        "tokens": lambda g: g.peak * g.out,
        "servers": lambda g: g.peak * g.out / g.per,
    },
    "feed-fanout": {
        "writes": lambda g: g.posts * g.followers,
        "celeb_time": lambda g: g.celeb / g.speed,
    },
    "group-chat-fanout": {"deliveries": lambda g: g.msgs * (g.size - 1)},
    "live-comments": {
        "naive": lambda g: g.viewers * g.comments,
        "sampled": lambda g: g.viewers * g.cap,
    },
    "push-blast": {
        "rate": lambda g: g.devices / g.window,
        "workers": lambda g: (g.devices / g.window) / (g.inflight / g.latency),
    },
    "downtime-budget": {"budget": lambda g: (1 - g.sla) * g.month},
    # Report section 3.3 latency table: 1 MB sequential from memory 250 us, from disk 30 ms.
    "memory-vs-disk-scan": {
        "mem": lambda g: g.data / 1e6 * 250e-6,
        "disk": lambda g: g.data / 1e6 * 30e-3,
    },
    # Report section 3.3: California-Netherlands round trip 150 ms; same datacenter 500 us.
    "cross-region-calls": {
        "serial": lambda g: g.calls * 0.150,
        "same_dc": lambda g: g.calls * 500e-6,
    },
    "replica-rebuild": {"time": lambda g: g.data / (g.link * g.share)},
    "photo-app#1": {"dau": lambda g: g.mau * g.daily},
    "photo-app#2": {
        "write": lambda g: g.dau * g.uploads / DAY,
        "read_peak": lambda g: g.dau * g.opens / DAY * 3,
    },
    "photo-app#3": {"total": lambda g: g.write * g.size * g.years},
    "photo-app#4": {"egress": lambda g: g.read_peak * g.images * g.thumb},
    "photo-app#5": {"servers": lambda g: g.read_peak / (g.per * g.target)},
    "chat-app#1": {"dau": lambda g: g.registered * g.daily},
    "chat-app#2": {"peak": lambda g: g.dau * g.msgs / DAY * 2},
    "chat-app#3": {"total": lambda g: g.daily * g.keep * g.size * g.copies},
    "chat-app#4": {"bw": lambda g: g.peak * g.size * (1 + g.recipients)},
    "chat-app#5": {"gateways": lambda g: g.dau * g.online / g.per},
    "ride-app#1": {"online": lambda g: g.drivers * g.online},
    "ride-app#2": {"updates": lambda g: g.online / g.every},
    "ride-app#3": {"total": lambda g: g.avg_online / g.every * g.point * g.keep},
    "ride-app#4": {"ingress": lambda g: g.updates * g.req},
    "ride-app#5": {"servers": lambda g: g.updates / g.per},
}

EXTRA_NOTATIONS = [
    "3e5 QPS", "300k QPS", "2.5M users", "1.2 TB", "40 MB/s", "320 Mbps", "10 Gb/s", "1 GiB", "2^30 B",
    "3×10^5 /s", "86,400 s", "5億 次/天", "2千萬人", "500 µs", "1.8 h", "34 GPU servers", "3k 次每秒",
    "~15k QPS", "2.5bn users", "8 Tb", "12 req/min", "40%",
]
EXTRA_REJECTS = ["300k", "3500", "1.2 tb", "40 mb/s", "5m", "40 MB/kg", "12 furlongs", "2.5B users"]


def main():
    kit = Path(__file__).resolve().parent.parent
    exe = None
    if "--exe" in sys.argv:
        exe = Path(sys.argv[sys.argv.index("--exe") + 1])
    items = read_bank(kit / "drills")
    chains = {it["id"]: it for it in items if it["rec"] == "chain"}
    problems, rows = [], []
    checked_items = checked_questions = 0
    worst = 0.0
    prev_answers = {}
    for it in items:
        if it["rec"] == "chain":
            continue
        givens = dict(it["given"])
        if it["rec"] == "step":
            chain = chains[it["chain"]]
            for k, v in chain["given"].items():
                givens.setdefault(k, v)
            # A given that names an earlier step's answer must equal that reference answer.
            earlier = prev_answers.setdefault(it["chain"], {})
            for k, v in it["given"].items():
                if k in earlier and abs(v[1] - earlier[k][1]) > 1e-9 * abs(earlier[k][1]):
                    problems.append(f"{it['id']}: given {k} = {v[1]} but the earlier answer is {earlier[k][1]}")
        formulas = FORMULAS.get(it["id"])
        if formulas is None:
            problems.append(f"{it['id']}: no formula")
            continue
        g = G(givens)
        checked_items += 1
        for ask in it["ask"]:
            k = ask["key"]
            if k not in formulas:
                problems.append(f"{it['id']}.{k}: no formula")
                continue
            exact = formulas[k](g)
            dim, ref = it["answer"][k]
            if dim != ask["dim"]:
                problems.append(f"{it['id']}.{k}: answer dimension {dim}, question wants {ask['dim']}")
            ratio = ref / exact
            worst = max(worst, abs(math.log10(ratio)))
            ok = 1 / ROUNDING <= ratio <= ROUNDING
            checked_questions += 1
            rows.append((it["id"], k, it["answer_text"][k], exact, ratio, ok))
            if not ok:
                problems.append(f"{it['id']}.{k}: reference {it['answer_text'][k]} vs exact {exact:.4g} (x{ratio:.2f})")
            if not any(it["answer_text"][k] in c for c in it["calc"]):
                problems.append(f"{it['id']}.{k}: answer text '{it['answer_text'][k]}' is not in any calc line")
        if it["rec"] == "step":
            prev_answers[it["chain"]].update(it["answer"])
    # chat-app step 3 uses step 2's daily volume, which is an intermediate, not an answer.
    by_id = {it["id"]: it for it in items}
    if "chat-app#3" in by_id:
        s2 = by_id["chat-app#2"]["given"]
        expect = s2["dau"][1] * s2["msgs"][1] / DAY
        got = by_id["chat-app#3"]["given"]["daily"][1]
        if abs(got - expect) > 1e-9 * expect:
            problems.append(f"chat-app#3: daily {got} != dau x msgs {expect}")

    for r in rows:
        print(f"{'ok ' if r[5] else 'BAD'} {r[0]:<26} {r[1]:<12} ref {r[2]:<20} exact {r[3]:<12.4g} ref/exact {r[4]:.3f}")
    print(f"\nRe-derived {checked_questions} questions in {checked_items} items "
          f"({sum(1 for i in items if i['rec'] == 'drill')} drills, "
          f"{sum(1 for i in items if i['rec'] == 'step')} chain steps). "
          f"Largest deviation from exact: x{10 ** worst:.3f} (allowed x{ROUNDING}).")

    if exe:
        texts = []
        for it in items:
            texts += list(it["given_text"].values()) + list(it["answer_text"].values())
        texts += EXTRA_NOTATIONS
        out = subprocess.run([str(exe), "parse", "--raw", *texts], capture_output=True, text=True,
                             encoding="utf-8", check=True).stdout.splitlines()
        agree = 0
        for text, line in zip(texts, out):
            f = line.split("\t")
            dim, val = parse(text)
            if f[0] != "ok" or f[1] != dim or abs(float(f[2]) - val) > 1e-9 * max(1.0, abs(val)):
                problems.append(f"parser disagreement on '{text}': est says {line!r}, Python says {dim} {val}")
            else:
                agree += 1
        rej = subprocess.run([str(exe), "parse", "--raw", *EXTRA_REJECTS], capture_output=True, text=True,
                             encoding="utf-8", check=True).stdout.splitlines()
        rejected = 0
        for text, line in zip(EXTRA_REJECTS, rej):
            try:
                parse(text)
                py_rejects = False
            except ParseError:
                py_rejects = True
            if line.startswith("err") and py_rejects:
                rejected += 1
            else:
                problems.append(f"reject disagreement on '{text}': est {line!r}, Python rejects={py_rejects}")
        print(f"Parsers agree on {agree}/{len(texts)} quantities and reject {rejected}/{len(EXTRA_REJECTS)} bad ones.")

    if problems:
        print("\nProblems:")
        for p in problems:
            print("  " + p)
        return 1
    print("All checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
