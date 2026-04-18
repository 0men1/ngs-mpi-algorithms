# MPI Distributed Graph Algorithm Runtime - Technical Report

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Architectural Design](#architectural-design)
3. [Algorithm Choices](#algorithm-choices)
4. [Implementation Details](#implementation-details)
5. [Data Formats](#data-formats)
6. [Partition Model](#partition-model)
7. [Assumptions for Correctness](#assumptions-for-correctness)
8. [Experimental Design](#experimental-design)
9. [Results and Analysis](#results-and-analysis)
10. [Key Insights and Decisions](#key-insights-and-decisions)

---

## Executive Summary

This project implements a distributed MPI-based runtime for executing graph algorithms on partitioned graphs. The system supports two primary algorithms: Distributed Dijkstra (shortest path) and Distributed Leader Election. The implementation distributes graph nodes across MPI ranks, with each rank maintaining local graph data and communicating via message passing.

The project was developed as a coursework assignment for CS453 (Distributed Systems) at [University]. The primary goal was to implement and verify distributed graph algorithms that can scale across multiple MPI processes while maintaining correctness and demonstrating effective communication patterns.

**Project Motivation:**
Distributed graph algorithms are fundamental to many real-world applications including network routing, social network analysis, and distributed database systems. This project provides a framework for exploring how traditional graph algorithms can be adapted to work in a distributed computing environment using MPI for inter-process communication.

**Key Contributions:**
- Implementation of a partition-based distributed Dijkstra algorithm with cross-partition edge handling
- Implementation of a flooding-based distributed leader election algorithm
- Comprehensive test suite with 29 tests covering correctness and edge cases
- Support for arbitrary graph partitioning via JSON configuration
- Integration with NetGameSim for large-scale graph generation

---

## Architectural Design

### System Overview

The runtime follows a partition-based distributed computing model where:

1. A graph is partitioned across N MPI processes
2. Each process owns a subset of nodes and their associated edges
3. Communication occurs via MPI point-to-point and collective operations
4. Results are aggregated across all ranks for verification

### Component Architecture

```
+------------------------------------------------------------------+
|                         Main Runtime                             |
+------------------------------------------------------------------+
|                                                                  |
|  +-------------------+      +-------------------+                |
|  |   GraphData       |      | DistributedAlg    |                |
|  | ------------------|      | (Abstract Base)   |                |
|  | - m_ownedNodes    |      | + execute()       |                |
|  | - m_adjList       |      | + reportMetrics() |                |
|  | - m_incomingEdges |      +--------+----------+                |
|  | - m_nodeOwnership |               |                           |
|  +-------------------+        +--------v--------+                |
|                               |                 |                |
|                 +--------------+    +---------+------+           |
|                 | Dijkstra     |    | LeaderElection |           |
|                 +--------------+    +----------------+           |
+------------------------------------------------------------------+

                          MPI Communication Layer
                          (MPI_Send/Recv, MPI_Allreduce, etc.)
```

### Data Structures

**GraphData Class:**
- `m_ownedNodes`: Set of node IDs owned by the current rank
- `m_adjList`: Map of source nodes to outgoing edges (for sending updates)
- `m_incomingEdges`: Map of destination nodes to incoming edges (for processing received updates)
- `m_nodeOwnership`: Global array mapping node IDs to rank ownership

**DistributedDijkstra Class:**
- `m_sourceNode`: The source node for shortest path computation
- `m_distances`: Vector of distances from source to each node
- `m_numIterations`: Number of main loop iterations executed
- `m_numMessages`: Count of point-to-point messages sent
- `m_bytesSent`: Total bytes transmitted

**DistributedLeaderElection Class:**
- `m_rounds`: Number of flooding rounds to execute
- `m_finalLeaders`: Map from node IDs to their elected leader
- `m_numIterations`: Actual number of rounds executed
- `m_numMessages`: Count of Alltoall operations
- `m_bytesSent`: Total bytes transmitted

### Memory Layout and Access Patterns

The memory layout is optimized for the specific access patterns of each algorithm:

**Dijkstra Access Pattern:**
- Each iteration requires finding the minimum distance among unvisited owned nodes (O(n_owned))
- When a node is settled, outgoing edges are traversed to generate update messages
- Updates are routed based on destination node ownership

**Leader Election Access Pattern:**
- Each round iterates over all owned nodes and their outgoing edges
- Messages are aggregated by destination rank for efficient Alltoall communication
- Final leader values are stored in a hash map for O(1) lookup

### Partitioning Strategy

The graph is partitioned by node ownership. The partition file specifies which rank owns each node. This design choice ensures:

1. Each rank has clear ownership of specific nodes
2. Message routing is deterministic based on node ownership
3. Workload distribution can be analyzed via partition statistics

**Partition Balancing Considerations:**
The default partitioning strategy uses a round-robin approach: `rank = (nodeId * numRanks) / numNodes`. This provides reasonably balanced partitions for random graphs but may not be optimal for structured graphs with non-uniform connectivity. Future improvements could include graph-aware partitioning using tools like METIS.

---

## Algorithm Choices

### Distributed Dijkstra (Shortest Path)

**Algorithm: Parallel Dijkstra with Distributed Priority Queue**

The implementation uses a synchronized local minimum approach:

```
1. Initialize: Set source node distance to 0, all others to infinity
2. Repeat until convergence:
   a. Each rank finds local unvisited node with minimum distance
   b. MPI_Allreduce (MINLOC) to find global minimum node
   c. If global minimum is infinity, terminate
   d. Mark global minimum node as settled
   e. Rank owning the settled node:
      - Sends distance updates to neighbors across ranks
      - Applies updates to local nodes
   f. Other ranks receive updates and apply to owned nodes
3. Return distance vector for all nodes
```

**Time Complexity:** O(V * E / N) where N is the number of ranks
**Message Complexity:** O(E) messages per iteration in worst case

**Rationale:** This approach balances work distribution while minimizing synchronization overhead. The Allreduce provides a barrier that ensures all ranks agree on the next node to settle before proceeding.

### Distributed Leader Election

**Algorithm: Flooding-based Maximum Finding**

```
1. Initialize: Each node's current max is its own ID
2. For specified number of rounds:
   a. Each node propagates its current max to neighbors
   b. Nodes receive max values from neighbors
   c. Each node updates to max(received_values, current)
3. All nodes converge to the maximum node ID in the graph
```

**Time Complexity:** O(D * rounds) where D is the graph diameter
**Message Complexity:** O(E * rounds)

**Rationale:** This algorithm is simple, robust, and requires no special coordination. The flooding approach naturally handles network topology without complex initialization.

**Comparison with Alternative Algorithms:**

| Algorithm | Complexity | Coordination Required | Robustness |
|-----------|------------|----------------------|------------|
| Chang-Roberts | O(n log n) | Ring topology | Moderate |
| LCR | O(n log n) | Ring topology | Moderate |
| Flooding-based | O(D * rounds) | None | High |
| HS Algorithm | O(log n) | Spanning tree | Low |

The flooding-based approach was chosen for its simplicity and robustness to network topology changes, despite higher message complexity. In practice, for small-to-medium graphs, the difference is negligible.

---

## Implementation Details

### MPI Communication Patterns

**Dijkstra Communication:**
- `MPI_Allreduce` (MINLOC): Synchronize global minimum node selection
- `MPI_Bcast`: Broadcast send counts to all ranks
- `MPI_Send/Recv`: Point-to-point transfer of distance updates

**Leader Election Communication:**
- `MPI_Alltoall`: Exchange message counts between all ranks
- `MPI_Alltoallv`: Variable-length all-to-all for actual message exchange

### Message Formats

**UpdateMsg (Dijkstra):**
```cpp
struct UpdateMsg {
    int nodeId;      // Target node for distance update
    float dist;      // New distance value
};
```

**ElectMsg (Leader Election):**
```cpp
struct ElectMsg {
    int destNode;    // Destination node
    int maxNodeId;   // Maximum ID being propagated
};
```

### Build System

The project uses CMake with the following structure:
- Main executable: `ngs_mpi`
- Test executables: `run_test_dijkstra`, `run_test_leaderelection`
- Dependencies: MPI, Google Test, nlohmann_json

**Build Requirements:**
- CMake 3.14+
- C++17 compiler
- OpenMPI 3.1+ or equivalent MPI implementation
- Google Test 1.10+
- nlohmann/json 3.2.0+

**Dependencies are automatically resolved via CMake's find_package:**
```cmake
find_package(MPI REQUIRED)
find_package(GTest REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
```

### Testing Infrastructure

The testing approach uses Google Test with MPI-aware test fixtures:

**Test Fixtures:**
- `GraphDataTest`: Base fixture for graph-related tests
- `DijkstraTest`: Fixture for Dijkstra algorithm tests

**Test Graphs Used:**
- `testgraph1.json`: 10-node graph with complex topology (cross-partition edges)
- `simple_graph.json`: 4-node bipartite graph for basic correctness
- `chain_graph.json`: 5-node linear chain for propagation testing

**Test Coverage:**
- Graph loading and partition parsing (4 tests)
- Dijkstra correctness on various graph types (8 tests)
- Leader election convergence and agreement (10 tests)

---

## Data Formats

### Graph Input Format (JSON)

The graph is stored in JSON format with the following structure:

```json
{
  "metadata": {
    "num_nodes": <integer>  // Total number of nodes in the graph
  },
  "adjacency_list": {
    "<node_id>": [           // Node ID as string key
      {
        "v": <integer>,     // Destination node ID
        "w": <float>        // Edge weight
      },
      ...
    ],
    ...
  }
}
```

**Example:**
```json
{
  "metadata": { "num_nodes": 4 },
  "adjacency_list": {
    "0": [{ "v": 1, "w": 0.5 }, { "v": 2, "w": 0.2 }],
    "1": [{ "v": 0, "w": 0.5 }],
    "2": [{ "v": 0, "w": 0.2 }, { "v": 3, "w": 0.4 }],
    "3": [{ "v": 2, "w": 0.4 }]
  }
}
```

### Partition Format (JSON)

The partition defines node ownership across MPI ranks:

```json
{
  "<node_id>": <rank_id>,
  ...
}
```

**Example:**
```json
{
  "0": 0,
  "1": 0,
  "2": 1,
  "3": 1
}
```

This partition assigns nodes 0-1 to rank 0 and nodes 2-3 to rank 1.

### Message Formats

#### Dijkstra UpdateMsg

```cpp
struct UpdateMsg {
    int nodeId;      // Target node for distance update (4 bytes)
    float dist;      // New distance value (4 bytes)
};
// Total: 8 bytes per message
```

Serialization: Raw byte transfer via `MPI_BYTE`

#### Leader Election ElectMsg

```cpp
struct ElectMsg {
    int destNode;    // Destination node ID (4 bytes)
    int maxNodeId;   // Maximum ID being propagated (4 bytes)
};
// Total: 8 bytes per message
```

Serialization: Raw byte transfer via `MPI_BYTE`

### MPI Datatypes

| Message Type | MPI Datatype | Rationale |
|-------------|--------------|-----------|
| UpdateMsg | `MPI_BYTE` | Custom struct, sent as raw bytes |
| ElectMsg | `MPI_BYTE` | Custom struct, sent as raw bytes |
| Distance array | `MPI_FLOAT` | Standard float array |
| Node IDs | `MPI_INT` | Standard integer array |

---

## Partition Model

### Ownership Model

The partition model uses **node-based ownership** where:

1. Each node is assigned to exactly one MPI rank
2. The owning rank is responsible for:
   - Storing the distance/leader value for that node
   - Initiating updates when this node is settled (Dijkstra)
   - Including this node in local minimum computations
3. Non-owning ranks track information about nodes they do not own

### Edge Storage Strategy

To support distributed computation, each rank maintains two edge views:

| Data Structure | Purpose | Edges Stored |
|---------------|---------|--------------|
| `m_adjList` | Outgoing edges for sending updates | Edges where rank owns SOURCE node |
| `m_incomingEdges` | Incoming edges for processing updates | Edges where rank owns DESTINATION node |

### Why Two Edge Views?

Consider a graph edge from node A (rank 0) to node B (rank 1):

```
Rank 0: owns node A
  - Must send updates about B when A is settled
  - Stores edge in m_adjList[A]

Rank 1: owns node B
  - Must receive and process updates about B
  - Stores edge in m_incomingEdges[B]
```

This design ensures each rank can:
- Send updates for nodes it owns (via m_adjList)
- Process updates received for nodes it owns (via m_incomingEdges)

### Partition File Requirements

1. Every node ID from 0 to (num_nodes - 1) must appear exactly once
2. All rank values must be valid MPI ranks (0 to size-1)
3. Partition files are JSON for human readability

**Partition Balancing Guidelines:**
- For uniform random graphs: simple round-robin partitioning works well
- For structured graphs: consider graph-aware partitioning (e.g., METIS)
- Each partition should have approximately equal node counts for balanced work

---

## Assumptions for Correctness

### Dijkstra Algorithm Assumptions

**CRITICAL: These assumptions are required for correctness**

1. **Positive Edge Weights**
   - All edge weights must be non-negative (w >= 0)
   - Reason: Dijkstra's algorithm fails with negative weights
   - The algorithm uses infinity as initial distance, which only works with positive weights
   - Impact: If negative weights exist, the algorithm may produce incorrect shortest paths

2. **Connected Graph**
   - The graph should be connected (or at least the source node should reach all target nodes)
   - Unreachable nodes will have distance = infinity
   - This is expected behavior, not an error

3. **No Self-Loops Required**
   - Self-loops are ignored in the current implementation
   - Does not affect correctness

4. **Undirected Edges Implicit**
   - The adjacency list stores directed edges
   - For undirected graphs, each direction must be explicitly added
   - Example: An undirected edge between 0 and 1 requires entries in both "0" and "1" lists

5. **Synchronous Communication**
   - The algorithm uses blocking MPI calls (MPI_Send, MPI_Recv)
   - All ranks must participate in the synchronization
   - Deadlock occurs if ranks diverge in control flow

### Leader Election Algorithm Assumptions

1. **Unique Node IDs**
   - All node IDs must be unique integers
   - The algorithm elects the maximum ID, which requires uniqueness

2. **Sufficient Rounds**
   - The algorithm requires at least (graph diameter) rounds to converge
   - With fewer rounds, nodes may not agree on the leader
   - The number of rounds must be specified explicitly

3. **Connected Graph**
   - The graph must be connected for all nodes to receive max values
   - In disconnected graphs, each component will elect its own maximum

4. **Bidirectional Communication**
   - For complete convergence, edges should be traversable in both directions
   - The partition must allow messages to flow across the graph

### General MPI Assumptions

1. **Reliable Communication**
   - MPI does not lose messages in normal operation
   - No error handling for communication failures

2. **Synchronized Termination**
   - All ranks must call MPI_Finalize together
   - Premature termination on one rank causes errors on others

3. **Consistent Partition Files**
   - All ranks must use the same partition file
   - Inconsistent partitions cause undefined behavior

---

## Experimental Design

### Hypothesis

**Primary Hypothesis:**
The distributed Dijkstra algorithm will correctly compute shortest paths when graph partitions result in cross-partition edges, with each rank maintaining accurate distances for its owned nodes.

**Secondary Hypothesis:**
The leader election algorithm will converge to the maximum node ID across all partitions, with all ranks agreeing on the same leader after sufficient rounds.

### Test Cases

**Dijkstra Tests (15 tests):**
1. Convergence verification - Algorithm completes without errors
2. Source distance verification - Node 0 has distance 0.0
3. Cross-partition distance verification - Nodes 5, 9 on rank 1 have correct distances
4. Alternative source node testing - Works with different source nodes
5. Simple graph correctness verification
6. Owned nodes have finite distances
7. Message tracking metrics
8. Edge weights are non-negative (correctness assumption)
9. Source 0 all distances - Verifies all distances from source 0 (testgraph1)
10. Source 5 all distances - Verifies all distances from source 5 (testgraph1)
11. Source 9 all distances - Verifies all distances from source 9 (testgraph1)
12. Simple graph source 0 - All distances from source 0 (simple_graph)
13. Simple graph source 3 - All distances from source 3 (simple_graph)
14. Chain graph source 0 - All distances from source 0 (chain_graph)
15. Chain graph source 2 - All distances from source 2 (chain_graph)

**GraphData Tests (4 tests):**
1. Graph and partition loading verification
2. Node ownership correctness
3. Rank 0 owns expected nodes
4. Adjacency list structure validation

**Leader Election Tests (10 tests):**
1. Convergence verification - Algorithm completes without errors
2. Maximum node ID election - Node 9 elected in testgraph1
3. Simple graph verification - Node 3 elected in simple_graph
4. Chain graph verification - Node 4 elected in chain_graph
5. Ownership coverage - All owned nodes have leaders
6. Cross-rank agreement - All nodes agree on same leader
7. Metric tracking - Message and byte counts recorded
8. Iteration count tracking - Correct number of rounds executed
9. Leader is valid node ID verification
10. Maximum rounds stress test - Works with 100 rounds

### Expected Results

**Dijkstra:**
- All nodes reachable from source should have finite distances
- Distances should match theoretical shortest paths
- Source node should always have distance 0

**Leader Election:**
- All nodes should elect the maximum node ID as leader
- All ranks should agree on the same leader value
- Election should complete within graph diameter rounds

### Reproducing Experiments

**Quick Test (5 seconds):**
```bash
cd mpi_runtime
make run_test
```

This runs all 22 tests with 2 MPI ranks and verifies correctness.

**Detailed Test Output:**
```bash
mpirun -n 2 ./build/run_test_dijkstra --gtest_output=xml:test_results.xml
mpirun -n 2 ./build/run_test_leaderelection --gtest_output=xml:test_results.xml
```

**Running with Different MPI Sizes:**
```bash
# Test with 4 ranks
mpirun -n 4 ./build/run_test_dijkstra
mpirun -n 4 ./build/run_test_leaderelection
```

**Creating Custom Experiments:**

1. Create a graph file (e.g., `my_graph.json`):
```json
{
  "metadata": { "num_nodes": 6 },
  "adjacency_list": {
    "0": [{ "v": 1, "w": 1.0 }, { "v": 2, "w": 2.0 }],
    "1": [{ "v": 0, "w": 1.0 }, { "v": 3, "w": 1.0 }],
    "2": [{ "v": 0, "w": 2.0 }, { "v": 4, "w": 1.0 }],
    "3": [{ "v": 1, "w": 1.0 }, { "v": 5, "w": 2.0 }],
    "4": [{ "v": 2, "w": 1.0 }, { "v": 5, "w": 1.0 }],
    "5": [{ "v": 3, "w": 2.0 }, { "v": 4, "w": 1.0 }]
  }
}
```

2. Create a partition file (e.g., `my_part.json`):
```json
{ "0": 0, "1": 0, "2": 0, "3": 1, "4": 1, "5": 1 }
```

3. Run the algorithm:
```bash
./build/ngs_mpi --graph my_graph.json --part my_part.json --algo dijkstra --source 0
```

**Expected Shortest Paths (source=0):**
- Node 0: 0.0
- Node 1: 1.0 (direct edge)
- Node 2: 2.0 (direct edge)
- Node 3: 2.0 (0->1->3)
- Node 4: 3.0 (0->2->4)
- Node 5: 3.0 (0->1->3->5 or 0->2->4->5)

**Expected Leader Election:**
- Maximum node ID is 5, so all nodes should elect node 5 as leader

### Experiment Suite

Three pre-configured experiments are provided to test scalability:

**Experiment 1: 100 Nodes / 5 Ranks**
- Graph: `experiments/graphs/exp1_100nodes.json`
- Partition: `experiments/graphs/exp1_100nodes_part.json`
- Tests scaling with moderate graph size and multiple partitions

**Experiment 2: 50 Nodes / 2 Ranks**
- Graph: `experiments/graphs/exp2_50nodes.json`
- Partition: `experiments/graphs/exp2_50nodes_part.json`
- Tests basic functionality with smaller graph

**Experiment 3: 200 Nodes / 10 Ranks**
- Graph: `experiments/graphs/exp3_200nodes.json`
- Partition: `experiments/graphs/exp3_200nodes_part.json`
- Tests large graph with many partitions

---

## Results and Analysis

### Test Results Summary

**Dijkstra Tests:** 15/15 PASSED
**Leader Election Tests:** 10/10 PASSED
**GraphData Tests:** 4/4 PASSED
**Total:** 29/29 PASSED

### Performance Metrics (Execution Time, Message Counts)

The following metrics are collected for each algorithm execution:

**Dijkstra Metrics:**
- Total Runtime (seconds)
- Total Iterations
- Total Point-to-Point Messages Sent
- Total Bytes Sent

**Leader Election Metrics:**
- Total Runtime (seconds)
- Total Rounds Executed
- Total Payload Messages Sent
- Total Payload Bytes Sent

Metrics are reported via `reportMetrics()` which aggregates values across all ranks using `MPI_Reduce`.

### Dijkstra Results

**Actual Behavior:**
- Rank 0 (owns nodes 0-4 in testgraph1): Computes correct distances for owned nodes
- Rank 1 (owns nodes 5-9 in testgraph1): Receives updates and computes correct distances

**Key Finding - Dual Edge Storage:**
Initial implementation stored edges only for source node ownership, causing rank 1 to have empty adjacency lists. This resulted in incorrect distance propagation across partition boundaries. The fix involved maintaining both outgoing edges (for sending) and incoming edges (for receiving), ensuring that:
1. When a node is settled, its owning rank can send updates via `m_adjList`
2. When updates arrive for a node, its owning rank can process them and propagate via `m_incomingEdges`

**Distance Verification:**
- Node 0: 0.0 (source)
- Node 1: 0.5 (0->1 direct)
- Node 2: 0.2 (0->2 direct)
- Node 5: 0.6 (0->2->5)
- Node 9: 1.5 (0->2->5->9)

### Leader Election Results

**Actual Behavior:**
- Algorithm correctly propagates maximum ID through the graph
- All nodes converge to node 9 (maximum in testgraph1.json)
- Cross-rank agreement verified

**Convergence Analysis:**
For the 10-node testgraph1.json with 2 ranks:
- Rank 0 owns nodes 0-4, Rank 1 owns nodes 5-9
- Maximum node ID is 9 (owned by rank 1)
- With 10 rounds, all nodes agree on leader = 9
- Convergence requires at least graph diameter rounds

### Performance Observations

**Message Complexity:**
- Dijkstra: O(E) messages per iteration, with E being cross-partition edges
- Leader Election: O(E * rounds) with Alltoallv for efficient bulk transfer

**Scalability Trends:**
- Increasing ranks increases communication overhead but may reduce per-rank computation
- Optimal partition size depends on graph structure and network characteristics

---

## Key Insights and Decisions

### Design Decisions

1. **Separate Edge Storage for Send/Receive**
   - Decision: Maintain `m_adjList` (outgoing) and `m_incomingEdges` (incoming)
   - Rationale: Different ranks need different edge views based on send/receive roles
   - Impact: Increased memory usage but clearer algorithm logic

2. **Getter Methods for Testing**
   - Decision: Added `getDistances()`, `getFinalLeaders()`, `getNumMessagesSent()` etc.
   - Rationale: Enable white-box testing of internal algorithm state
   - Trade-off: Breaks encapsulation but essential for verification

3. **Partition File Format**
   - Decision: JSON-based partition specification
   - Rationale: Human-readable, easy to generate and modify
   - Alternative considered: Binary format for efficiency (rejected for simplicity)

4. **Command-line Interface**
   - Decision: Named arguments with `--` prefix (e.g., `--graph`, `--algo`)
   - Rationale: Self-documenting, order-independent argument parsing
   - Implementation: Custom argument parser in `main.cpp`

### Algorithm Insights

1. **Dijkstra Synchronization Point**
   - The Allreduce creates a synchronization barrier each iteration
   - This is necessary for correctness but limits parallelism
   - Future: Investigate asynchronous variants with speculative execution

2. **Leader Election Round Count**
   - Algorithm requires explicit round parameter
   - Future: Implement termination detection for automatic stopping
   - Approach: Use convergence detection with Allreduce

3. **Message Batching**
   - Messages are batched by destination rank before sending
   - Reduces number of MPI operations at the cost of memory buffering
   - Trade-off favorable for most practical scenarios

### Testing Insights

1. **MPI Test Execution**
   - Tests require `mpirun -n 2` to execute properly
   - Each rank runs the full test suite independently
   - Future: Add distributed verification tests that cross-check results

2. **Test Data Requirements**
   - Simple graphs essential for debugging
   - Chain graphs reveal propagation issues
   - Future: Add stress tests with larger graphs (1000+ nodes)

3. **Graph Generation Integration**
   - NetGameSim provides realistic graph topologies
   - Configurable parameters enable systematic experimentation
   - Future: Add graph comparison and validation tools

### Future Improvements

1. **Asynchronous Dijkstra**
   - Remove synchronization barrier between iterations
   - Use message queues and out-of-order processing
   - Expected speedup: 2-5x for sparse graphs

2. **Automatic Termination Detection**
   - Implement distributed convergence detection for leader election
   - Eliminate need for explicit round parameter
   - Use message counting and phase synchronization

3. **Graph-Aware Partitioning**
   - Integrate METIS or similar for balanced partitions
   - Consider edge-cut minimization
   - Expected improvement: 20-40% reduction in cross-partition messages

4. **Fault Tolerance**
   - Add timeout and retry logic for MPI operations
   - Handle rank failures gracefully
   - Support for dynamic rank addition/removal

---

## Experiments

### Experiment 1: 100 Nodes / 5 Ranks

**Objective:** Evaluate algorithm performance with moderate graph size and multiple partitions.

**Setup:**
- Graph: `experiments/graphs/exp1_100nodes.json` (100 nodes)
- Partition: `experiments/graphs/exp1_100nodes_part.json` (5 ranks)
- Leader Election: 30 rounds

**Results:**
TODO: Run `./experiments/experiment1.sh` and record results.

### Experiment 2: 50 Nodes / 2 Ranks

**Objective:** Verify correctness on smaller graph with fewer partitions.

**Setup:**
- Graph: `experiments/graphs/exp2_50nodes.json` (50 nodes)
- Partition: `experiments/graphs/exp2_50nodes_part.json` (2 ranks)
- Leader Election: 50 rounds

**Results:**
TODO: Run `./experiments/experiment2.sh` and record results.

### Experiment 3: 200 Nodes / 10 Ranks

**Objective:** Test scalability with large graph and many partitions.

**Setup:**
- Graph: `experiments/graphs/exp3_200nodes.json` (200 nodes)
- Partition: `experiments/graphs/exp3_200nodes_part.json` (10 ranks)
- Leader Election: 40 rounds

**Results:**
TODO: Run `./experiments/experiment3.sh` and record results.

---

## Conclusion

This project successfully implements a distributed MPI-based runtime for graph algorithms with two key algorithms: Distributed Dijkstra and Distributed Leader Election. The implementation demonstrates correct behavior across 22 unit tests covering various graph topologies and edge cases.

**Key Achievements:**
1. Implemented partition-based graph algorithm distribution
2. Achieved correct cross-partition communication for both algorithms
3. Developed comprehensive test suite with high code coverage
4. Integrated with NetGameSim for realistic graph generation

**Limitations:**
1. Static partition configuration (no dynamic rebalancing)
2. Blocking MPI calls limit parallelism
3. No fault tolerance or error recovery

**Future Work:**
- Asynchronous algorithm variants for improved performance
- Automatic termination detection for leader election
- Graph-aware partitioning for reduced communication
- Support for dynamic graph updates

---

## Appendix

### File Structure
```
mpi_runtime/
├── CMakeLists.txt
├── Makefile
├── README.md
├── REPORT.md
├── configs/
│   └── defconfig.conf
├── src/
│   ├── main.cpp
│   ├── GraphData.cpp
│   ├── GraphData.h
│   ├── DistributedDijkstra.cpp
│   ├── DistributedDijkstra.h
│   ├── DistributedLeaderElection.cpp
│   ├── DistributedLeaderElection.h
│   └── DistributedAlgorithm.h
├── tests/
│   ├── test_dijkstra.cpp
│   ├── test_leaderelection.cpp
│   └── test_graphs/
│       ├── testgraph1.json
│       ├── testpart1.json
│       ├── simple_graph.json
│       ├── simple_part.json
│       ├── chain_graph.json
│       └── chain_part.json
├── tools/
│   ├── graph_export/
│   │   ├── run.sh
│   │   └── enrichment.py
│   └── partition/
│       ├── run.sh
│       └── partition.py
├── experiments/
│   ├── experiment1.sh
│   ├── experiment2.sh
│   ├── experiment3.sh
│   └── graphs/
│       ├── exp1_100nodes.json
│       ├── exp1_100nodes_part.json
│       ├── exp2_50nodes.json
│       ├── exp2_50nodes_part.json
│       ├── exp3_200nodes.json
│       └── exp3_200nodes_part.json
└── NetGameSim/
    └── (NetGameSim library and tools)
```

### Build Instructions

```bash
cd mpi_runtime

# Configure and build
cmake -B build
cmake --build build

# Run all tests
make run_test

# Run experiments
./experiments/experiment1.sh
./experiments/experiment2.sh
./experiments/experiment3.sh
```

### References

1. Dijkstra, E. W. (1959). "A note on two problems in connexion with graphs". Numerische Mathematik. 1: 269-271.

2. MPI Forum. (2021). "MPI: A Message-Passing Interface Standard Version 4.0". https://www.mpi-forum.org/docs/mpi-4.0/mpi40-report.pdf

3. Tel, G. (2000). "Introduction to Distributed Algorithms" (2nd ed.). Cambridge University Press.

4. Grechanik, M. (2023). "NetGameSim: Network Graph Simulation Experimental Platform". https://github.com/0x1DOCD00D/NetGameSim

5. nlohmann/json: JSON for Modern C++. https://github.com/nlohmann/json

6. Google Test: Google Testing and Mocking Framework. https://github.com/google/googletest
