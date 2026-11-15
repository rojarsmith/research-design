/*
 * frame.h：資料採集器的二進位封包格式，裝置端與電腦端共用。
 *
 *   位移   0      1     2      3 .. 3+LEN-1   3+LEN    4+LEN
 *          SYNC   LEN   TYPE   PAYLOAD        CRC 低   CRC 高
 *
 *   SYNC = 0xA5；LEN = payload 長度（0..FRAME_MAX_PAYLOAD）；
 *   CRC = CRC-16/CCITT-FALSE（多項式 0x1021、初值 0xFFFF），涵蓋 LEN、TYPE、PAYLOAD。
 *   多位元組欄位一律 little-endian，由程式逐位元組組裝，不依賴 struct 排列。
 *
 * 取樣封包的 payload 是 7 bytes，所以整個封包是 12 bytes，與第一課的計算相同：
 *   seq u16、tick_ms u16（毫秒計數的低 16 位）、value i16、status u8
 */
#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>

#define FRAME_SYNC 0xA5u
#define FRAME_MAX_PAYLOAD 32u
#define FRAME_OVERHEAD 5u /* SYNC + LEN + TYPE + CRC(2) */
#define FRAME_MAX_SIZE (FRAME_OVERHEAD + FRAME_MAX_PAYLOAD)

/* 裝置 → 電腦 */
#define TYPE_SAMPLE 0x01u
#define TYPE_ACK 0x80u    /* payload：命令 type、結果碼 */
#define TYPE_STATUS 0x81u /* payload：period、dropped、rx_errors、sensor_errors、missed，各 u16 */
/* 電腦 → 裝置 */
#define TYPE_SET_PERIOD 0x10u /* payload：period_ms u16，有效範圍 1..1000 */
#define TYPE_GET_STATUS 0x11u /* payload：無 */

#define SAMPLE_PAYLOAD 7u
#define SAMPLE_FRAME_SIZE (FRAME_OVERHEAD + SAMPLE_PAYLOAD)

/* 取樣封包 status 位元 */
#define STATUS_OVERFLOW 0x01u       /* 上一個送出的封包之後，有封包因 TX 緩衝區滿而被丟棄 */
#define STATUS_SENSOR_TIMEOUT 0x02u /* 感測器逾時，value 無效 */

/* ACK 結果碼 */
#define RESULT_OK 0u
#define RESULT_BAD_LENGTH 1u
#define RESULT_OUT_OF_RANGE 2u
#define RESULT_UNKNOWN_TYPE 3u

typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t payload[FRAME_MAX_PAYLOAD];
} frame_t;

uint16_t crc16_ccitt_false(const uint8_t *data, uint32_t n);

/* 把封包寫進 out（至少 FRAME_MAX_SIZE bytes），回傳總長度；len 超過上限回傳 0 */
uint32_t frame_encode(uint8_t type, const uint8_t *payload, uint8_t len, uint8_t *out);

static inline void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static inline uint16_t get_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

/* ---- 逐位元組解析器 ---- */
typedef void (*frame_handler_t)(void *ctx, const frame_t *f);

typedef struct {
    int state;
    uint8_t raw[FRAME_MAX_SIZE]; /* 從 SYNC 開始收到的原始位元組，出錯時用來重新找同步 */
    uint32_t n;
    uint8_t replay[2u * FRAME_MAX_SIZE]; /* 等待重新掃描的位元組 */
    uint32_t replay_n;
    uint32_t last_byte_ms;
    uint32_t timeout_ms; /* 封包進行中，位元組間隔超過這個值就放棄 */
    frame_handler_t on_frame;
    void *ctx;
    /* 統計：每一種錯誤分開計數，才能判斷是雜訊、斷線還是程式錯 */
    uint32_t frames_ok, err_crc, err_len, err_timeout, bytes_skipped;
} parser_t;

void parser_init(parser_t *p, uint32_t timeout_ms, frame_handler_t on_frame, void *ctx);
void parser_feed(parser_t *p, uint8_t byte, uint32_t now_ms);
uint32_t parser_errors(const parser_t *p);

#endif
