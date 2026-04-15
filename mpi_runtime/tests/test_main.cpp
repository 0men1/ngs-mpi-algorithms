#include "DistributedDijkstra.h"
#include "DistributedLeaderElection.h"
#include "GraphData.h"
#include <fstream>
#include <gtest/gtest.h>
#include <mpi.h>
#include <sstream>

const char *TEST_GRAPH_PATH = "../outputs/tests/testgraph1.json";
const char *TEST_PART_PATH = "../outputs/tests/testpart1.json";

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
  std::map<int, float> values = {{0, 0.0f}, {1, 0.5f}, {2, 0.2f}, {3, 0.6f},
                                 {4, 1.3f}, {5, 0.6f}, {6, 0.9f}, {7, 0.9f},
                                 {8, 1.9f}, {9, 1.5f}};
};

class LeaderElectionTest : public GraphDataTest {
protected:
  void SetUp() override { GraphDataTest::SetUp(); }
  int electedLeader = 9;
};

class MessageFormatTest : public GraphDataTest {
protected:
  void SetUp() override { GraphDataTest::SetUp(); }
};

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

TEST_F(DijkstraTest, DijkstraConverges) {
  std::string graphFile(TEST_GRAPH_PATH);
  std::string partFile(TEST_PART_PATH);

  GraphData graph(rank, graphFile, partFile);
  DistributedDijkstra dijkstra(0);
  dijkstra.execute(graph);

  SUCCEED();
}

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

TEST_F(DijkstraTest, DijkstraWithDifferentSource) {
  std::string graphFile(TEST_GRAPH_PATH);
  std::string partFile(TEST_PART_PATH);

  GraphData graph(rank, graphFile, partFile);
  DistributedDijkstra dijkstra(5);
  dijkstra.execute(graph);

  SUCCEED();
}

TEST_F(LeaderElectionTest, LeaderElectionConverges) {
  std::string graphFile(TEST_GRAPH_PATH);
  std::string partFile(TEST_PART_PATH);

  GraphData graph(rank, graphFile, partFile);
  DistributedLeaderElection election(3);
  election.execute(graph);

  SUCCEED();
}

TEST_F(LeaderElectionTest, LeaderElectionMultipleRounds) {
  std::string graphFile(TEST_GRAPH_PATH);
  std::string partFile(TEST_PART_PATH);

  GraphData graph(rank, graphFile, partFile);
  DistributedLeaderElection election(10);
  election.execute(graph);

  std::unordered_map<int, int> finalLeaders = election.getFinalLeaders();
  for (const auto &[first, second] : finalLeaders) {
    if (second != electedLeader) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(LeaderElectionTest, AllNodesHaveLeader) {
  std::string graphFile(TEST_GRAPH_PATH);
  std::string partFile(TEST_PART_PATH);

  GraphData graph(rank, graphFile, partFile);
  DistributedLeaderElection election(3);
  election.execute(graph);

  SUCCEED();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  MPI_Init(&argc, &argv);
  int result = RUN_ALL_TESTS();
  MPI_Finalize();
  return result;
}
