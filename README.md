# MPI Distributed Graph Algorithm Runtime

A distributed MPI-based runtime for executing graph algorithms on partitioned graphs. Supports Distributed Dijkstra (shortest path) and Distributed Leader Election.

## Table of Contents
1. [Dependencies](#dependencies)
2. [Installation](#installation)
3. [Building](#building)
4. [Running](#running)
5. [Project Structure](#project-structure)
6. [Quick Example](#quick-example)

---

## Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| MPI (OpenMP or similar) | 3.1+ | Distributed communication |
| CMake | 3.14+ | Build system |
| C++ Compiler | C++17 | Language support |
| Google Test | 1.10+ | Unit testing framework |
| nlohmann/json | 3.2.0+ | JSON parsing for graph input |

### Installing Dependencies (macOS)

```bash
# Install MPI (if not already installed)
brew install open-mpi

# Install CMake
brew install cmake

# Install Google Test
brew install googletest
```

### Installing Dependencies (Linux/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y openmpi-bin libopenmpi-dev cmake
sudo apt-get install -y libgtest-dev
```

**Note:** nlohmann/json is a header-only library and is included via CMake's find_package.

---

## Installation

1. Clone or download the project to your local machine.

2. Ensure all dependencies are installed (see above).

3. Navigate to the project directory:
```bash
cd mpi_runtime
```

---

## Building

### Full Build

```bash
cmake -B build
cmake --build build
```

### Build with Verbose Output

```bash
cmake -B build
cmake --build build --verbose
```

### Clean Rebuild

```bash
rm -rf build
cmake -B build
cmake --build build
```

---

## Running

### Run All Tests

```bash
make run_test
```

This command will:
1. Build the project (if not already built)
2. Run Dijkstra tests with 2 MPI ranks
3. Run Leader Election tests with 2 MPI ranks

### Run Individual Test Suites

```bash
# Run only Dijkstra tests
mpirun -n 2 ./build/run_test_dijkstra

# Run only Leader Election tests
mpirun -n 2 ./build/run_test_leaderelection
```

### Run Specific Tests

```bash
# Run a specific test
mpirun -n 2 ./build/run_test_dijkstra --gtest_filter=DijkstraTest.DijkstraSourceDistanceZero

# Run all tests matching a pattern
mpirun -n 2 ./build/run_test_leaderelection --gtest_filter=LeaderElectionTest.*
```

### Run the Main Application

```bash
# Run with the main executable (requires graph and partition files)
mpirun -n 2 ./build/ngs_mpi <graph_file.json> <partition_file.json>
```

---

## Project Structure

```
mpi_runtime/
├── CMakeLists.txt           # Build configuration
├── Makefile                 # Convenience build targets
├── README.md                # This file
├── REPORT.md                # Technical documentation
├── src/                     # Source code
│   ├── main.cpp
│   ├── GraphData.cpp/h      # Graph loading and partitioning
│   ├── DistributedDijkstra.cpp/h  # Shortest path algorithm
│   ├── DistributedLeaderElection.cpp/h  # Leader election
│   └── DistributedAlgorithm.h     # Abstract base class
├── tests/                   # Test suites
│   ├── test_dijkstra.cpp    # Dijkstra tests (4 tests)
│   └── test_leaderelection.cpp  # Leader election tests (7 tests)
└── tests/test_graphs/         # Test data
    ├── testgraph1.json       # 10-node test graph
    ├── testpart1.json       # Partition file
    ├── simple_graph.json    # 4-node simple graph
    ├── simple_part.json     # Simple partition
    ├── chain_graph.json     # 5-node chain graph
    └── chain_part.json      # Chain partition
```

---

## Quick Example

### End-to-End Test Run

This example demonstrates running the test suite to verify correctness:

```bash
# Navigate to project directory
cd mpi_runtime

# Clean and build
rm -rf build
cmake -B build
cmake --build build

# Run all tests
make run_test
```

**Expected Output:**
```
[==========] Running 8 tests from 2 test suites.
[----------] 4 tests from DijkstraTest
[----------] 4 tests from GraphDataTest
[==========] 8 tests from 2 test suites ran.
[  PASSED  ] 8 tests.

[==========] Running 7 tests from 1 test suite.
[----------] 7 tests from LeaderElectionTest
[==========] 7 tests from 1 test suite ran.
[  PASSED  ] 7 tests.

Total: 15/15 tests passed
```

### Creating Custom Test Data

1. Create a graph JSON file:
```json
{
  "metadata": { "num_nodes": 3 },
  "adjacency_list": {
    "0": [{ "v": 1, "w": 1.0 }],
    "1": [{ "v": 0, "w": 1.0 }, { "v": 2, "w": 1.0 }],
    "2": [{ "v": 1, "w": 1.0 }]
  }
}
```

2. Create a partition JSON file:
```json
{
  "0": 0,
  "1": 0,
  "2": 1
}
```

3. Run with the partition:
```bash
mpirun -n 2 ./build/ngs_mpi my_graph.json my_partition.json
```

---

## Troubleshooting

### "No such file or directory" errors

Ensure test data files exist in the correct location. The tests expect files in `tests/test_graphs/` relative to the test executable.

### MPI initialization errors

Ensure MPI is properly installed:
```bash
which mpirun
mpirun --version
```

### Build errors

Clean the build directory and rebuild:
```bash
rm -rf build
cmake -B build
cmake --build build
```
