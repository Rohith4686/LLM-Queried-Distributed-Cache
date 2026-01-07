#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "protocol.h"

static const uint8_t MAGIC[3] = { 0xCA, 0xC4, 0xE0 };

/* Read exactly n bytes from fd, handling partial reads */
static int read_exact(int fd, void *buf, size_t n) {
    size_t total = 0;
    char *p = (char *)buf;
    while (total < n) {
        ssize_t r = read(fd, p + total, n - total);
        if (r <= 0) return (r == 0) ? 0 : -1;
        total += (size_t)r;
    }
    return (int)total;
}

/* Write exactly n bytes, handling partial writes */
static int write_exact(int fd, const void *buf, size_t n) {
    size_t total = 0;
    const char *p = (const char *)buf;
    while (total < n) {
        ssize_t w = write(fd, p + total, n - total);
        if (w <= 0) return -1;
        total += (size_t)w;
    }
    return (int)total;
}

/*
 * Read one frame from fd.
 * buf must be at least buf_size bytes; key and value in frame point into buf.
 * Returns number of bytes read on success, 0 on EOF, -1 on error.
 */
int proto_read_frame(int fd, proto_frame_t *out, char *buf, size_t buf_size) {
    proto_header_t hdr;

    int rc = read_exact(fd, &hdr, PROTO_HEADER_SIZE);
    if (rc <= 0) return rc;

    /* Validate magic */
    if (memcmp(hdr.magic, MAGIC, 3) != 0) {
        fprintf(stderr, "proto: bad magic bytes\n");
        return -1;
    }
    if (hdr.version != PROTO_VERSION) {
        fprintf(stderr, "proto: unsupported version %d\n", hdr.version);
        return -1;
    }

    /* Network to host byte order */
    hdr.request_id = ntohl(hdr.request_id);
    hdr.key_len    = ntohs(hdr.key_len);
    hdr.val_len    = ntohl(hdr.val_len);

    size_t payload  = (size_t)hdr.key_len + (size_t)hdr.val_len;
    if (payload >= buf_size) {
        fprintf(stderr, "proto: payload %zu exceeds buffer %zu\n", payload, buf_size);
        return -1;
    }

    if (payload > 0) {
        rc = read_exact(fd, buf, payload);
        if (rc <= 0) return rc;
    }
    buf[payload] = '\0';

    out->header = hdr;
    out->key    = (hdr.key_len > 0) ? buf : NULL;
    out->value  = (hdr.val_len > 0) ? buf + hdr.key_len : NULL;

    return (int)(PROTO_HEADER_SIZE + payload);
}

/*
 * Write one frame to fd.
 */
int proto_write_frame(int fd, uint8_t op, uint8_t status,
                      uint32_t req_id,
                      const char *key,   uint16_t key_len,
                      const char *value, uint32_t val_len) {
    proto_header_t hdr;
    memcpy(hdr.magic, MAGIC, 3);
    hdr.version    = PROTO_VERSION;
    hdr.request_id = htonl(req_id);
    hdr.op_code    = op;
    hdr.status     = status;
    hdr.key_len    = htons(key_len);
    hdr.val_len    = htonl(val_len);

    if (write_exact(fd, &hdr, PROTO_HEADER_SIZE) < 0) return -1;
    if (key   && key_len  > 0 && write_exact(fd, key,   key_len)  < 0) return -1;
    if (value && val_len  > 0 && write_exact(fd, value, val_len)  < 0) return -1;

    return 0;
}
