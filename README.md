# LLM-Queried Distributed Cache

A distributed in-memory cache written in C, queryable via natural language through an LLM API.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                   Client Layer                       │
│   Python CLI / NL Query Interface (LLM API)         │
└───────────────────┬─────────────────────────────────┘
                    │ TCP (custom binary protocol)
        ┌───────────┼───────────┐
        ▼           ▼           ▼
   ┌─────────┐ ┌─────────┐ ┌─────────┐
   │ Node 0  │ │ Node 1  │ │ Node 2  │
   │(leader) │◄──────────►│         │
   │         │ │         │ │         │
   │ cache   │ │ cache   │ │ cache   │
   └─────────┘ └─────────┘ └─────────┘
   Write replication + leader election (bully algorithm)
```

## Features

- **Distributed**: 3-node cluster with write replication
- **Fault tolerant**: Leader election via bully algorithm
- **Fast**: Sub-10ms read latency under concurrent load
- **Natural language interface**: Query the cache in plain English via LLM API

## Protocol

Custom binary TCP protocol with the following operations:

| Op Code | Name   | Description              |
|---------|--------|--------------------------|
| 0x01    | SET    | Store a key-value pair   |
| 0x02    | GET    | Retrieve a value by key  |
| 0x03    | DELETE | Remove a key             |
| 0x04    | STATS  | Get node statistics      |
| 0x05    | NODES  | List cluster nodes       |
| 0xFF    | ACK    | Acknowledgement/response |

## Build

```bash
make        # build the server
make clean  # clean build artifacts
make test   # run tests
make run    # build and start node 0
```

## Project Structure

```
.
├── include/        # header files
├── src/            # C source files
│   ├── main.c      # server entry point
│   ├── cache.c     # in-memory hash map
│   ├── server.c    # TCP server + protocol
│   ├── replication.c
│   └── election.c
├── client/         # Python client + LLM interface
│   ├── client.py
│   └── llm_query.py
├── tests/          # test suite
├── scripts/        # helper scripts (start cluster, etc.)
├── docs/           # architecture notes
└── Makefile
```

## Quick Start

```bash
# Terminal 1 — start node 0 (leader)
./bin/cache-server --port 7370 --node-id 0

# Terminal 2 — start node 1
./bin/cache-server --port 7371 --node-id 1 --peers localhost:7370

# Terminal 3 — start node 2
./bin/cache-server --port 7372 --node-id 2 --peers localhost:7370,localhost:7371

# Query via Python client
python3 client/client.py set mykey "hello world"
python3 client/client.py get mykey

# Query via natural language
python3 client/llm_query.py "store the string hello under the key greeting"
```

## Configuration

Nodes are configured via CLI flags or `config/nodes.conf`:

```
node_id=0
port=7370
peers=localhost:7371,localhost:7372
```

## Performance

| Metric              | Value       |
|---------------------|-------------|
| Read latency (p99)  | < 10ms      |
| Concurrent clients  | 100+        |
| Nodes               | 3           |
| Replication         | Synchronous |
