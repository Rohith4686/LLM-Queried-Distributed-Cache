"""
Python client for the distributed cache.
Usage:
    python3 client.py set <key> <value> [--port 7370]
    python3 client.py get <key>
    python3 client.py delete <key>
    python3 client.py stats
"""

import socket
import struct
import sys
import argparse

HOST    = "localhost"
PORT    = 7370

MAGIC   = bytes([0xCA, 0xC4, 0xE0])
VERSION = 0x01
OP_SET, OP_GET, OP_DELETE, OP_STATS = 0x01, 0x02, 0x03, 0x04
OP_ACK = 0xFF
STATUS_OK, STATUS_NOT_FOUND, STATUS_ERROR = 0x00, 0x01, 0x02

_req = 0
def next_id():
    global _req; _req += 1; return _req

def frame(op, key=b"", value=b""):
    k = key   if isinstance(key,   bytes) else key.encode()
    v = value if isinstance(value, bytes) else value.encode()
    hdr = struct.pack("!3sBIBBHI", MAGIC, VERSION, next_id(), op, 0, len(k), len(v))
    return hdr + k + v

def parse(data):
    if len(data) < 16: raise ValueError("short response")
    _, _, _, _, status, key_len, val_len = struct.unpack_from("!3sBIBBHI", data)
    key   = data[16:16+key_len]
    value = data[16+key_len:16+key_len+val_len]
    return status, key, value

def send(op, key="", value="", port=PORT):
    with socket.create_connection((HOST, port), timeout=3) as s:
        s.sendall(frame(op, key, value))
        return parse(s.recv(4096))

def main():
    ap = argparse.ArgumentParser(description="Cache CLI")
    ap.add_argument("op",    choices=["set","get","delete","stats"])
    ap.add_argument("key",   nargs="?", default="")
    ap.add_argument("value", nargs="?", default="")
    ap.add_argument("--port", type=int, default=PORT)
    args = ap.parse_args()

    ops = {"set": OP_SET, "get": OP_GET, "delete": OP_DELETE, "stats": OP_STATS}
    status, _, value = send(ops[args.op], args.key, args.value, args.port)

    if status == STATUS_OK:
        print(value.decode() if value else "OK")
    elif status == STATUS_NOT_FOUND:
        print("(not found)")
    else:
        print(f"error: status={status}")
        sys.exit(1)

if __name__ == "__main__":
    main()
