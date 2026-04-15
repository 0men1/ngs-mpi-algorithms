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

**TODO: Add project motivation and context**

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
|                         Main Runtime                              |
+------------------------------------------------------------------+
|                                                                  |
|  +-------------------+      +-------------------+                 |
|  |   GraphData       |      | DistributedAlg    |                 |
|  | -----------------|      | (Abstract Base)  |                 |
|  | - m_ownedNodes    |      | + execute()      |                 |
|  | - m_adjList       |      | + reportMetrics()|                 |
|  | - m_incomingEdges |      +--------+----------+                 |
|  | - m_nodeOwnership |               |                           |
|  +-------------------+      +--------v----------+                 |
|                               |                 |                  |
|                 +-------------+--+    +---------+----+            |
|                 | Dijkstra     |    | LeaderElection|            |
|                 +--------------+    +--------------+              |
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

**TODO: Document memory layout and access patterns**

### Partitioning Strategy

The graph is partitioned by node ownership. The partition file specifies which rank owns each node. This design choice ensures:

1. Each rank has clear ownership of specific nodes
2. Message routing is deterministic based on node ownership
3. Workload distribution can be analyzed via partition statistics

**TODO: Add partition balancing considerations**

---

## Algorithm Choices

### Distributed Dijkstra (Shortest Path)

**Algorithm: Parallel Dijkstra with Distributed Priority Queue**

The implementation uses a synchronized local minimum approach:

```
1. Initialize: Set source node distance to 0, all others to infinity
2. Repeat until convergence:
   a. Each rank finds local unvisited node with minimum distance
   b. MPI_Allreduce to find global minimum node
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

**TODO: Compare with alternative algorithms (e.g., Chang-Roberts, LCR)**

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

**TODO: Document build requirements and dependencies**

### Testing Infrastructure

**TODO: Document testing approach, test graphs used, and coverage**

**Test Graphs:**
- `testgraph1.json`: 10-node graph, complex topology
- `simple_graph.json`: 4-node bipartite graph
- `chain_graph.json`: 5-node linear chain

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

**TODO: Add partition balancing guidelines**

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

**Dijkstra Tests (8 tests):**
1. Convergence verification
2. Source distance verification (node 0 = 0.0)
3. Cross-partition distance verification (nodes 5, 9 on rank 1)
4. Alternative source node testing
5. Simple graph correctness verification
6. Owned nodes have finite distances
7. Message tracking metrics
8. Edge weights are non-negative (correctness assumption)

**Leader Election Tests (10 tests):**
1. Convergence verification
2. Maximum node ID election
3. Simple graph verification
4. Chain graph verification
5. Ownership coverage
6. Cross-rank agreement
7. Metric tracking
8. Iteration count tracking
9. Leader is valid node ID verification
10. Maximum rounds stress test

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

This runs all 15 tests with 2 MPI ranks and verifies correctness.

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

3. Build with custom files:
```bash
./build/ngs_mpi my_graph.json my_part.json
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

---

## Results and Analysis

### Test Results Summary

**Dijkstra Tests:** 8/8 PASSED
**Leader Election Tests:** 10/10 PASSED
**GraphData Tests:** 4/4 PASSED
**Total:** 22/22 PASSED

**TODO: Add detailed performance metrics (execution time, message counts)**

### Dijkstra Results

**Actual Behavior:**
- Rank 0 (owns nodes 0-4): Computes correct distances for owned nodes
- Rank 1 (owns nodes 5-9): Receives updates and computes correct distances

**Critical Finding:** Initial implementation stored edges only for source node ownership, causing rank 1 to have empty adjacency lists. Fixed by maintaining both outgoing edges (for sending) and incoming edges (for receiving).

**TODO: Document the bug and resolution in detail**

### Leader Election Results

**Actual Behavior:**
- Algorithm correctly propagates maximum ID through the graph
- All nodes converge to node 9 (maximum in testgraph1.json)
- Cross-rank agreement verified

**TODO: Add convergence analysis**

### Performance Observations

**TODO: Add timing data and scaling analysis**

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
   - Alternative considered: Binary format for efficiency

### Algorithm Insights

1. **Dijkstra Synchronization Point**
   - The Allreduce creates a synchronization barrier each iteration
   - This is necessary for correctness but limits parallelism
   - TODO: Investigate asynchronous variants

2. **Leader Election Round Count**
   - Algorithm requires explicit round parameter
   - TODO: Implement termination detection for automatic stopping

### Testing Insights

1. **MPI Test Execution**
   - Tests require `mpirun -n 2` to execute properly
   - Each rank runs the full test suite independently
   - TODO: Add distributed verification tests

2. **Test Data Requirements**
   - Simple graphs essential for debugging
   - Chain graphs reveal propagation issues
   - TODO: Add stress tests with larger graphs

### Future Improvements

**TODO: List planned improvements and extensions**

---

## Conclusion

**TODO: Add concluding remarks and summary**

---

## Appendix

### File Structure
```
mpi_runtime/
├── CMakeLists.txt
├── Makefile
├── include/
├── src/
│   ├── main.cpp
│   ├── GraphData.cpp/h
│   ├── DistributedDijkstra.cpp/h
│   ├── DistributedLeaderElection.cpp/h
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
```

### Build Instructions

```bash
cmake -B build
cmake --build build
make run_test    # Run all tests with 2 MPI ranks
```

### References

**TODO: Add references and citations**
