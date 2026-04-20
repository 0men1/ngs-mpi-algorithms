#!/bin/bash

# Experiment 2: 51 nodes with 2 MPI ranks (50 + 1 initial node)
# Compare smaller graph with fewer partitions
#
# Prerequisites:
# - Ensure MPI runtime is built at ../mpi_runtime/build/ngs_mpi
# - Graphs should exist in ./graphs/ directory

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GRAPH="${SCRIPT_DIR}/graphs/exp2/exp2_50nodes_graph.json"
PART="${SCRIPT_DIR}/graphs/exp2/exp2_50nodes_part.json"
MPIEXEC="${SCRIPT_DIR}/../build/ngs_mpi"

echo "=========================================="
echo "Experiment 2: 50 Nodes / 2 Ranks"
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
echo "--- Running Leader Election (2 ranks) ---"
mpirun -n 2 "$MPIEXEC" --graph "$GRAPH" --part "$PART" --algo leader --rounds 50

echo ""
echo "--- Running Dijkstra (2 ranks) ---"
mpirun -n 2 "$MPIEXEC" --graph "$GRAPH" --part "$PART" --algo dijkstra --source 0

echo ""
echo "=========================================="
echo "Experiment 2 Complete"
echo "=========================================="
