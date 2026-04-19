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

The project was developed as a coursework assignment for CS453 (Distributed Systems) at University of Illinois Chicago. The primary goal was to implement and verify distributed graph algorithms that can scale across multiple MPI processes while maintaining correctness and demonstrating effective communication patterns.

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

---

## Assumptions for Correctness

### Dijkstra Algorithm Assumptions

**CRITICAL: These assumptions are required for correctness**

1. **Positive Edge Weights**
   - All edge weights must be non-negative (w >= 0)

2. **Connected Graph**
   - The graph should be connected (or at least the source node should reach all target nodes)

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

4. **Bidirectional Communication**
   - For complete convergence, edges should be traversable in both directions
   - The partition must allow messages to flow across the graph

### General MPI Assumptions

1. **Reliable Communication**
   - MPI does not lose messages in normal operation

---

## Experimental Design

### Experiment 1: 100 Nodes / 5 Ranks

**Objective:** Evaluate algorithm performance with moderate graph size and multiple partitions.

**Setup:**
- Graph: `experiments/graphs/exp1_100nodes.json` (101 nodes)
- Partition: `experiments/graphs/exp1_100nodes_part.json` (5 ranks)
- Leader Election: 30 rounds

**Results:**
- **Leader Election:**
  - Total Runtime: 0.002027s
  - Total Rounds: 30
  - Total Payload Messages: 1500
  - Total Payload Bytes: 61560
  - STATUS: SUCCESS
  - Agreed Leader ID: 100
- **Dijkstra (source=0):**
  - Total Runtime: 0.003580s
  - Total Iterations: 101
  - Total P2P Messages: 188
  - Total P2P Bytes: 1952
  - All 101 nodes reached with finite distances

### Experiment 2: 50 Nodes / 2 Ranks

**Objective:** Verify correctness on smaller graph with fewer partitions.

**Setup:**
- Graph: `experiments/graphs/exp2_50nodes.json` (51 nodes)
- Partition: `experiments/graphs/exp2_50nodes_part.json` (2 ranks)
- Leader Election: 50 rounds

**Results:**
- **Leader Election:**
  - Total Runtime: 0.001305s
  - Total Rounds: 50
  - Total Payload Messages: 400
  - Total Payload Bytes: 26400
  - STATUS: SUCCESS
  - Agreed Leader ID: 50
- **Dijkstra (source=0):**
  - Total Runtime: 0.000893s
  - Total Iterations: 51
  - Total P2P Messages: 41
  - Total P2P Bytes: 512
  - All 51 nodes reached with finite distances

### Experiment 3: 200 Nodes / 10 Ranks

**Objective:** Test scalability with large graph and many partitions.

**Setup:**
- Graph: `experiments/graphs/exp3_200nodes.json` (201 nodes)
- Partition: `experiments/graphs/exp3_200nodes_part.json` (10 ranks)
- Leader Election: 40 rounds

**Results:**
- **Leader Election:**
  - Total Runtime: 0.171973s
  - Total Rounds: 40
  - Total Payload Messages: 8000
  - Total Payload Bytes: 228480
  - STATUS: SUCCESS
  - Agreed Leader ID: 200
- **Dijkstra (source=0):**
  - Total Runtime: 0.009428s
  - Total Iterations: 201
  - Total P2P Messages: 564
  - Total P2P Bytes: 5312
  - All 201 nodes reached with finite distances

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

---

## Conclusion

[TODO]

**Key Achievements:**
1. Implemented partition-based graph algorithm distribution
2. Achieved correct cross-partition communication for both algorithms
3. Developed comprehensive test suite with high code coverage
4. Integrated with NetGameSim for realistic graph generation

---

### References

1. Grechanik, M. (2023). "NetGameSim: Network Graph Simulation Experimental Platform". https://github.com/0x1DOCD00D/NetGameSim
2. nlohmann/json: JSON for Modern C++. https://github.com/nlohmann/json
3. Google Test: Google Testing and Mocking Framework. https://github.com/google/googletest
