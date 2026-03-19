#pragma once
#include <stdint.h>
#include <string.h>
#include "esp_rom_crc.h"   // ROM CRC32 (Ethernet poly)  [6](https://sourcevu.sysprogs.com/espressif/esp-idf/symbols/esp_rom_crc32_le)

#ifdef __cplusplus
extern "C" {
#endif

// --------- 常量/标志 与 PC 端一致 ----------
enum {
    IMU_MAGIC_TLV = 0x49554D54u,   // 'IUMT'
    IMU_PROTO_VER = 1,
};
enum {
    FLAG_ACC_MPS2 = 1u << 0,       // 0=g,1=m/s^2
    FLAG_GYR_RAD  = 1u << 1,       // 0=deg/s,1=rad/s
};
enum {
    TLV_ACCEL  = 0x01,             // float[3]
    TLV_GYRO   = 0x02,             // float[3]
    TLV_TEMP   = 0x03,             // float[1]
    TLV_MAG    = 0x04,             // float[3] (可选)
    TLV_LINACC = 0x05,             // float[3] (可选)
    TLV_QUAT   = 0x06,             // float[4] (可选)
    TLV_TEXT   = 0x7F,             // utf-8 (可选)
};

#pragma pack(push,1)
typedef struct {
    uint32_t magic;        // IMU_MAGIC_TLV
    uint8_t  version;      // IMU_PROTO_VER
    uint8_t  flags;        // 单位标志
    uint8_t  r0, r1;       // 保留
    uint32_t device_id;
    uint32_t seq;          // 序列号
    uint64_t t_us;         // 微秒时间戳
    uint16_t hdr_len;      // 必须为 28
    uint16_t payload_len;  // TLV 总长度
} imu_msg_header_t;        // 28 bytes
#pragma pack(pop)

_Static_assert(sizeof(imu_msg_header_t)==28, "imu_msg_header_t must be 28 bytes");

#pragma pack(push,1)
typedef struct {
    uint8_t  type;
    uint16_t len;
    // followed by value[len]
} imu_tlv_t;               // 3 bytes
#pragma pack(pop)

_Static_assert(sizeof(imu_tlv_t)==3, "imu_tlv_t must be 3 bytes");

typedef struct {
    uint8_t* buf;      // 外部提供或内部管理的缓存
    uint16_t cap;      // 缓存容量
    uint16_t size;     // 当前已用
    uint16_t pay;      // 当前 payload 已追加字节
} imu_pkt_builder_t;

// --- 小端写工具 ---
static inline void le_store16(void* p, uint16_t v){ ((uint8_t*)p)[0]=v&0xFF; ((uint8_t*)p)[1]=v>>8; }
static inline void le_store32(void* p, uint32_t v){ uint8_t* b=(uint8_t*)p; b[0]=v; b[1]=v>>8; b[2]=v>>16; b[3]=v>>24; }
static inline void le_store64(void* p, uint64_t v){ uint8_t* b=(uint8_t*)p; b[0]=v; b[1]=v>>8; b[2]=v>>16; b[3]=v>>24; b[4]=v>>32; b[5]=v>>40; b[6]=v>>48; b[7]=v>>56; }

// --- 初始化构包器：buffer 由调用者提供 ---
static inline void imu_pkt_reset(imu_pkt_builder_t* pb, void* buffer, uint16_t capacity){
    pb->buf=(uint8_t*)buffer; pb->cap=capacity; pb->size=0; pb->pay=0;
    // 预留 Header
    if(capacity>=sizeof(imu_msg_header_t)){
        pb->size = sizeof(imu_msg_header_t);
        memset(pb->buf, 0, pb->size);
    }
}

static inline int imu_pkt_begin(imu_pkt_builder_t* pb, uint32_t seq, uint64_t t_us, uint8_t flags,uint32_t device){
    if(pb->cap < sizeof(imu_msg_header_t)) return -1;
    imu_msg_header_t h={0};
    h.magic = IMU_MAGIC_TLV;
    h.version = IMU_PROTO_VER;
    h.flags = flags;
    h.seq = seq;
    h.t_us = t_us;
    h.hdr_len = sizeof(imu_msg_header_t);
    h.payload_len = 0;

    le_store32(&pb->buf[0],  h.magic);
    pb->buf[4] = h.version;
    pb->buf[5] = h.flags;
    pb->buf[6] = 0; pb->buf[7] = 0;
    le_store32(&pb->buf[8],  device);
    le_store32(&pb->buf[12],  h.seq);
    le_store64(&pb->buf[16], h.t_us);
    le_store16(&pb->buf[24], h.hdr_len);
    le_store16(&pb->buf[26], h.payload_len);
    pb->pay = 0;
    pb->size = sizeof(imu_msg_header_t);
    return 0;
}

static inline int imu_pkt_add_tlv(imu_pkt_builder_t* pb, uint8_t type, const void* data, uint16_t len){
    uint32_t need = sizeof(imu_tlv_t) + len;
    if(pb->size + need + 4 > pb->cap) return -1; // +4 for CRC
    uint8_t* p = pb->buf + pb->size;
    p[0] = type;
    le_store16(&p[1], len);
    if(len) memcpy(p+sizeof(imu_tlv_t), data, len);
    pb->size += need;
    pb->pay  += need;
    return 0;
}

static inline int imu_pkt_finalize(imu_pkt_builder_t* pb){
    if(pb->size + 4 > pb->cap) return -1;
    // 回填 payload_len
    le_store16(&pb->buf[26], pb->pay);

    const uint16_t hdr_len = (uint16_t)sizeof(imu_msg_header_t);  // 线协议头固定 24B
    const uint32_t crc = esp_rom_crc32_le((uint32_t)~0xFFFFFFFFu,
                                          pb->buf,
                                          (size_t)hdr_len + (size_t)pb->pay);

    // 4) 写入包尾 CRC（小端）
    le_store32(pb->buf + pb->size, crc);
    pb->size += 4;

    return pb->size;
}

#ifdef __cplusplus
}
#endif