/*
 * Leader Election Algorithm Tests
 * 
 * This file contains tests for the DistributedLeaderElection algorithm.
 * The algorithm uses a distributed approach to elect a leader node by propagating
 * the maximum node ID through the graph over multiple rounds.
 * 
 * Total Tests: 10
 */

#include "DistributedLeaderElection.h"
#include "GraphData.h"
#include <gtest/gtest.h>
#include <mpi.h>

const char *TEST_GRAPH_PATH = "tests/test_graphs/testgraph1.json";
const char *TEST_PART_PATH = "tests/test_graphs/testpart1.json";
const char *SIMPLE_GRAPH_PATH = "tests/test_graphs/simple_graph.json";
const char *SIMPLE_PART_PATH = "tests/test_graphs/simple_part.json";
const char *CHAIN_GRAPH_PATH = "tests/test_graphs/chain_graph.json";
const char *CHAIN_PART_PATH = "tests/test_graphs/chain_part.json";

class LeaderElectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
    }
    int rank, size;
};

// Test 1: Verifies the leader election algorithm converges without errors
// Runs the algorithm for 3 rounds and checks it completes successfully
TEST_F(LeaderElectionTest, LeaderElectionConverges) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    DistributedLeaderElection election(3);
    election.execute(graph);
    SUCCEED();
}

// Test 2: Verifies that the maximum node ID in the graph is elected as leader
// Runs for enough rounds to ensure full propagation, then checks all nodes
// agree on node 9 being the leader (the highest node ID in testgraph1.json)
TEST_F(LeaderElectionTest, LeaderElectionElectsMaxNodeAsLeader) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    int expectedLeader = 9;
    
    DistributedLeaderElection election(10);
    election.execute(graph);
    
    std::map<int, int> finalLeaders = election.getFinalLeaders();
    
    int leaderCount = 0;
    for (const auto& [node, leader] : finalLeaders) {
        if (leader == expectedLeader) {
            leaderCount++;
        }
    }
    
    EXPECT_EQ(leaderCount, finalLeaders.size());
}

// Test 3: Tests leader election on a simple 4-node graph
// Graph: 0 -- 1, 0 -- 2, 1 -- 3, 2 -- 3 (complete-ish bipartite)
// Partition: rank 0 owns nodes 0,1; rank 1 owns nodes 2,3
// Expected leader: node 3 (max ID)
TEST_F(LeaderElectionTest, LeaderElectionSimpleGraph) {
    std::string graphFile(SIMPLE_GRAPH_PATH);
    std::string partFile(SIMPLE_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    int expectedLeader = 3;
    
    DistributedLeaderElection election(5);
    election.execute(graph);
    
    std::map<int, int> finalLeaders = election.getFinalLeaders();
    
    for (const auto& [node, leader] : finalLeaders) {
        EXPECT_EQ(leader, expectedLeader) << "Node " << node << " has wrong leader";
    }
}

// Test 4: Tests leader election on a 5-node linear chain graph
// Graph: 0 -- 1 -- 2 -- 3 -- 4
// Partition: rank 0 owns nodes 0,1; rank 1 owns nodes 2,3,4
// Expected leader: node 4 (max ID, at the end of the chain)
TEST_F(LeaderElectionTest, LeaderElectionChainGraph) {
    std::string graphFile(CHAIN_GRAPH_PATH);
    std::string partFile(CHAIN_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    int expectedLeader = 4;
    
    DistributedLeaderElection election(5);
    election.execute(graph);
    
    std::map<int, int> finalLeaders = election.getFinalLeaders();
    
    for (const auto& [node, leader] : finalLeaders) {
        EXPECT_EQ(leader, expectedLeader) << "Node " << node << " has wrong leader";
    }
}

// Test 5: Verifies that each rank produces leaders for all nodes it owns
// Each partition should track leaders for exactly its owned nodes
TEST_F(LeaderElectionTest, AllNodesHaveLeader) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    DistributedLeaderElection election(3);
    election.execute(graph);
    
    std::map<int, int> finalLeaders = election.getFinalLeaders();
    EXPECT_EQ(finalLeaders.size(), graph.m_ownedNodes.size());
}

// Test 6: Verifies all nodes agree on the same leader value
// After sufficient rounds, all nodes should converge to the same leader
TEST_F(LeaderElectionTest, AllNodesAgreeOnSameLeader) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    
    DistributedLeaderElection election(10);
    election.execute(graph);
    
    std::map<int, int> finalLeaders = election.getFinalLeaders();
    
    int firstLeader = -1;
    for (const auto& [node, leader] : finalLeaders) {
        if (firstLeader == -1) {
            firstLeader = leader;
        } else {
            EXPECT_EQ(leader, firstLeader) << "Nodes disagree on leader";
        }
    }
}

// Test 7: Verifies the algorithm correctly tracks metrics
// Checks that message count and byte count are properly recorded
TEST_F(LeaderElectionTest, LeaderElectionMetrics) {
    std::string graphFile(SIMPLE_GRAPH_PATH);
    std::string partFile(SIMPLE_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    
    DistributedLeaderElection election(5);
    election.execute(graph);
    
    int numMsgs = election.getNumMessagesSent();
    int numBytes = election.getNumBytesSent();
    
    EXPECT_GE(numMsgs, 0);
    EXPECT_GE(numBytes, 0);
}

// Test 8: Verifies the algorithm tracks iteration count correctly
// The number of iterations should equal the number of rounds executed
TEST_F(LeaderElectionTest, LeaderElectionTracksIterations) {
    std::string graphFile(SIMPLE_GRAPH_PATH);
    std::string partFile(SIMPLE_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    
    int rounds = 5;
    DistributedLeaderElection election(rounds);
    election.execute(graph);
    
    int iterations = election.getNumIterations();
    EXPECT_EQ(iterations, rounds);
}

// Test 9: Verifies that leader is always a valid node ID in the graph
// The elected leader must be one of the nodes that exists in the graph
TEST_F(LeaderElectionTest, LeaderIsValidNodeId) {
    std::string graphFile(TEST_GRAPH_PATH);
    std::string partFile(TEST_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    int maxNodeId = 9;
    
    DistributedLeaderElection election(10);
    election.execute(graph);
    
    std::map<int, int> finalLeaders = election.getFinalLeaders();
    
    for (const auto& [node, leader] : finalLeaders) {
        EXPECT_GE(leader, 0) << "Leader ID cannot be negative";
        EXPECT_LE(leader, maxNodeId) << "Leader ID exceeds maximum node ID";
    }
}

// Test 10: Verifies leader election with maximum rounds
// Running for many rounds should still produce correct results
TEST_F(LeaderElectionTest, LeaderElectionWithMaxRounds) {
    std::string graphFile(SIMPLE_GRAPH_PATH);
    std::string partFile(SIMPLE_PART_PATH);
    GraphData graph(rank, graphFile, partFile);
    int expectedLeader = 3;
    
    DistributedLeaderElection election(100);
    election.execute(graph);
    
    std::map<int, int> finalLeaders = election.getFinalLeaders();
    
    for (const auto& [node, leader] : finalLeaders) {
        EXPECT_EQ(leader, expectedLeader) 
            << "Node " << node << " has leader " << leader 
            << " but expected " << expectedLeader;
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    int result = RUN_ALL_TESTS();
    MPI_Finalize();
    return result;
}
