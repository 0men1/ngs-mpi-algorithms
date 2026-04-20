# MPI Distributed Graph Algorithm Runtime - Technical Report

## Table of Contents
1. [Executive Summary](#1-executive-summary)
2. [Architectural Design](#2-architectural-design)
3. [Implementation Details](#3-implementation-details)
4. [Testing Methodology](#4-testing-methodology)
5. [Determinism and Stability](#5-determinism-and-stability)
6. [Algorithm Choices](#6-algorithm-choices)
7. [Data Formats](#7-data-formats)
8. [Assumptions for Correctness](#8-assumptions-for-correctness)
9. [Experimental Design and Analysis](#9-experimental-design-and-analysis)
10. [Key Insights and Decisions](#10-key-insights-and-decisions)
11. [Conclusion](#11-conclusion)

---

## Executive Summary

This project implements a distributed MPI-based runtime for executing graph algorithms on partitioned graphs. The system supports two primary algorithms: Distributed Dijkstra (shortest path) and Distributed Leader Election. The implementation distributes graph nodes across MPI ranks, with each rank maintaining local graph data and communicating via message passing.

The project was developed as a coursework assignment for CS453 (Distributed Systems) at the University of Illinois Chicago. The primary goal was to implement and verify distributed graph algorithms that can scale across multiple MPI processes while maintaining correctness and demonstrating effective communication patterns.

**Key Contributions:**
- Implementation of a partition-based distributed Dijkstra algorithm with cross-partition boundary handling.
- Implementation of a synchronous, flooding-based distributed leader election algorithm utilizing dynamic MPI buffer allocation.
- Comprehensive Google Test suite covering correctness and edge cases.
- Strict enforcement of application determinism via controlled RNG seeding and ordered data structures.
- Integration with NetGameSim for large-scale connected graph generation.

---

## Architectural Design

### System Overview

The runtime follows a partition-based distributed computing model where:
1. A global graph is generated and partitioned across N MPI processes.
2. Each process parses the global file but only retains its owned subset of nodes and their boundary edges in local memory.
3. Execution proceeds via MPI point-to-point (`MPI_Send`/`MPI_Recv`) and collective (`MPI_Alltoallv`, `MPI_Allreduce`) operations.
4. Results are aggregated at Rank 0 for validation and logging.

### Component Architecture

```text
+------------------------------------------------------------------+
|                          Main Runtime                            |
+------------------------------------------------------------------+
|                                                                  |
|  +-------------------+      +-------------------+                |
|  |   GraphData       |      | DistributedAlg    |                |
|  | ------------------|      | (Abstract Base)   |                |
|  | - m_ownedNodes    |      | + execute()       |                |
|  | - m_adjList       |      | + reportMetrics() |                |
|  | - m_incomingEdges |      +--------+----------+                |
|  | - m_nodeOwnership |               |                           |
|  +-------------------+        +------v----------+                |
|                               |                 |                |
|                 +-------------v+    +-----------v----+           |
|                 | Dijkstra     |    | LeaderElection |           |
|                 +--------------+    +----------------+           |
+------------------------------------------------------------------+
                          MPI Communication Layer
                  (MPI_Send/Recv, MPI_Alltoallv, MPI_Allreduce)
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

### Partitioning Strategy

The graph is partitioned by node ownership. The partition file specifies which rank owns each node. This design choice ensures:

1. Each rank has clear ownership of specific nodes
2. Message routing is deterministic based on node ownership
3. Workload distribution can be analyzed via partition statistics

**Partition Balancing Considerations:**
The default partitioning strategy uses a round-robin approach: `rank = (nodeId * numRanks) / numNodes`. This provides reasonably balanced partitions for random graphs.

---

## Implementation Details

**Ghost Node Allocation Strategy**
Ghost nodes are not instantiated as standalone memory objects. Instead, they are mapped dynamically via adjacency structures during the parsing phase. During loadData, the global JSON graph is parsed. When an edge originates from a locally owned node but targets a remote node (m_nodeOwnership[dst] != m_rankId), the remote node acts as a boundary ghost node. This boundary classification triggers targeted point-to-point MPI routing for Dijkstra updates and dictates out-of-partition message buffering for Leader Election.


---

## Determinism and Stability

Strict determinism is enforced across the system to guarantee absolute reproducibility, avoiding the automatic zero condition specified in the assignment constraints.

1. **Graph Generation Phase: ** The Python enrichment script uses a pseudo-random number generator initialized with an explicit CLI seed (random.seed(seed)). This ensures the stochastic distribution of positive edge weights is identical across consecutive runs using the same input configuration.

2. **Runtime Phase: ** Implicit variability derived from memory layouts and hashing algorithms is explicitly eliminated. All internal C++ containers requiring iteration (e.g., m_adjList, currentMax, nextMax) strictly utilize ordered std::map instead of std::unordered_map. This guarantees stable execution ordering over nodes and edges, locking the MPI message serialization sequence to the exact same byte order on every run.

---

## Testing Methodology

The system utilizes the Google Test framework wrapped in MPI context (MPI_Init/MPI_Finalize) to execute 29 isolated unit and integration tests across multiple distributed ranks. Test execution targets deterministic, pre-computed JSON graph topologies—including a 10-node weighted complex graph, a 4-node complete bipartite graph, and a 5-node linear chain to guarantee predictable message routing and validation baselines.

**Distributed Dijkstra Validation (19 Tests):**
    - **Distance Verification:** Core correctness is enforced by asserting the runtime-computed distance vector against mathematically derived shortest-path distances. Tests iterate through variable source nodes (e.g., origin node 0 vs. origin node 5) to guarantee routing logic is origin-agnostic.

    - **Invariant Enforcement:** Verifies algorithm preconditions, asserting that all edge weights across standard and ghost-node boundaries are strictly non-negative.

    - **Reachability:** Asserts that all nodes owned by a local rank resolve to finite distances to guarantee graph traversal connectivity.

    - **Diagnostic Auditing:** Validates telemetry accumulators for point-to-point message counts and transmitted byte payloads.

**Distributed Leader Election Validation (10 Tests):**

    - **Global Consensus:** Correctness is defined by unanimous agreement. Tests assert that the final map of elected leaders on every rank resolves exclusively to the highest integer node ID present in the global graph.

    - **Boundary Validation:** Ensures the computed leader ID is a valid node within the graph structure and restricts local ranks to only tracking states for their assigned partitions.

    - **Convergence Stability:** Tests standard execution thresholds against extreme round limits (e.g., 100 rounds) to verify memory stability and proper termination of the flooding mechanism.

    - **Metric Accuracy:** Asserts iteration counts explicitly match the round configurations alongside collective network telemetry.

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

---

## Data Formats

### Graph Input Format (JSON)

The graph is stored in JSON format with the following structure:

```json
{
  "metadata": {
    "num_nodes": <integer>,  // Total number of nodes
    "seed": <integer>        // Seed used for graph generation
  },
  "adjacency_list": {
    "<node_id>": [            // Node ID as string key
      {
        "v": <integer>,     // Destination node ID
        "w": <float>         // Edge weight (positive for Dijkstra)
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
  "metadata": { "num_nodes": 4, "seed": 42 },
  "adjacency_list": {
    "0": [{ "v": 1, "w": 0.5 }, { "v": 2, "w": 0.2 }],
    "1": [{ "v": 0, "w": 0.5 }],
    "2": [{ "v": 0, "w": 0.2 }, { "v": 3, "w": 0.4 }],
    "3": [{ "v": 2, "w": 0.4 }]
  }
}
```

### Partition Format (JSON)

The partition defines node ownership across (2) MPI ranks:

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

**Dijkstra UpdateMsg (8 bytes):**
```cpp
struct UpdateMsg {
    int nodeId;   // Target node
    float dist;   // New distance
};
```

**Leader Election ElectMsg (8 bytes):**
```cpp
struct ElectMsg {
    int destNode;  // Destination node
    int maxNodeId; // Maximum ID being propagated
};
```

---

## Assumptions for Correctness

### NetGameSim Initial Node

1. **NetGameSim Initial Node:** NetGameSim adds an extra "initial node" (node 0) to the generated graph. For a config with `statesTotal = N`, the output will contain N+1 nodes (IDs 0 to N). This initial node serves as an entry point for graph traversal and ensures connectivity.

2. **Positive Edge Weight:** Dijkstra's correctness intrinsically assumes all edge weights are strictly positive ($w \ge 0$). This is enforced during the Python enrichment phase.

3. **Unique Node IDs:** Leader election relies on comparing integers. Node IDs must be strictly unique to prevent collisions during maximum value aggregation.

4. **Round Configuration:** The Leader Election algorithm assumes the user-supplied --rounds parameter is greater than or equal to the mathematical diameter of the underlying graph geometry.

---

## Experimental Design and Analysis

**Reproducibility Commands:**
All experiments utilize pre-generated deterministic graphs stored within the `experiments/graphs/` directory. Execution is automated via shell scripts that validate file paths and trigger the MPI runtime.

```bash
# Reproduce Experiment 1 (101 Nodes / 5 Ranks)
./experiments/experiment1.sh

# Reproduce Experiment 2 (51 Nodes / 2 Ranks)
./experiments/experiment2.sh

# Reproduce Experiment 3 (201 Nodes / 10 Ranks)
./experiments/experiment3.sh
```

### Experiment 1: 101 Nodes / 5 Ranks

**Objective:** Evaluate algorithm performance with moderate graph size and multiple partitions.

**Experimental Hypothesis:** The distribution of a 101-node graph across 5 ranks will establish the baseline latency. Dijsktra's iteration count will equal the total node count. Leader election will correlate strictly to the graph's diameter.

**Expected Results:** Dijkstra will require 101 iterations to settle all the nodes in the graph. Leader election will terminate successfully within the 30-round limit if the diameter is 30 or less.

**Setup:**
- Graph: `experiments/graphs/exp1_100nodes_graph.json` (101 nodes)
- Partition: `experiments/graphs/exp1_100nodes_part.json` (5 ranks)

**Results:**
- **Leader Election:**
  - Total Runtime: 0.002169s
  - Total Rounds: 30
  - Total Payload Messages: 1500
  - Total Payload Bytes: 62040
  - STATUS: SUCCESS
  - Agreed Leader ID: 100
- **Dijkstra (source=0):**
  - Total Runtime: 0.001063s
  - Total Iterations: 101
  - Total P2P Messages: 186
  - Total P2P Bytes: 1968
  - All 101 nodes reached with finite distances

**Explanation:** Empirical results perfectly match theoretical baselines. Dijkstra executed 101 operations to settle every node. All ranks were able to agree on leader ID 30 showing that the diameter is 30 or less.

### Experiment 2: 51 Nodes / 2 Ranks

**Objective:** Verify correctness on smaller graph with fewer partitions.

**Experimental Hypothesis:** Decreasing the rank allocation from 5 to 2 should heavily increase local processing loads but reduce inter-rank message volume.

**Expected Results:** P2P messaging overhead for Dijkstra will drop compared to Experiment 1. Total payload messages for Leader Election will reflect a smaller 2-rank collective model.

**Setup:**
- Graph: `experiments/graphs/exp2_50nodes_graph.json` (51 nodes)
- Partition: `experiments/graphs/exp2_50nodes_part.json` (2 ranks)

**Results:**
- **Leader Election:**
  - Total Runtime: 0.001294s
  - Total Rounds: 50
  - Total Payload Messages: 400
  - Total Payload Bytes: 25600
  - STATUS: SUCCESS
  - Agreed Leader ID: 50
- **Dijkstra (source=0):**
  - Total Runtime: 0.000935s
  - Total Iterations: 51
  - Total P2P Messages: 43
  - Total P2P Bytes: 496
  - All 51 nodes reached with finite distances

**Explanation:** The hypothesis is confirmed. P2P message volume for Dijkstra fell steeply from 186 down to 43. The total payload messages for leader election dropped from 1500 to 400.

### Experiment 3: 201 Nodes / 10 Ranks

**Objective:** Test scalability with large graph and many partitions.

**Experimental Hypothesis:** Expanding to 10 ranks will cause an increase in MPI collective message counts compared to smaller clusters.

**Expected Results:** Dijkstra will execute ~201 global reductions. Leader election will generate a spike in payload messages due to N * N rank communication.

**Setup:**
- Graph: `experiments/graphs/exp3_200nodes_graph.json` (201 nodes)
- Partition: `experiments/graphs/exp3_200nodes_part.json` (10 ranks)

**Results:**
- **Leader Election:**
  - Total Runtime: 0.013432s
  - Total Rounds: 40
  - Total Payload Messages: 8000
  - Total Payload Bytes: 198400
  - STATUS: SUCCESS
  - Agreed Leader ID: 200
- **Dijkstra (source=0):**
  - Total Runtime: 0.007362s
  - Total Iterations: 201
  - Total P2P Messages: 505
  - Total P2P Bytes: 4560
  - All 201 nodes reached with finite distances

**Explanation:** Message transfer across 10 ranks forced the payload message count to spike 8000 for leader election. Despite the heavy network scaling, the distributed state remained consistent, successfully computing accurate shortest paths and electing the correct global maximum node ID.

---

## Key Insights and Decisions

### Design Decisions

1. **Separate Edge Storage for Send/Receive**
   - Decision: Maintain `m_adjList` (outgoing) and `m_incomingEdges` (incoming)
   - Rationale: Different algorithms require different directional views. Dijkstra primarily pushes state forward along outgoing edges, but processing incoming ghost-node boundaries safely required tracking inverse topology.

2. **Getter Methods for Testing**
   - Decision: Added `getDistances()`, `getFinalLeaders()`, `getNumMessagesSent()` etc.
   - Rationale: Enable white-box testing of internal algorithm state

3. **Partition File Format**
   - Decision: JSON-based partition specification
   - Rationale: Human-readable, easy to generate and modify

4. **Command-line Interface**
   - Decision: Named arguments with `--` prefix (e.g., `--graph`, `--algo`)
   - Rationale: Self-documenting, order-independent argument parsing

---

## Conclusion

This project successfully demonstrates a highly functional distributed MPI runtime for evaluating classic graph algorithms. The implementation handles strict architectural requirements including partition-based memory isolation, inter-partition message routing, and convergence verification.

By enforcing strict determinism across Python graph generation and C++ internal data structures, the system guarantees 100% reproducible execution environments. The experimental results validate that both Distributed Dijkstra and FloodMax Leader Election scale logically with rank allocation, balancing local compute bounds against necessary MPI network overhead.

---

### References

1. Grechanik, M. (2023). "NetGameSim: Network Graph Simulation Experimental Platform". https://github.com/0x1DOCD00D/NetGameSim
2. nlohmann/json: JSON for Modern C++. https://github.com/nlohmann/json
3. Google Test: Google Testing and Mocking Framework. https://github.com/google/googletest
