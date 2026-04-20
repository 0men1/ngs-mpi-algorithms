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
| Python | 3.8+ | Running graph export and partition tools |
| Java (JDK) | 11+ | Running NetGameSim for graph generation |

### Installing Dependencies (macOS)

```bash
# Install MPI (if not already installed)
brew install open-mpi

# Install CMake
brew install cmake

# Install Google Test
brew install googletest

# Install java (if not already installed)
brew install openjdk@17

# Install Python (if not already installed)
brew install python3
```

### Installing Dependencies (Linux/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y openmpi-bin libopenmpi-dev cmake
sudo apt-get install -y libgtest-dev
sudo apt-get install -y python3 python3-pip
sudo apt-get install -y openjdk-17-jdk
```

**Note:** nlohmann/json is a header-only library and is included via CMake's find_package.

---

## Installation

1. Clone or download the project to your local machine.

2. Initialize the NetGameSim submodule (required for graph generation):
   ```bash
   git submodule update --init --recursive
   ```

3. Build NetGameSim (required for graph generation):
   ```bash
   cd NetGameSim && sbt clean compile assembly
   ```
   This will create the JAR at `NetGameSim/target/scala-3.2.2/netmodelsim.jar`.

4. Ensure all dependencies are installed (see above).

---

## Building

This project uses **CMake** as its build system, with **make** as a convenience wrapper. CMake handles cross-platform configuration and generates the appropriate build files for your system, ensuring consistent builds across different devices and platforms.

### Full Build

```bash
make build
```
### Clean Rebuild

```bash
make clean_build
```

### Direct CMake Usage

If you prefer to use CMake directly:
```bash
cmake -B build -S mpi_runtime
cmake --build build
```
---

## Running

### Run the Main Application

The main executable requires graph and partition files in JSON format:

```bash
# Ensure binaries are already built before executing the commands below.

# Run Dijkstra algorithm
mpirun -n <# of ranks> ./build/ngs_mpi --graph <path to exported graph> --part <path to exported partition> --algo dijkstra --source <source node id>

# Run Leader Election algorithm
mpirun -n <# of ranks> ./build/ngs_mpi --graph <path to exported graph> --part <path to exported partition> --algo leader --rounds <# of rounds>
```

**Command-line Options:**

All arguments are required. Use `--help` or `-h` to display usage information.

| Option | Description | Required For |
|--------|-------------|--------------|
| `--graph`, `-g` | Path to graph JSON file | All algorithms |
| `--part`, `-p` | Path to partition JSON file | All algorithms |
| `--algo`, `-a` | Algorithm to run (`dijkstra` or `leader`) | All algorithms |
| `--source`, `-s` | Source node for Dijkstra | dijkstra |
| `--rounds`, `-r` | Number of rounds for Leader Election | leader |

### Run All Tests

```bash
make run_test
```

This command will:
1. Build the project (if not already built)
2. Run Dijkstra tests with 2 MPI ranks
3. Run Leader Election tests with 2 MPI ranks

---

## Project Structure

```
ngs-mpi-algorithms/                          # Project root
├── README.md                   # This file
├── REPORT.md                   # Technical documentation
├── student.txt                 # Student info
├── build/                  # Build output directory
├── configs/                    # NetGameSim configurations
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
│   │   ├── test_dijkstra.cpp  # Dijkstra and GraphData tests (20 tests)
│   │   ├── test_leaderelection.cpp  # Leader election tests (10 tests)
│   │   └── test_graphs/       # Test data
├── tools/                      # Utility scripts
│   ├── graph_export/           # Graph generation tools
│   │   ├── run.sh             # Generate graph from NetGameSim
│   │   └── enrichment.py      # Convert NGS to JSON format
│   └── partition/              # Graph partitioning tools
│       ├── run.sh              # Create partition files
│       └── partition.py        # Partition algorithm
├── experiments/                # Experiment scripts
│   ├── experiment1.sh         # 101 nodes / 5 ranks
│   ├── experiment2.sh         # 51 nodes / 2 ranks
│   ├── experiment3.sh         # 201 nodes / 10 ranks
│   └── graphs/                 # Pre-generated experiment graphs
├── NetGameSim/                 # Graph generation library (git submodule)
│   └── target/scala-3.2.2/
│       └── netmodelsim.jar     # Build with: cd NetGameSim && sbt clean compile assembly
└── outputs/                    # Generated output directory
```

---

## Tools

### Graph Export (`tools/graph_export/`)

Generates connected weighted graphs using NetGameSim and converts them to JSON. The script automatically extracts `seed`, `outputDirectory`, and `fileName` from the provided config file. 

**Files:**
- `run.sh` - Wrapper script that runs NetGameSim and calls enrichment.py
- `enrichment.py` - Converts NGS format to JSON, adds edge weights, verifies connectivity


**Arguments:**
| Option | Description | Required |
|--------|-------------|---------|
| `-c` | Path to NGS config file | Yes |
| `-o` | Output JSON file path | Yes |
| `-h` | Show help | No |

**Usage:**
```bash
# Generate graph from config (seed/directory extracted from config)
./tools/graph_export/run.sh -c configs/example.conf -o outputs/mygraph.json
```

**Output:** JSON file with `metadata` (num_nodes, seed) and `adjacency_list` (node -> edges with v and w fields).

**Note:** NetGameSim adds an extra "initial node" to the graph. For `statesTotal = N`, the generated graph will have N+1 nodes (nodes 0 to N). The initial node provides an entry point for graph traversal.

---

### Partition (`tools/partition/`)

Partitions graph nodes across MPI ranks using round-robin assignment.

**Files:**
- `run.sh` - Wrapper script (note: uses getopts, call python directly for simpler syntax)
- `partition.py` - Reads graph JSON, assigns each node to a rank, outputs ownership map


**Arguments:**
| Option | Description | Required |
|--------|-------------|---------|
| `-g` | Path to exported graph.json | Yes |
| `-r` | Number of ranks | Yes |
| `-o` | Output JSON file path | Yes |
| `-h` | Show help | No |

**Usage:**
```bash
# Example: Partition graph "graph.json" into 4 ranks and output into "part.json" file
./tools/partition/run.sh -g outputs/graph.json -r 4 -o outputs/part.json
```

**Output:** JSON file mapping node ID to rank (e.g., `{"0": 0, "1": 0, "2": 1, ...}`).

---

## Quick Example

### Step-by-Step

This example walks through the complete pipeline: generate a graph with NetGameSim, partition it, and run the algorithms.

**Step 1: Generate a graph with NetGameSim**

Create a config file (e.g., `configs/myconfig.conf`) or use the default `example.conf`:
```bash
./tools/graph_export/run.sh -c configs/example.conf -o outputs/example_graph.json
```

Generated files:
- `outputs/example_graph.ngs` - Raw NetGameSim output
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
make clean_build

# Run Dijkstra (source node 0)
mpirun -n 2 ./build/ngs_mpi --graph outputs/example_graph.json --part outputs/example_part.json --algo dijkstra --source 0

# Run Leader Election (10 rounds)
mpirun -n 2 ./build/ngs_mpi --graph outputs/example_graph.json --part outputs/example_part.json --algo leader --rounds 10
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

### Running Experiments

Experiments use pre-generated graphs from NetGameSim. Run these from the project root directory:

```bash
# Run all experiments (from root directory, NOT mpi_runtime/)
./experiments/experiment1.sh  # 101 nodes, 5 ranks
./experiments/experiment2.sh  # 51 nodes, 2 ranks
./experiments/experiment3.sh  # 201 nodes, 10 ranks
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
make clean_build
```
