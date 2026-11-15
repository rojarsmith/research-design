/*
 * budget.c：資料率、緩衝區與時間預算計算器。
 *
 * 建置：gcc -std=c11 -Wall -Wextra -Werror -O2 -o budget budget.c -lm
 *       （-lm 是 Linux 上的數學函式庫；MinGW 加上也無妨）
 * 用法：
 *   ./budget                                  使用第一課的預設值
 *   ./budget --rate=100 --frame=12 --baud=115200 --bits=10 --buffer=1024 --stall-ms=50
 *   ./budget --self-test                      用第一課的答案檢查計算
 *
 * 模型是連續平均速率：不考慮封包突發、起始佔用與傳送端的額外間隔。
 * 結果是下限或上限的估計，設計時還要加上餘裕，並以實測驗證。
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    double rate;      /* 每秒幾個封包 */
    double frame;     /* 每個封包幾 bytes */
    double baud;      /* 每秒幾個符號；本例每符號 1 bit */
    double bits;      /* 線路上每 byte 需要幾 bits，8N1 是 10 */
    double buffer;    /* 傳送緩衝區 bytes */
    double stall_ms;  /* 傳送停住多久 */
} input_t;

typedef struct {
    double source_Bps, link_Bps, utilization;
    double growth_Bps;     /* > 0 表示持續超載 */
    double fill_s;         /* 從空到滿的時間；不超載時為 INFINITY */
    double drop_fps;       /* 超載時穩定狀態下每秒丟幾個封包 */
    double drain_s;        /* 不超載時，滿的緩衝區要多久清空 */
    double stall_bytes;    /* 停住期間新產生的資料 */
    double max_fps;        /* 線路能承載的最大封包率 */
    double byte_us, frame_us;
} result_t;

static result_t compute(const input_t *in)
{
    result_t r;
    r.source_Bps = in->rate * in->frame;
    r.link_Bps = in->baud / in->bits;
    r.utilization = r.source_Bps / r.link_Bps;
    r.growth_Bps = r.source_Bps - r.link_Bps;
    r.fill_s = r.growth_Bps > 0 ? in->buffer / r.growth_Bps : INFINITY;
    r.drop_fps = r.growth_Bps > 0 ? r.growth_Bps / in->frame : 0.0;
    r.drain_s = r.growth_Bps < 0 ? in->buffer / -r.growth_Bps : INFINITY;
    r.stall_bytes = r.source_Bps * in->stall_ms / 1000.0;
    r.max_fps = floor(r.link_Bps / in->frame);
    r.byte_us = in->bits / in->baud * 1e6;
    r.frame_us = r.byte_us * in->frame;
    return r;
}

static void print_result(const input_t *in, const result_t *r)
{
    printf("input: %.0f frames/s x %.0f bytes, %.0f baud, %.0f bits/byte, buffer %.0f bytes, stall %.0f ms\n\n",
           in->rate, in->frame, in->baud, in->bits, in->buffer, in->stall_ms);
    printf("source data rate        %10.1f bytes/s\n", r->source_Bps);
    printf("link capacity           %10.1f bytes/s\n", r->link_Bps);
    printf("link utilization        %10.1f %%\n", r->utilization * 100.0);
    printf("max frames on the link  %10.0f frames/s\n", r->max_fps);
    printf("time per byte on wire   %10.1f us   (rough RX service deadline with a 1-byte data register)\n",
           r->byte_us);
    printf("time per frame on wire  %10.1f us\n", r->frame_us);
    if (r->growth_Bps > 0) {
        printf("backlog growth          %10.1f bytes/s  -> sustained overload\n", r->growth_Bps);
        printf("empty buffer fills in   %10.2f s\n", r->fill_s);
        printf("then drops about        %10.1f frames/s (a bigger buffer only delays this)\n",
               r->drop_fps);
    } else {
        printf("spare capacity          %10.1f bytes/s\n", -r->growth_Bps);
        printf("full buffer drains in   %10.2f s\n", r->drain_s);
    }
    printf("data produced in stall  %10.1f bytes  (%s the buffer, before existing backlog and margin)\n",
           r->stall_bytes, r->stall_bytes <= in->buffer ? "fits in" : "exceeds");
}

static int near(double a, double b, double tol) { return fabs(a - b) <= tol; }

static int self_test(void)
{
    input_t in = {1000, 12, 115200, 10, 1024, 50};
    result_t r = compute(&in);
    print_result(&in, &r);
    int ok = near(r.source_Bps, 12000, 1e-9) && near(r.link_Bps, 11520, 1e-9) &&
             near(r.growth_Bps, 480, 1e-9) && near(r.fill_s, 2.133, 0.001) &&
             near(r.stall_bytes, 600, 1e-9) && near(r.byte_us, 86.8, 0.05) &&
             near(r.max_fps, 960, 1e-9) && near(r.drop_fps, 40, 1e-9);
    input_t slow = {100, 12, 115200, 10, 1024, 50};
    result_t s = compute(&slow);
    ok = ok && s.growth_Bps < 0 && isinf(s.fill_s) && near(s.stall_bytes, 60, 1e-9);
    printf("\nself-test %s\n", ok ? "passed" : "FAILED");
    return ok ? 0 : 1;
}

static int parse(const char *arg, const char *name, double *out)
{
    size_t n = strlen(name);
    if (strncmp(arg, name, n) != 0 || arg[n] != '=') return 0;
    char *end;
    double v = strtod(arg + n + 1, &end);
    if (*end != '\0' || !(v > 0)) {
        fprintf(stderr, "invalid value: %s\n", arg);
        exit(2);
    }
    *out = v;
    return 1;
}

int main(int argc, char **argv)
{
    input_t in = {1000, 12, 115200, 10, 1024, 50};
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--self-test") == 0) return self_test();
        if (parse(argv[i], "--rate", &in.rate) || parse(argv[i], "--frame", &in.frame) ||
            parse(argv[i], "--baud", &in.baud) || parse(argv[i], "--bits", &in.bits) ||
            parse(argv[i], "--buffer", &in.buffer) || parse(argv[i], "--stall-ms", &in.stall_ms))
            continue;
        fprintf(stderr,
                "usage: budget [--rate=N] [--frame=N] [--baud=N] [--bits=N] [--buffer=N] [--stall-ms=N] | --self-test\n");
        return 2;
    }
    result_t r = compute(&in);
    print_result(&in, &r);
    return 0;
}
