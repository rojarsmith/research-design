"""Bitcoin 與橢圓曲線的最小示範，只用 Python 標準函式庫。

    python toy_bitcoin.py curve   # 小曲線 y^2 = x^3 + 7 (mod 17) 上的點與加法
    python toy_bitcoin.py keys    # secp256k1 私鑰 -> 公鑰，正向快、反向只能猜
    python toy_bitcoin.py sign    # Schnorr 式簽章：簽、驗、改一個字就失敗、nonce 重用洩漏私鑰
    python toy_bitcoin.py pow     # 工作量證明：找讓雜湊開頭有 N 個 0 的 nonce
    python toy_bitcoin.py chain   # 區塊鏈：改舊區塊會讓後面全部對不上
    python toy_bitcoin.py all

這是教學程式，不是錢包：沒有防旁路攻擊、沒有安全亂數保證以外的保護，
簽章格式也是簡化版（不是 BIP340 的逐位元相容實作）。不要拿來管真的錢。
"""

import hashlib
import secrets
import sys
import time

# ---------------------------------------------------------------- 有限體上的曲線


class Curve:
    """y^2 = x^3 + a x + b (mod p)。點用 (x, y) 表示，None 代表「無窮遠點」（加法的 0）。"""

    def __init__(self, p, a, b, g=None, n=None):
        self.p, self.a, self.b, self.g, self.n = p, a, b, g, n

    def on_curve(self, pt):
        if pt is None:
            return True
        x, y = pt
        return (y * y - (x * x * x + self.a * x + self.b)) % self.p == 0

    def add(self, p1, p2):
        """弦切法：兩點連線交曲線第三點，再對 x 軸翻過來。"""
        if p1 is None:
            return p2
        if p2 is None:
            return p1
        x1, y1 = p1
        x2, y2 = p2
        p = self.p
        if x1 == x2 and (y1 + y2) % p == 0:
            return None  # P + (-P) = 0
        if p1 == p2:
            m = (3 * x1 * x1 + self.a) * pow(2 * y1, -1, p) % p  # 切線斜率
        else:
            m = (y2 - y1) * pow(x2 - x1, -1, p) % p  # 割線斜率
        x3 = (m * m - x1 - x2) % p
        y3 = (m * (x1 - x3) - y1) % p
        return (x3, y3)

    def mul(self, k, pt):
        """純量乘法 k*P：倍加法（double-and-add），步數約等於 k 的位元數。"""
        result, addend, steps = None, pt, 0
        while k:
            if k & 1:
                result = self.add(result, addend)
                steps += 1
            addend = self.add(addend, addend)
            steps += 1
            k >>= 1
        self.last_steps = steps
        return result


# Bitcoin 用的曲線 secp256k1（SEC 2 v2, 2.4.1）
P = 2**256 - 2**32 - 977
N = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
G = (
    0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798,
    0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8,
)
SECP256K1 = Curve(P, 0, 7, G, N)


def h(*parts):
    data = b"".join(x if isinstance(x, bytes) else str(x).encode() for x in parts)
    return hashlib.sha256(data).digest()


def hint(*parts):
    return int.from_bytes(h(*parts), "big")


# ---------------------------------------------------------------- 示範


def demo_curve():
    print("== 小曲線 y^2 = x^3 + 7 (mod 17)：和 Bitcoin 同一個公式，只是數字小到看得見 ==")
    c = Curve(17, 0, 7)
    pts = [(x, y) for x in range(17) for y in range(17) if c.on_curve((x, y))]
    print(f"曲線上共有 {len(pts)} 個點（加上無窮遠點共 {len(pts) + 1} 個）：")
    print("  " + " ".join(f"({x},{y})" for x, y in pts))
    print("\n座標圖（# 是曲線上的點；不是一條線，而是散落的格子點）：")
    for y in range(16, -1, -1):
        row = "".join(" #" if (x, y) in pts else " ." for x in range(17))
        print(f"  y={y:2d} {row}")
    print("        " + "".join(f"{x:2d}"[-2:] for x in range(17)))

    g = (15, 13)
    print(f"\n選一個起點 G = {g}，一直加自己：")
    acc, k = None, 0
    while True:
        k += 1
        acc = c.add(acc, g)
        print(f"  {k:2d}G = {acc if acc else '無窮遠點（回到 0）'}")
        if acc is None:
            break
    print(f"走 {k} 步就繞回原點：這個 G 的「階」是 {k}。順序看起來是亂跳的——")
    print("這就是單向性的來源：知道 k 算 kG 很容易，看到 kG 想猜 k 只能一個一個試。")


def demo_keys():
    print("== secp256k1：私鑰是一個隨機數 d，公鑰是點 d*G ==")
    d = secrets.randbelow(N - 1) + 1
    t0 = time.perf_counter()
    q = SECP256K1.mul(d, G)
    dt = time.perf_counter() - t0
    print(f"私鑰 d   = {d:064x}")
    print(f"公鑰 d*G = ({q[0]:064x},\n            {q[1]:064x})")
    print(f"在曲線上：{SECP256K1.on_curve(q)}；倍加法用了 {SECP256K1.last_steps} 次點運算，花 {dt * 1000:.1f} ms")
    print(f"可能的私鑰數量 n ≈ {N:.3e}（約 2^256）")

    print("\n反過來：只給公鑰，暴力猜私鑰。先用一個很小的私鑰示範可以猜到：")
    secret_small = 54321
    target = SECP256K1.mul(secret_small, G)
    t0 = time.perf_counter()
    acc, k = None, 0
    while acc != target:
        k += 1
        acc = SECP256K1.add(acc, G)
    dt = time.perf_counter() - t0
    rate = k / dt
    print(f"  試了 {k} 次找到 d = {k}，花 {dt:.2f} 秒，約每秒 {rate:,.0f} 次")
    # 最好的通用攻擊（Pollard rho）約需 sqrt(n) ≈ 2^128 步
    years = (2**128) / rate / 3.15e7
    print(f"  真的 256 位元私鑰：最好的已知通用演算法也要約 2^128 步，")
    print(f"  以這個速度約 {years:.1e} 年；就算快一兆倍也還要 {years / 1e12:.1e} 年。")


def sign(d, msg):
    """Schnorr 式簽章（概念同 BIP340，但編碼簡化）。"""
    q = SECP256K1.mul(d, G)
    k = secrets.randbelow(N - 1) + 1  # nonce：每次都必須是全新的隨機數
    r = SECP256K1.mul(k, G)
    e = hint(r[0], q[0], msg) % N
    s = (k + e * d) % N
    return r, s


def verify(q, msg, sig):
    r, s = sig
    e = hint(r[0], q[0], msg) % N
    return SECP256K1.mul(s, G) == SECP256K1.add(r, SECP256K1.mul(e, q))


def demo_sign():
    print("== 數位簽章：證明「我知道私鑰」，卻不把私鑰交出去 ==")
    d = secrets.randbelow(N - 1) + 1
    q = SECP256K1.mul(d, G)
    msg = "Alice 付 Bob 0.1 BTC"
    sig = sign(d, msg)
    print(f"訊息：{msg}")
    print(f"簽章 (R.x, s) = ({sig[0][0]:x}, {sig[1]:x})")
    print(f"用公鑰驗證原訊息：{verify(q, msg, sig)}")
    print(f"把金額改成 1.0 BTC 再驗：{verify(q, 'Alice 付 Bob 1.0 BTC', sig)}")
    print("驗證式：s*G == R + e*Q。因為 s = k + e*d，兩邊乘 G 就相等；不知道 d 就湊不出 s。")

    print("\n反例：兩筆簽章用了同一個 nonce k，會發生什麼？")
    k = secrets.randbelow(N - 1) + 1
    r = SECP256K1.mul(k, G)
    m1, m2 = "交易一", "交易二"
    e1, e2 = hint(r[0], q[0], m1) % N, hint(r[0], q[0], m2) % N
    s1, s2 = (k + e1 * d) % N, (k + e2 * d) % N
    recovered = (s1 - s2) * pow(e1 - e2, -1, N) % N
    print(f"  s1 - s2 = (e1 - e2) * d  =>  d = (s1 - s2)/(e1 - e2)")
    print(f"  算出的私鑰與真的私鑰相同：{recovered == d}")
    print("  曲線本身沒被破解，是使用方式錯了：nonce 重複就等於公開私鑰。")


def demo_pow():
    print("== 工作量證明：找一個 nonce，讓 SHA-256(SHA-256(區塊)) 開頭夠多個 0 ==")
    header = "前一塊雜湊=...|交易摘要=...|時間=..."
    for zeros in range(1, 6):
        t0 = time.perf_counter()
        nonce = 0
        while True:
            digest = hashlib.sha256(h(header, f"|難度{zeros}|", nonce)).hexdigest()
            if digest.startswith("0" * zeros):
                break
            nonce += 1
        dt = time.perf_counter() - t0
        print(f"  開頭 {zeros} 個 0：試了 {nonce + 1:>9,} 次（期望約 {16**zeros:>9,}），{dt:6.2f} 秒，雜湊 {digest[:20]}…")
    print("每多要求一個十六進位 0，平均工作量乘 16；但別人驗證只要算一次雜湊。")


def demo_chain():
    print("== 區塊鏈：每一塊都記住前一塊的雜湊 ==")
    blocks, prev = [], "0" * 16
    for txs in ["礦工獲得 50", "A 付 B 10", "B 付 C 3"]:
        digest = hashlib.sha256(h(prev, txs)).hexdigest()[:16]
        blocks.append([prev, txs, digest])
        prev = digest
    for i, (p, t, d) in enumerate(blocks):
        print(f"  區塊 {i}: 前={p} 內容={t:<10} 雜湊={d}")
    print("\n偷改區塊 1 的內容（A 付 B 10 -> A 付 B 1000）：")
    blocks[1][1] = "A 付 B 1000"
    ok = True
    for i, (p, t, d) in enumerate(blocks):
        real = hashlib.sha256(h(p, t)).hexdigest()[:16]
        link = i == 0 or p == blocks[i - 1][2]
        status = "OK" if (real == d and link) else "對不上！"
        ok &= status == "OK"
        print(f"  區塊 {i}: 重算雜湊={real} 記錄的={d} {status}")
    print("\n就算連區塊 1 的雜湊一起改掉，區塊 2 記的「前一塊雜湊」也會對不上：")
    blocks[1][2] = hashlib.sha256(h(blocks[1][0], blocks[1][1])).hexdigest()[:16]
    print(f"  區塊 1 新雜湊={blocks[1][2]}，區塊 2 記的前一塊={blocks[2][0]}，相同嗎？{blocks[1][2] == blocks[2][0]}")
    print("要讓整條鏈重新對上，得把區塊 1 之後每一塊的工作量證明全部重做，還要追過全網持續產生的新區塊。")


DEMOS = {"curve": demo_curve, "keys": demo_keys, "sign": demo_sign, "pow": demo_pow, "chain": demo_chain}

if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    which = sys.argv[1] if len(sys.argv) > 1 else "all"
    for name, fn in DEMOS.items():
        if which in (name, "all"):
            fn()
            print()
