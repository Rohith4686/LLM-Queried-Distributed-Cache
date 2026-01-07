"""
Basic smoke tests for the distributed cache.
Run after starting a server: ./bin/cache-server --port 7370 --node-id 0
"""

import socket
import struct
import time
import sys

HOST = "localhost"
PORT = 7370

MAGIC   = bytes([0xCA, 0xC4, 0xE0])
VERSION = 0x01

OP_SET    = 0x01
OP_GET    = 0x02
OP_DELETE = 0x03
OP_STATS  = 0x04
OP_ACK    = 0xFF

STATUS_OK        = 0x00
STATUS_NOT_FOUND = 0x01
STATUS_ERROR     = 0x02

_req_id = 0

def next_req_id():
    global _req_id
    _req_id += 1
    return _req_id

def build_frame(op, key=b"", value=b"", req_id=None):
    if req_id is None:
        req_id = next_req_id()
    key   = key   if isinstance(key,   bytes) else key.encode()
    value = value if isinstance(value, bytes) else value.encode()
    header = struct.pack(
        "!3sBIBBHI",
        MAGIC, VERSION, req_id,
        op, 0x00,
        len(key), len(value)
    )
    return header + key + value

def parse_response(data):
    """Parse the fixed header, return (status, key_bytes, value_bytes)."""
    hdr_size = 12  # 3+1+4+1+1+2 = 12? let's recalc:
    # magic(3) version(1) req_id(4) op(1) status(1) key_len(2) val_len(4) = 16
    hdr_size = 16
    if len(data) < hdr_size:
        raise ValueError(f"response too short: {len(data)} bytes")
    magic, version, req_id, op, status, key_len, val_len = struct.unpack_from(
        "!3sBIBBHI", data, 0
    )
    key   = data[hdr_size : hdr_size + key_len]
    value = data[hdr_size + key_len : hdr_size + key_len + val_len]
    return status, key, value

def send_recv(sock, frame):
    sock.sendall(frame)
    return sock.recv(4096)

def test_set_get(sock):
    print("  test: SET key → GET key → expect same value")
    frame = build_frame(OP_SET, key="hello", value="world")
    resp  = send_recv(sock, frame)
    status, _, _ = parse_response(resp)
    assert status == STATUS_OK, f"SET failed with status {status}"

    frame = build_frame(OP_GET, key="hello")
    resp  = send_recv(sock, frame)
    status, _, value = parse_response(resp)
    assert status == STATUS_OK,           f"GET status {status}"
    assert value == b"world",             f"GET value mismatch: {value}"
    print("    PASS")

def test_delete(sock):
    print("  test: SET key → DELETE key → GET key → expect NOT_FOUND")
    send_recv(sock, build_frame(OP_SET, key="tmp", value="data"))
    send_recv(sock, build_frame(OP_DELETE, key="tmp"))
    resp = send_recv(sock, build_frame(OP_GET, key="tmp"))
    status, _, _ = parse_response(resp)
    assert status == STATUS_NOT_FOUND, f"expected NOT_FOUND, got {status}"
    print("    PASS")

def test_missing_key(sock):
    print("  test: GET nonexistent key → expect NOT_FOUND")
    resp = send_recv(sock, build_frame(OP_GET, key="__no_such_key__"))
    status, _, _ = parse_response(resp)
    assert status == STATUS_NOT_FOUND, f"expected NOT_FOUND, got {status}"
    print("    PASS")

def test_stats(sock):
    print("  test: STATS op returns hit/miss/key counts")
    resp = send_recv(sock, build_frame(OP_STATS))
    status, _, value = parse_response(resp)
    assert status == STATUS_OK, f"STATS failed: {status}"
    assert b"hits=" in value, f"STATS missing hits: {value}"
    print(f"    PASS  ({value.decode()})")

def main():
    print(f"connecting to {HOST}:{PORT} ...")
    try:
        sock = socket.create_connection((HOST, PORT), timeout=3)
    except ConnectionRefusedError:
        print(f"ERROR: no server running on {PORT}. Start it first:")
        print(f"  ./bin/cache-server --port {PORT} --node-id 0")
        sys.exit(1)

    print("running smoke tests...\n")
    try:
        test_set_get(sock)
        test_delete(sock)
        test_missing_key(sock)
        test_stats(sock)
    finally:
        sock.close()

    print("\nall tests passed.")

if __name__ == "__main__":
    main()
