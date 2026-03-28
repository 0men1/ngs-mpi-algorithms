#include "DistributedLeaderElection.h"
#include "GraphData.h"
#include <mpi.h>
#include <unordered_map>


void DistributedLeaderElection::execute(GraphData& graph) {
	int rank, rankSize;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &rankSize);

	std::unordered_map<int, int> currentMax; // Read
	std::unordered_map<int, int> nextMax; // Write

	for (const auto& [u, edge] : graph.m_adjList) {
		currentMax[u] = u;
		nextMax[u] = u;
	}

	for (int round = 0; round < m_rounds; round++) {
		std::vector<std::vector<ElectMsg>> outMsgs(rankSize);
		std::vector<int> sendCounts(rankSize);

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
		MPI_Alltoall(sendCounts.data(), 1, MPI_INT, recvCounts.data(), 1, MPI_INT, MPI_COMM_WORLD);

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

		MPI_Alltoallv(sendBuf.data(), sendCounts.data(), sdispls.data(), MPI_BYTE, recvBuf.data(), recvCounts.data(), rdispls.data(), MPI_BYTE, MPI_COMM_WORLD);

		for (const auto& msg : recvBuf) {
			nextMax[msg.destNode] = std::max(nextMax[msg.destNode], msg.maxNodeId);
		}
		currentMax = nextMax;
	}

	for (const auto& [node, maxNode] : currentMax) {
		printf("%d elects %d\n", node, maxNode);
	}
}


void DistributedLeaderElection::reportMetrics() const {
	printf("Printing metrics");
}
