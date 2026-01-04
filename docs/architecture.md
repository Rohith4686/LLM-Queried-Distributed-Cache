# Architecture Notes

## Cache Layer (C)

- Hash map with `CACHE_TABLE_SIZE` (1024) buckets
- djb2 hash function
- Per-bucket `pthread_mutex_t` for fine-grained locking
- Separate read path (lock bucket → lookup → copy → unlock) to minimize contention
- Lazy TTL expiry: expired keys detected on GET, cleaned by background reaper thread (TODO)

## TCP Protocol

Binary framing over raw TCP sockets.

**Header** (16 bytes, big-endian):
```
[magic: 3B][version: 1B][request_id: 4B][op_code: 1B][status: 1B][key_len: 2B][val_len: 4B]
```

**Payload**: `key_len` bytes of key, then `val_len` bytes of value (either can be zero).

## Distribution (TODO)

- 3 nodes, each running `cache-server` on a different port
- Leader election: bully algorithm — highest node ID wins
- Write replication: leader fans SET/DELETE to all followers before ACKing client
- Heartbeat: peers ping leader every 1s; if no response in 3s, trigger election

## LLM Interface (TODO)

- Python script wraps the cache client
- System prompt defines available ops and JSON schema for responses
- LLM returns structured JSON: `{"op": "SET", "key": "...", "value": "...", "ttl": 0}`
- Python parses + executes, returns natural-language result to user
