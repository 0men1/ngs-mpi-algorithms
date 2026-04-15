# MPI Distributed Graph Algorithm Runtime - Technical Report

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Architectural Design](#architectural-design)
3. [Algorithm Choices](#algorithm-choices)
4. [Implementation Details](#implementation-details)
5. [Experimental Design](#experimental-design)
6. [Results and Analysis](#results-and-analysis)
7. [Key Insights and Decisions](#key-insights-and-decisions)

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

## Experimental Design

### Hypothesis

**Primary Hypothesis:**
The distributed Dijkstra algorithm will correctly compute shortest paths when graph partitions result in cross-partition edges, with each rank maintaining accurate distances for its owned nodes.

**Secondary Hypothesis:**
The leader election algorithm will converge to the maximum node ID across all partitions, with all ranks agreeing on the same leader after sufficient rounds.

### Test Cases

**Dijkstra Tests (4 tests):**
1. Convergence verification
2. Source distance verification (node 0 = 0.0)
3. Cross-partition distance verification (nodes 5, 9 on rank 1)
4. Alternative source node testing

**Leader Election Tests (7 tests):**
1. Convergence verification
2. Maximum node ID election
3. Simple graph verification
4. Chain graph verification
5. Ownership coverage
6. Cross-rank agreement
7. Metric tracking

### Expected Results

**Dijkstra:**
- All nodes reachable from source should have finite distances
- Distances should match theoretical shortest paths
- Source node should always have distance 0

**Leader Election:**
- All nodes should elect the maximum node ID as leader
- All ranks should agree on the same leader value
- Election should complete within graph diameter rounds

---

## Results and Analysis

### Test Results Summary

**Dijkstra Tests:** 4/4 PASSED
**Leader Election Tests:** 7/7 PASSED
**Total:** 11/11 PASSED

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
│   └── test_leaderelection.cpp
└── ../outputs/tests/
    ├── testgraph1.json
    ├── testpart1.json
    ├── simple_graph.json
    ├── simple_part.json
    ├── chain_graph.json
    └── chain_part.json
```

### Build Instructions

```bash
cmake -B build
cmake --build build
make run_test    # Run all tests with 2 MPI ranks
```

### References

**TODO: Add references and citations**
