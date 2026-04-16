#!/bin/bash

# Experiment 3: 200 nodes with 10 MPI ranks
# Test large graph with many partitions (uneven but close distribution)

echo "=========================================="
echo "Experiment 3: 200 Nodes / 10 Ranks"
echo "=========================================="

cd /Users/homenhoma/dev_projects/class/CS453/mpi_runtime

GRAPH="../outputs/exp3_200nodes.json"
PART="../outputs/exp3_200nodes_part.json"

echo ""
echo "--- Running Leader Election (10 ranks) ---"
mpirun -n 10 ./build/ngs_mpi --graph $GRAPH --part $PART --algo leader --rounds 40

echo ""
echo "--- Running Dijkstra (10 ranks) ---"
mpirun -n 10 ./build/ngs_mpi --graph $GRAPH --part $PART --algo dijkstra --source 0

echo ""
echo "=========================================="
echo "Experiment 3 Complete"
echo "=========================================="