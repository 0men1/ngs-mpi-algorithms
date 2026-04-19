# MPI Distributed Graph Algorithm Runtime

A distributed MPI-based runtime for executing graph algorithms on partitioned graphs. Supports Distributed Dijkstra (shortest path) and Distributed Leader Election.

## Table of Contents
1. [Dependencies](#dependencies)
2. [Installation](#installation)
3. [Building](#building)
4. [Running](#running)
5. [Project Structure](#project-structure)
6. [Tools](#tools)
7. [Quick Example](#quick-example)

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
### Clean Rebuild

```bash
rm -rf build
cmake -B build
cmake --build build
```

---

## Running

### Run the Main Application

The main executable requires graph and partition files in JSON format. Run this from the `mpi_runtime/` directory:

```bash
cd mpi_runtime

# Clean and build (if necessary)
rm -rf build
cmake -B build
cmake --build build

# Run Dijkstra algorithm
mpirun -n 2 ./build/ngs_mpi --graph ../outputs/graph.json --part ../outputs/part.json --algo dijkstra --source 0

# Run Leader Election algorithm
mpirun -n 2 ./build/ngs_mpi --graph ../outputs/graph.json --part ../outputs/part.json --algo leader --rounds 30
```

**Command-line Options:**

| Option | Description | Default |
|--------|-------------|---------|
| `--graph`, `-g` | Path to graph JSON file | `../outputs/graph.json` |
| `--part`, `-p` | Path to partition JSON file | `../outputs/part.json` |
| `--algo`, `-a` | Algorithm to run (`dijkstra` or `leader`) | `dijkstra` |
| `--source`, `-s` | Source node for Dijkstra | `0` |
| `--rounds`, `-r` | Number of rounds for Leader Election | `200` |

**Note:** Default paths are relative to the `mpi_runtime/` directory where the executable runs. Use absolute paths or adjust accordingly.

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


---

## Project Structure

```
CS453/                          # Project root
├── README.md                   # This file
├── REPORT.md                   # Technical documentation
├── student.txt                 # Student info
├── CMakeLists.txt              # Build configuration
├── configs/                    # NetGameSim configuration
│   ├── defconfig.conf          # Default config (100 nodes)
│   └── example.conf            # Example config (10 nodes)
├── mpi_runtime/                # MPI runtime implementation
│   ├── CMakeLists.txt          # Build configuration
│   ├── Makefile                # Convenience build targets
│   ├── include/               # Header files
│   ├── src/                    # Source code
│   │   ├── main.cpp           # Application entry point
│   │   ├── GraphData.cpp/h    # Graph loading and partitioning
│   │   ├── DistributedDijkstra.cpp/h  # Shortest path algorithm
│   │   ├── DistributedLeaderElection.cpp/h  # Leader election
│   │   └── DistributedAlgorithm.h  # Abstract base class
│   ├── tests/                 # Test suites
│   │   ├── test_dijkstra.cpp  # Dijkstra tests (8 tests)
│   │   ├── test_leaderelection.cpp  # Leader election tests (10 tests)
│   │   └── test_graphs/       # Test data
│   │       ├── testgraph1.json    # 10-node test graph
│   │       ├── testpart1.json    # Partition file
│   │       ├── simple_graph.json  # 4-node simple graph
│   │       ├── simple_part.json   # Simple partition
│   │       ├── chain_graph.json   # 5-node chain graph
│   │       └── chain_part.json    # Chain partition
│   └── build/                  # Build output directory
├── tools/                      # Utility scripts
│   ├── graph_export/           # Graph generation tools
│   │   ├── run.sh             # Generate graph from NetGameSim
│   │   └── enrichment.py      # Convert NGS to JSON format
│   └── partition/              # Graph partitioning tools
│       ├── run.sh              # Create partition files
│       └── partition.py        # Partition algorithm
├── experiments/                # Experiment scripts
│   ├── experiment1.sh         # 100 nodes / 5 ranks
│   ├── experiment2.sh         # 50 nodes / 2 ranks
│   ├── experiment3.sh         # 200 nodes / 10 ranks
│   └── graphs/                 # Pre-generated experiment graphs
│       ├── exp1_100nodes.json
│       ├── exp1_100nodes_part.json
│       ├── exp2_50nodes.json
│       ├── exp2_50nodes_part.json
│       ├── exp3_200nodes.json
│       └── exp3_200nodes_part.json
├── NetGameSim/                 # Graph generation library
│   └── target/scala-3.2.2/
│       └── netmodelsim.jar     # Pre-built JAR
└── outputs/                    # Generated output directory
```

---

## Tools

### Graph Export (`tools/graph_export/`)

Generates connected weighted graphs using NetGameSim and converts them to JSON.

**Files:**
- `run.sh` - Wrapper script that runs NetGameSim and calls enrichment.py
- `enrichment.py` - Converts NGS format to JSON, adds edge weights, verifies connectivity

**Usage:**
```bash
# Using default config (configs/defconfig.conf)
./tools/graph_export/run.sh

# Using custom config and output path
./tools/graph_export/run.sh -c configs/myconfig.conf -o outputs/mygraph.json
```

**Arguments:**
| Option | Description | Default |
|--------|-------------|---------|
| `-c` | Path to NGS config file | `configs/defconfig.conf` |
| `-o` | Output JSON file path | `outputs/graph.json` |
| `-h` | Show help | - |

**Output:** JSON file with `metadata` (num_nodes, seed) and `adjacency_list` (node -> edges with v and w fields).

---

### Partition (`tools/partition/`)

Partitions graph nodes across MPI ranks using round-robin assignment.

**Files:**
- `run.sh` - Wrapper script (note: uses getopts, call python directly for simpler syntax)
- `partition.py` - Reads graph JSON, assigns each node to a rank, outputs ownership map

**Usage:**
```bash
# Direct call (recommended)
python tools/partition/partition.py <graph.json> <num_ranks> <output.json>

# Example
python tools/partition/partition.py outputs/graph.json 4 outputs/part.json
```

**Arguments:**
| Position | Description |
|----------|-------------|
| 1 | Input graph JSON file |
| 2 | Number of MPI ranks |
| 3 | Output partition JSON file |

**Output:** JSON file mapping node ID to rank (e.g., `{"0": 0, "1": 0, "2": 1, ...}`).

---

## Quick Example

### End-to-End Graph to Algorithm Run

This example walks through the complete pipeline: generate a graph with NetGameSim, partition it, and run the algorithms.

**Step 1: Generate a graph with NetGameSim**

Create a config file (e.g., `configs/example.conf`) or use the default:
```bash
# Generate graph with default config (seed=100, 100 nodes)
./tools/graph_export/run.sh

# Or use a custom config
./tools/graph_export/run.sh -c configs/example.conf -o outputs/example_graph.json
```

Generated files:
- `outputs/raw_ngs_graph.ngs` - Raw NetGameSim output
- `outputs/example_graph.json` - Enriched graph in JSON format (with weights)

**Step 2: Partition the graph across MPI ranks**

```bash
# Partition into 2 ranks
./tools/partition/run.sh -o outputs/example_part.json -g outputs/example_graph.json -r 2
```

Generated file:
- `outputs/example_part.json` - Node-to-rank mapping

**Step 3: Run the algorithms**

```bash
cd mpi_runtime

# Clean and build (if necessary)
rm -rf build
cmake -B build
cmake --build build

# Run Dijkstra (source node 0)
mpirun -n 2 ./build/ngs_mpi --graph ../outputs/example_graph.json --part ../outputs/example_part.json --algo dijkstra --source 0

# Run Leader Election (10 rounds)
mpirun -n 2 ./build/ngs_mpi --graph ../outputs/example_graph.json --part ../outputs/example_part.json --algo leader --rounds 10
```

**Example Output (Dijkstra):**
```
================ DIJKSTRA METRICS ================
Total Runtime (s): 0.000559
Total Iterations: 11
Total P2P Messages Sent: 9
Total P2P Bytes Sent: 176

--- Final Distances ---
Node 0 | Distance: 0.000000
Node 1 | Distance: 0.488197
...
==================================================
```

**Example Output (Leader Election):**
```
================ LEADER ELECTION METRICS ================
Total Runtime (s): 0.000077
Total Rounds: 10
Total Payload Messages Sent: 40
Total Payload Bytes Sent: 240

--- Agreement Validation ---
STATUS: SUCCESS
Agreed Leader ID: 9
=========================================================
```

### End-to-End Test Run

This example demonstrates running the test suite to verify correctness:

```bash
# Navigate to project directory
cd mpi_runtime

# Clean and build (if necessary)
rm -rf build
cmake -B build
cmake --build build

# Run all tests
make run_test
```

**Expected Output:**
```
[==========] Running 19 tests from 2 test suites.
[----------] 15 tests from DijkstraTest
[----------] 4 tests from GraphDataTest
[==========] 19 tests from 2 test suites ran.
[  PASSED  ] 19 tests.

[==========] Running 10 tests from 1 test suite.
[----------] 10 tests from LeaderElectionTest
[==========] 10 tests from 1 test suite ran.
[  PASSED  ] 10 tests.

Total: 29/29 tests passed
```

### Running Experiments

Experiments use pre-generated graphs from NetGameSim. Run these from the project root directory:

```bash
# Run all experiments (from CS453/ directory, NOT mpi_runtime/)
./experiments/experiment1.sh  # 100 nodes, 5 ranks
./experiments/experiment2.sh  # 50 nodes, 2 ranks
./experiments/experiment3.sh  # 200 nodes, 10 ranks
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
