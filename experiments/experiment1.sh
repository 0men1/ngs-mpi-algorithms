#!/bin/bash

# Experiment 1: 100 nodes with 5 MPI ranks
# Compare scaling with more partitions

echo "=========================================="
echo "Experiment 1: 100 Nodes / 5 Ranks"
echo "=========================================="

cd /Users/homenhoma/dev_projects/class/CS453/mpi_runtime

GRAPH="../outputs/exp1_100nodes.json"
PART="../outputs/exp1_100nodes_part.json"

echo ""
echo "--- Running Leader Election (5 ranks) ---"
mpirun -n 5 ./build/ngs_mpi --graph $GRAPH --part $PART --algo leader --rounds 30

echo ""
echo "--- Running Dijkstra (5 ranks) ---"
mpirun -n 5 ./build/ngs_mpi --graph $GRAPH --part $PART --algo dijkstra --source 0

echo ""
echo "=========================================="
echo "Experiment 1 Complete"
echo "=========================================="