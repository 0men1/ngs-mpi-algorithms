#include <chrono>
#include <limits>
#include <vector>
#include "DistributedLeaderElection.h"
#include "GraphData.h"
#include "mpi_utils.h"

void DistributedLeaderElection::execute(GraphData& graph) {
    auto start = std::chrono::high_resolution_clock::now();

    int rank, rankSize;
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &rankSize));

    std::map<int, int> currentMax;
    std::map<int, int> nextMax;

    m_numIterations = 0;
    m_numMessages = 0;
    m_bytesSent = 0;

    for (const auto& [u, edge] : graph.m_adjList) {
        currentMax[u] = u;
        nextMax[u] = u;
    }

    for (int round = 0; round < m_rounds; round++) {
        std::vector<std::vector<ElectMsg>> outMsgs(rankSize);
        std::vector<int> sendCounts(rankSize, 0);

        for (const auto & [u, edges] : graph.m_adjList) {
            int u_max = currentMax[u];

            for (const Edge& edge : edges) {
                int v = edge.dest;
                int v_rank = graph.m_nodeOwnership[v];

                if (rank == v_rank) {
                    nextMax[v] = std::max(u_max, nextMax[v]);
                } else {
                    outMsgs[v_rank].push_back({v, u_max});
                    sendCounts[v_rank]++;
                }
            }
        }

        std::vector<int> recvCounts(rankSize, 0);
        MPI_CHECK(MPI_Alltoall(sendCounts.data(), 1, MPI_INT, recvCounts.data(), 1, MPI_INT, MPI_COMM_WORLD));
        
        m_numMessages += rankSize;
        m_bytesSent += rankSize * sizeof(int);

        std::vector<int> sdispls(rankSize, 0), rdispls(rankSize, 0);
        int totalSend = 0, totalRecv = 0;
        for (int i = 0; i < rankSize; ++i) {
            sdispls[i] = totalSend;
            totalSend += sendCounts[i] * sizeof(ElectMsg);
            sendCounts[i] *= sizeof(ElectMsg);

            rdispls[i] = totalRecv;
            totalRecv += recvCounts[i] * sizeof(ElectMsg);
            recvCounts[i] *= sizeof(ElectMsg);
        }

        std::vector<ElectMsg> sendBuf;
        sendBuf.reserve(totalSend / sizeof(ElectMsg));
        for (int i = 0; i < rankSize; ++i) {
            sendBuf.insert(sendBuf.end(), outMsgs[i].begin(), outMsgs[i].end());
        }

        std::vector<ElectMsg> recvBuf(totalRecv / sizeof(ElectMsg));

        MPI_CHECK(MPI_Alltoallv(sendBuf.data(), sendCounts.data(), sdispls.data(), MPI_BYTE, recvBuf.data(), recvCounts.data(), rdispls.data(), MPI_BYTE, MPI_COMM_WORLD));
        
        m_numMessages += rankSize;
        m_bytesSent += totalSend;

        for (const auto& msg : recvBuf) {
            nextMax[msg.destNode] = std::max(nextMax[msg.destNode], msg.maxNodeId);
        }
        currentMax = nextMax;
        m_numIterations++;
    }

    m_finalLeaders = currentMax;
    m_totalRuntime = std::chrono::high_resolution_clock::now() - start;
}

void DistributedLeaderElection::reportMetrics() const {
    int rank;
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    int globalMsgs = 0, globalBytes = 0;
    MPI_CHECK(MPI_Reduce(&m_numMessages, &globalMsgs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD));
    MPI_CHECK(MPI_Reduce(&m_bytesSent, &globalBytes, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD));

    int localMinLeader = std::numeric_limits<int>::max();
    int localMaxLeader = -1;

    for (const auto& [node, leader] : m_finalLeaders) {
        if (leader < localMinLeader) localMinLeader = leader;
        if (leader > localMaxLeader) localMaxLeader = leader;
    }

    if (m_finalLeaders.empty()) {
        localMinLeader = -1;
        localMaxLeader = -1;
    }

    int globalMinLeader = -1, globalMaxLeader = -1;
    MPI_CHECK(MPI_Reduce(&localMinLeader, &globalMinLeader, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD));
    MPI_CHECK(MPI_Reduce(&localMaxLeader, &globalMaxLeader, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD));

    if (rank == 0) {
        printf("\n================ LEADER ELECTION METRICS ================\n");
        printf("Total Runtime (s): %f\n", m_totalRuntime.count());
        printf("Total Rounds: %d\n", m_numIterations);
        printf("Total Payload Messages Sent: %d\n", globalMsgs);
        printf("Total Payload Bytes Sent: %d\n", globalBytes);
        printf("\n--- Agreement Validation ---\n");
        
        if (globalMinLeader == globalMaxLeader && globalMaxLeader != -1) {
            printf("STATUS: SUCCESS\n");
            printf("Agreed Leader ID: %d\n", globalMaxLeader);
        } else {
            printf("STATUS: FAILED\n");
            printf("Minimum Leader Registered: %d\n", globalMinLeader);
            printf("Maximum Leader Registered: %d\n", globalMaxLeader);
        }
        printf("=========================================================\n");
    }
}
