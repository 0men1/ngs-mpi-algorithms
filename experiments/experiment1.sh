#!/bin/bash

# Experiment 1: 100 nodes with 5 MPI ranks
# Compare scaling with more partitions
#
# Prerequisites:
# - Ensure MPI runtime is built at ../build/ngs_mpi
# - Graphs should exist in ./graphs/ directory

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GRAPH="${SCRIPT_DIR}/graphs/exp1/exp1_100nodes_graph.json"
PART="${SCRIPT_DIR}/graphs/exp1/exp1_100nodes_part.json"
MPIEXEC="${SCRIPT_DIR}/../build/ngs_mpi"

echo "=========================================="
echo "Experiment 1: 100 Nodes / 5 Ranks"
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
echo "--- Running Leader Election (5 ranks) ---"
mpirun -n 5 "$MPIEXEC" --graph "$GRAPH" --part "$PART" --algo leader --rounds 30

echo ""
echo "--- Running Dijkstra (5 ranks) ---"
mpirun -n 5 "$MPIEXEC" --graph "$GRAPH" --part "$PART" --algo dijkstra --source 0

echo ""
echo "=========================================="
echo "Experiment 1 Complete"
echo "=========================================="
