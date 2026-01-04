#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/* Magic bytes to validate frames */
#define PROTO_MAGIC      0xCAC4E0
#define PROTO_VERSION    0x01

/* Operation codes */
#define OP_SET    0x01
#define OP_GET    0x02
#define OP_DELETE 0x03
#define OP_STATS  0x04
#define OP_NODES  0x05
#define OP_ACK    0xFF

/* Status codes (in ACK frames) */
#define STATUS_OK        0x00
#define STATUS_NOT_FOUND 0x01
#define STATUS_ERROR     0x02
#define STATUS_REDIRECT  0x03   /* client should retry on another node */

/*
 * Wire frame layout (big-endian):
 *
 *  0        1        2        3
 *  ┌────────┬────────┬────────┬────────┐
 *  │  magic (3 bytes)         │version │
 *  ├────────┴────────┴────────┴────────┤
 *  │           request_id (4 bytes)    │
 *  ├───────────────────────────────────┤
 *  │  op_code (1)  │  status (1)       │
 *  │  key_len (2)  │  val_len (4)      │
 *  ├───────────────────────────────────┤
 *  │  key   (key_len bytes)            │
 *  ├───────────────────────────────────┤
 *  │  value (val_len bytes)            │
 *  └───────────────────────────────────┘
 */

/* Fixed-size header that precedes every frame */
#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[3];
    uint8_t  version;
    uint32_t request_id;
    uint8_t  op_code;
    uint8_t  status;
    uint16_t key_len;
    uint32_t val_len;
} proto_header_t;
#pragma pack(pop)

#define PROTO_HEADER_SIZE sizeof(proto_header_t)

/* Full parsed frame (header + payload pointers) */
typedef struct {
    proto_header_t  header;
    char           *key;    /* points into read buffer — not owned */
    char           *value;  /* points into read buffer — not owned */
} proto_frame_t;

/* Encode/decode helpers */
int proto_read_frame(int fd, proto_frame_t *out, char *buf, size_t buf_size);
int proto_write_frame(int fd, uint8_t op, uint8_t status,
                      uint32_t req_id,
                      const char *key,   uint16_t key_len,
                      const char *value, uint32_t val_len);

#endif /* PROTOCOL_H */
