#include <chrono>
#include <cstdio>
#include <limits>
#include <vector>
#include <mpi.h>
#include "DistributedDijkstra.h"
#include "GraphData.h"

void DistributedDijkstra::execute(GraphData& graph) {
	auto start = std::chrono::high_resolution_clock::now();

	int rank, rankSize;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &rankSize);

	int numNodes = graph.m_nodeOwnership.size();

	std::vector<float> distance(numNodes, std::numeric_limits<float>::infinity());
	std::vector<bool> settled(numNodes, false);

	if (graph.m_nodeOwnership[m_sourceNode] == rank) {
		distance[m_sourceNode] = 0.0;
	}

	struct {
		float distance;
		int nodeId;
	} local_min, global_min;

	while (true) {
		local_min.distance = std::numeric_limits<float>::infinity();
		local_min.nodeId = -1;

		for (const auto & [u, edges] : graph.m_adjList) {
			if (!settled[u] && (distance[u] < local_min.distance)) {
				local_min.distance = distance[u];
				local_min.nodeId = u;
			}
		}

		MPI_Allreduce(&local_min, &global_min, 1, MPI_FLOAT_INT, MPI_MINLOC, MPI_COMM_WORLD);

		if (global_min.distance == std::numeric_limits<float>::infinity()) {
		    break;
		}

		int globalNode = global_min.nodeId;
		int globalRank = graph.m_nodeOwnership[globalNode];

		settled[globalNode] = true;

		std::vector<int> sendCounts(rankSize, 0);
		std::vector<std::vector<UpdateMsg>> outgoingUpdates(rankSize);
		if (globalRank == rank) {
			for (Edge edge : graph.m_adjList[globalNode]) {
				float newDist = global_min.distance + edge.weight;
				int edgeOwnerRank = graph.m_nodeOwnership[edge.dest];

				if (edgeOwnerRank == rank) {
					if (newDist < distance[edge.dest]) {
						distance[edge.dest] = newDist;
					}
				} else {
					outgoingUpdates[edgeOwnerRank].push_back({edge.dest, newDist});
					sendCounts[edgeOwnerRank]++;
					m_numMessages++;
				}
			}
		}

		MPI_Bcast(sendCounts.data(), rankSize, MPI_INT, globalRank, MPI_COMM_WORLD);

		if (globalRank == rank) {

			for (int r =0; r < rankSize; r++) {
				if (r != rank && sendCounts[r] > 0) {
					MPI_Send(outgoingUpdates[r].data(), sendCounts[r] * sizeof(UpdateMsg), MPI_BYTE, r, 0, MPI_COMM_WORLD);
				}
			}
		} else if (sendCounts[rank] > 0) {
			std::vector<UpdateMsg> incMsgs(sendCounts[rank]);
			MPI_Recv(incMsgs.data(), sendCounts[rank] * sizeof(UpdateMsg), MPI_BYTE, globalRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			for (const auto& msg : incMsgs) {
				if (msg.dist < distance[msg.nodeId]) {
					distance[msg.nodeId] = msg.dist;
				}
			}
		}

		m_numIterations++;
	}
	m_totalRuntime = std::chrono::high_resolution_clock::now() - start;
}

void DistributedDijkstra::reportMetrics() const {
	printf("# Iterations: %d\n", m_numIterations);
	printf("# Messages: %d\n", m_numMessages);
	printf("Total Runtime: %f\n", m_totalRuntime.count());
}
