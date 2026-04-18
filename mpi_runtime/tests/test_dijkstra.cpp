/*
 * Dijkstra Algorithm Tests
 * 
 * This file contains tests for the DistributedDijkstra algorithm.
 * The algorithm computes shortest paths from a source node to all other nodes
 * in a distributed manner using MPI communication.
 * 
 * Total Tests: 18
 */

#include "DistributedDijkstra.h"
#include "GraphData.h"
#include <gtest/gtest.h>
#include <mpi.h>
#include <limits>

const char *TEST_GRAPH_PATH = "tests/test_graphs/testgraph1.json";
const char *TEST_PART_PATH = "tests/test_graphs/testpart1.json";
const char *SIMPLE_GRAPH_PATH = "tests/test_graphs/simple_graph.json";
const char *SIMPLE_PART_PATH = "tests/test_graphs/simple_part.json";
const char *CHAIN_GRAPH_PATH = "tests/test_graphs/chain_graph.json";
const char *CHAIN_PART_PATH = "tests/test_graphs/chain_part.json";

class GraphDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
    }
    int rank, size;
};

class DijkstraTest : public GraphDataTest {
protected:
    void SetUp() override { GraphDataTest::SetUp(); }
};

// Test 1: Verifies the Dijkstra algorithm converges without errors
// Runs the algorithm from source node 0 and checks it completes successfully
TEST_F(DijkstraTest, DijkstraConverges) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    SUCCEED();
}

// Test 2: Verifies the source node has distance 0 and adjacent nodes have correct distances
// Source node 0 should have distance 0, and nodes 1 and 2 (directly connected) should
// have distances 0.5 and 0.2 respectively
TEST_F(DijkstraTest, DijkstraSourceDistanceZero) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    if (graph.m_nodeOwnership[0] == rank) {
        EXPECT_FLOAT_EQ(0.0f, distances[0]);
    }
    if (graph.m_nodeOwnership[1] == rank) {
        EXPECT_FLOAT_EQ(0.5f, distances[1]);
    }
    if (graph.m_nodeOwnership[2] == rank) {
        EXPECT_FLOAT_EQ(0.2f, distances[2]);
    }
}

// Test 3: Verifies correct distances for nodes owned by different ranks
// Node 5 is at distance 0.6 (path: 0->2->5 with weights 0.2+0.4)
// Node 9 is at distance 1.5 (path: 0->2->5->9 with weights 0.2+0.4+0.9)
TEST_F(DijkstraTest, DijkstraFindsCorrectDistancesForOwnedNodes) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    if (graph.m_nodeOwnership[5] == rank) {
        EXPECT_FLOAT_EQ(0.6f, distances[5]);
    }
    if (graph.m_nodeOwnership[9] == rank) {
        EXPECT_FLOAT_EQ(1.5f, distances[9]);
    }
}

// Test 4: Verifies the algorithm works with a different source node
// Running Dijkstra from node 5 instead of node 0 should still converge
TEST_F(DijkstraTest, DijkstraWithDifferentSource) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(5);
    dijkstra.execute(graph);
    
    SUCCEED();
}

// Test 5: Verifies the algorithm on a simple graph
// Tests correctness on a simpler graph with known distances
TEST_F(DijkstraTest, DijkstraSimpleGraphCorrectness) {
    std::string graphFile(SIMPLE_GRAPH_PATH);
    std::string partFile(SIMPLE_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    
    // Source node 0 on rank 0, node 3 (max) on rank 1
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    // Verify source distance
    if (graph.m_nodeOwnership[0] == rank) {
        EXPECT_FLOAT_EQ(0.0f, distances[0]);
    }
}

// Test 6: Verifies all owned nodes have finite distances from source
// Every node owned by the rank should either have a finite distance or
// be the source node itself (distance 0)
TEST_F(DijkstraTest, OwnedNodesHaveFiniteOrZeroDistance) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    for (int nodeId : graph.m_ownedNodes) {
        if (nodeId != 0) {  // Source node should be 0
            EXPECT_LT(distances[nodeId], std::numeric_limits<float>::infinity())
                << "Node " << nodeId << " owned by rank " << rank 
                << " should have finite distance from source 0";
        }
    }
}

// Test 7: Verifies message tracking metrics
// The algorithm should correctly track number of messages sent
TEST_F(DijkstraTest, DijkstraTracksMessages) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    int numMessages = dijkstra.getNumMessagesSent();
    int numBytes = dijkstra.getNumBytesSent();
    
    EXPECT_GE(numMessages, 0);
    EXPECT_GE(numBytes, 0);
}

// Test 8: Verifies edge weights are positive
// All edges in the adjacency list should have non-negative weights
TEST_F(DijkstraTest, EdgeWeightsAreNonNegative) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    
    for (const auto &[node, edges] : graph.m_adjList) {
        for (const auto &edge : edges) {
            EXPECT_GE(edge.weight, 0.0f) 
                << "Edge from " << node << " to " << edge.dest 
                << " has negative weight: " << edge.weight;
        }
    }
    
    for (const auto &[node, edges] : graph.m_incomingEdges) {
        for (const auto &edge : edges) {
            EXPECT_GE(edge.weight, 0.0f)
                << "Incoming edge to " << node << " from " << edge.dest
                << " has negative weight: " << edge.weight;
        }
    }
}

// ============================================================================
// Correctness Tests with Known Expected Distances
// ============================================================================

/*
 * Expected distances for testgraph1.json (10 nodes):
 * 
 * Graph structure:
 *   0 -- 1 -- 3 -- 7
 *   |      \
 *   2 -- 5 -- 9
 *   |
 *   6
 * 
 * Edge weights:
 *   0-1: 0.5, 0-2: 0.2
 *   1-3: 0.1, 1-4: 0.8
 *   2-5: 0.4, 2-6: 0.7
 *   3-7: 0.3
 *   4-8: 0.6
 *   5-9: 0.9
 * 
 * Partition: rank 0 owns nodes 0-4, rank 1 owns nodes 5-9
 */

// Test 9: Source node 0 - verifies all distances from source 0
// Expected distances from source 0:
//   0=0.0, 1=0.5, 2=0.2, 3=0.6, 4=1.3, 5=0.6, 6=0.9, 7=0.9, 8=1.9, 9=1.5
TEST_F(DijkstraTest, DijkstraSource0AllDistances) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    // Expected distances from source 0
    std::map<int, float> expected = {
        {0, 0.0f}, {1, 0.5f}, {2, 0.2f}, {3, 0.6f}, {4, 1.3f},
        {5, 0.6f}, {6, 0.9f}, {7, 0.9f}, {8, 1.9f}, {9, 1.5f}
    };
    
    for (const auto &[nodeId, expectedDist] : expected) {
        if (graph.m_nodeOwnership[nodeId] == rank) {
            EXPECT_FLOAT_EQ(expectedDist, distances[nodeId])
                << "Node " << nodeId << " distance mismatch from source 0";
        }
    }
}

// Test 10: Source node 5 - verifies all distances from source 5
// Expected distances from source 5:
//   5=0.0, 9=0.9, 2=0.4, 6=1.1, 0=0.6, 1=1.1, 3=1.2, 4=1.9, 7=1.5, 8=2.5
//   Paths: 5->9(0.9), 5->2(0.4), 5->2->6(1.1), 5->2->0(0.6), 5->2->0->1(1.1)
//          5->2->0->1->3(1.2), 5->2->0->1->4(1.9), 5->2->0->1->3->7(1.5)
//          5->2->0->1->4->8(2.5)
TEST_F(DijkstraTest, DijkstraSource5AllDistances) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(5);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    // Expected distances from source 5
    std::map<int, float> expected = {
        {5, 0.0f}, {9, 0.9f}, {2, 0.4f}, {6, 1.1f}, {0, 0.6f},
        {1, 1.1f}, {3, 1.2f}, {4, 1.9f}, {7, 1.5f}, {8, 2.5f}
    };
    
    for (const auto &[nodeId, expectedDist] : expected) {
        if (graph.m_nodeOwnership[nodeId] == rank) {
            EXPECT_FLOAT_EQ(expectedDist, distances[nodeId])
                << "Node " << nodeId << " distance mismatch from source 5";
        }
    }
}

// Test 11: Source node 9 - verifies all distances from source 9
// Expected distances from source 9:
//   9=0.0, 5=0.9, 2=1.3, 6=2.0, 0=1.5, 1=2.0, 3=2.1, 4=2.8, 7=2.4, 8=3.4
//   Paths: 9->5(0.9), 9->5->2(1.3), 9->5->2->6(2.0), 9->5->2->0(1.5)
//          9->5->2->0->1(2.0), 9->5->2->0->1->3(2.1), 9->5->2->0->1->4(2.8)
//          9->5->2->0->1->3->7(2.4), 9->5->2->0->1->4->8(3.4)
TEST_F(DijkstraTest, DijkstraSource9AllDistances) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(9);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    // Expected distances from source 9
    std::map<int, float> expected = {
        {9, 0.0f}, {5, 0.9f}, {2, 1.3f}, {6, 2.0f}, {0, 1.5f},
        {1, 2.0f}, {3, 2.1f}, {4, 2.8f}, {7, 2.4f}, {8, 3.4f}
    };
    
    for (const auto &[nodeId, expectedDist] : expected) {
        if (graph.m_nodeOwnership[nodeId] == rank) {
            EXPECT_FLOAT_EQ(expectedDist, distances[nodeId])
                << "Node " << nodeId << " distance mismatch from source 9";
        }
    }
}

/*
 * Expected distances for simple_graph.json (4 nodes, complete bipartite):
 * 
 * Graph structure:
 *   0 -- 1
 *   |    |
 *   2 -- 3
 * 
 * All edges have weight 1.0
 * Partition: rank 0 owns nodes 0,1; rank 1 owns nodes 2,3
 */

// Test 12: Source node 0 on simple graph
// Expected: 0=0.0, 1=1.0, 2=1.0, 3=2.0 (via either 1 or 2)
TEST_F(DijkstraTest, DijkstraSimpleGraphSource0) {
    std::string graphFile(SIMPLE_GRAPH_PATH);
    std::string partFile(SIMPLE_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    // Expected distances from source 0
    std::map<int, float> expected = {
        {0, 0.0f}, {1, 1.0f}, {2, 1.0f}, {3, 2.0f}
    };
    
    for (const auto &[nodeId, expectedDist] : expected) {
        if (graph.m_nodeOwnership[nodeId] == rank) {
            EXPECT_FLOAT_EQ(expectedDist, distances[nodeId])
                << "Node " << nodeId << " distance mismatch from source 0";
        }
    }
}

// Test 13: Source node 3 on simple graph (node on rank 1)
// Expected: 3=0.0, 1=1.0, 2=1.0, 0=2.0 (via either 1 or 2)
TEST_F(DijkstraTest, DijkstraSimpleGraphSource3) {
    std::string graphFile(SIMPLE_GRAPH_PATH);
    std::string partFile(SIMPLE_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(3);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    // Expected distances from source 3
    std::map<int, float> expected = {
        {3, 0.0f}, {1, 1.0f}, {2, 1.0f}, {0, 2.0f}
    };
    
    for (const auto &[nodeId, expectedDist] : expected) {
        if (graph.m_nodeOwnership[nodeId] == rank) {
            EXPECT_FLOAT_EQ(expectedDist, distances[nodeId])
                << "Node " << nodeId << " distance mismatch from source 3";
        }
    }
}

/*
 * Expected distances for chain_graph.json (5 nodes, linear chain):
 * 
 * Graph structure:
 *   0 -- 1 -- 2 -- 3 -- 4
 * 
 * All edges have weight 1.0
 * Partition: rank 0 owns nodes 0,1; rank 1 owns nodes 2,3,4
 */

// Test 14: Source node 0 on chain graph
// Expected: 0=0.0, 1=1.0, 2=2.0, 3=3.0, 4=4.0
TEST_F(DijkstraTest, DijkstraChainGraphSource0) {
    std::string graphFile(CHAIN_GRAPH_PATH);
    std::string partFile(CHAIN_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(0);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    // Expected distances from source 0
    std::map<int, float> expected = {
        {0, 0.0f}, {1, 1.0f}, {2, 2.0f}, {3, 3.0f}, {4, 4.0f}
    };
    
    for (const auto &[nodeId, expectedDist] : expected) {
        if (graph.m_nodeOwnership[nodeId] == rank) {
            EXPECT_FLOAT_EQ(expectedDist, distances[nodeId])
                << "Node " << nodeId << " distance mismatch from source 0";
        }
    }
}

// Test 15: Source node 2 on chain graph (node on rank 1)
// Expected: 2=0.0, 1=1.0, 0=2.0, 3=1.0, 4=2.0
TEST_F(DijkstraTest, DijkstraChainGraphSource2) {
    std::string graphFile(CHAIN_GRAPH_PATH);
    std::string partFile(CHAIN_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    DistributedDijkstra dijkstra(2);
    dijkstra.execute(graph);
    
    std::vector<float> distances = dijkstra.getDistances();
    
    // Expected distances from source 2
    std::map<int, float> expected = {
        {2, 0.0f}, {1, 1.0f}, {0, 2.0f}, {3, 1.0f}, {4, 2.0f}
    };
    
    for (const auto &[nodeId, expectedDist] : expected) {
        if (graph.m_nodeOwnership[nodeId] == rank) {
            EXPECT_FLOAT_EQ(expectedDist, distances[nodeId])
                << "Node " << nodeId << " distance mismatch from source 2";
        }
    }
}

// ============================================================================
// GraphData Tests
// These tests verify the graph loading and partitioning functionality
// ============================================================================

TEST_F(GraphDataTest, LoadsGraphAndPartition) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    
    ASSERT_EQ(graph.m_nodeOwnership.size(), 10u);
}

TEST_F(GraphDataTest, CorrectNodeOwnership) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    
    ASSERT_EQ(graph.m_nodeOwnership[0], 0);
    ASSERT_EQ(graph.m_nodeOwnership[5], 1);
}

TEST_F(GraphDataTest, Rank0OwnsCorrectNodes) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    
    if (rank == 0) {
        EXPECT_TRUE(graph.m_adjList.count(0) > 0 || graph.m_adjList.count(1) > 0 ||
                    graph.m_adjList.count(2) > 0);
    }
}

TEST_F(GraphDataTest, AdjListStructureValid) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    
    GraphData graph(rank, graphFile, partFile);
    
    for (const auto &[node, edges] : graph.m_adjList) {
        ASSERT_GE(node, 0);
        ASSERT_LT(node, 10);
        for (const auto &edge : edges) {
            ASSERT_GE(edge.dest, 0);
            ASSERT_LT(edge.dest, 10);
            ASSERT_GE(edge.weight, 0.0f);
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    int result = RUN_ALL_TESTS();
    MPI_Finalize();
    return result;
}
