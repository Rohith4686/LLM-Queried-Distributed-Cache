#!/usr/bin/env bash
# Start a 3-node local cluster for development.
# Run from the repo root: ./scripts/start_cluster.sh

set -e
BIN=./bin/cache-server

if [ ! -f "$BIN" ]; then
    echo "Binary not found. Running make..."
    make
fi

echo "Starting node 0 (leader candidate) on port 7370..."
$BIN --node-id 0 --port 7370 --peers localhost:7371,localhost:7372 &
NODE0_PID=$!

sleep 0.3
echo "Starting node 1 on port 7371..."
$BIN --node-id 1 --port 7371 --peers localhost:7370,localhost:7372 &
NODE1_PID=$!

sleep 0.3
echo "Starting node 2 on port 7372..."
$BIN --node-id 2 --port 7372 --peers localhost:7370,localhost:7371 &
NODE2_PID=$!

echo ""
echo "Cluster running. PIDs: $NODE0_PID $NODE1_PID $NODE2_PID"
echo "Stop with: kill $NODE0_PID $NODE1_PID $NODE2_PID"
echo "Or press Ctrl-C to stop all."

trap "kill $NODE0_PID $NODE1_PID $NODE2_PID 2>/dev/null; exit" SIGINT SIGTERM
wait
