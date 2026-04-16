#!/bin/bash

# Experiment 2: 50 nodes with 2 MPI ranks
# Compare smaller graph with fewer partitions

echo "=========================================="
echo "Experiment 2: 50 Nodes / 2 Ranks"
echo "=========================================="

cd /Users/homenhoma/dev_projects/class/CS453/mpi_runtime

GRAPH="../outputs/exp2_50nodes.json"
PART="../outputs/exp2_50nodes_part.json"

echo ""
echo "--- Running Leader Election (2 ranks) ---"
mpirun -n 2 ./build/ngs_mpi --graph $GRAPH --part $PART --algo leader --rounds 50

echo ""
echo "--- Running Dijkstra (2 ranks) ---"
mpirun -n 2 ./build/ngs_mpi --graph $GRAPH --part $PART --algo dijkstra --source 0

echo ""
echo "=========================================="
echo "Experiment 2 Complete"
echo "=========================================="