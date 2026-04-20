#!/bin/bash

# Experiment 3: 200 nodes with 10 MPI ranks
# Test large graph with many partitions
#
# Prerequisites:
# - Ensure MPI runtime is built at ../build/ngs_mpi
# - Graphs should exist in ./graphs/ directory

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GRAPH="${SCRIPT_DIR}/graphs/exp3/exp3_200nodes_graph.json"
PART="${SCRIPT_DIR}/graphs/exp3/exp3_200nodes_part.json"
MPIEXEC="${SCRIPT_DIR}/../build/ngs_mpi"

echo "=========================================="
echo "Experiment 3: 200 Nodes / 10 Ranks"
echo "=========================================="

if [ ! -f "$GRAPH" ]; then
    echo "Error: Graph file not found: $GRAPH"
    echo "Please run graph generation first"
    exit 1
fi

if [ ! -f "$MPIEXEC" ]; then
    echo "Error: MPI executable not found: $MPIEXEC"
    echo "Please build the MPI runtime first"
    exit 1
fi

echo "Using graph: $GRAPH"
echo "Using partition: $PART"

echo ""
echo "--- Running Leader Election (10 ranks) ---"
mpirun -n 10 "$MPIEXEC" --graph "$GRAPH" --part "$PART" --algo leader --rounds 40

echo ""
echo "--- Running Dijkstra (10 ranks) ---"
mpirun -n 10 "$MPIEXEC" --graph "$GRAPH" --part "$PART" --algo dijkstra --source 0

echo ""
echo "=========================================="
echo "Experiment 3 Complete"
echo "=========================================="
