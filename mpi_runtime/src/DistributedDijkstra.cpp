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

	std::vector<float> distances(numNodes, std::numeric_limits<float>::infinity());
	std::vector<bool> settled(numNodes, false);

	if (graph.m_nodeOwnership[m_sourceNode] == rank) {
		distances[m_sourceNode] = 0.0;
	}

	struct {
		float distance;
		int nodeId;
	} local_min, global_min;

	m_numIterations = 0;
	m_numMessages = 0;
	m_bytesSent = 0;

	while (true) {
		local_min.distance = std::numeric_limits<float>::infinity();
		local_min.nodeId = -1;

		for (const auto & [u, edges] : graph.m_adjList) {
			if (!settled[u] && (distances[u] < local_min.distance)) {
				local_min.distance = distances[u];
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
		std::vector<std::vector<UpdateMsg>> outMsgs(rankSize);
		if (globalRank == rank) {
			for (Edge edge : graph.m_adjList[globalNode]) {
				float newDist = global_min.distance + edge.weight;
				int edgeOwnerRank = graph.m_nodeOwnership[edge.dest];

				if (edgeOwnerRank == rank) {
					if (newDist < distances[edge.dest]) {
						distances[edge.dest] = newDist;
					}
				} else {
					outMsgs[edgeOwnerRank].push_back({edge.dest, newDist});
					sendCounts[edgeOwnerRank]++;
				}
			}
		}

		MPI_Bcast(sendCounts.data(), rankSize, MPI_INT, globalRank, MPI_COMM_WORLD);

		if (globalRank == rank) {

			for (int r =0; r < rankSize; r++) {
				if (r != rank && sendCounts[r] > 0) {
					int bytes = sendCounts[r] * sizeof(UpdateMsg);
					MPI_Send(outMsgs[r].data(), bytes, MPI_BYTE, r, 0, MPI_COMM_WORLD);
					m_numMessages++;
					m_bytesSent += bytes;
				}
			}
		} else if (sendCounts[rank] > 0) {
			std::vector<UpdateMsg> incMsgs(sendCounts[rank]);
			MPI_Recv(incMsgs.data(), sendCounts[rank] * sizeof(UpdateMsg), MPI_BYTE, globalRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			for (const auto& msg : incMsgs) {
				if (msg.dist < distances[msg.nodeId]) {
					distances[msg.nodeId] = msg.dist;
				}
			}
		}

		m_numIterations++;
	}
	m_totalRuntime = std::chrono::high_resolution_clock::now() - start;
	m_distances = distances;
}


void DistributedDijkstra::reportMetrics() const {
    int rank, rankSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &rankSize);

    int globalIters = 0, globalMsgs = 0, globalBytes = 0;
    MPI_Reduce(&m_numIterations, &globalIters, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&m_numMessages, &globalMsgs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&m_bytesSent, &globalBytes, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    std::vector<float> globalDistances(m_distances.size());
    MPI_Reduce(m_distances.data(), globalDistances.data(), m_distances.size(), MPI_FLOAT, MPI_MIN, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\n================ DIJKSTRA METRICS ================\n");
        printf("Total Runtime (s): %f\n", m_totalRuntime.count());
        printf("Total Iterations: %d\n", globalIters);
        printf("Total P2P Messages Sent: %d\n", globalMsgs);
        printf("Total P2P Bytes Sent: %d\n", globalBytes);
        printf("\n--- Final Distances ---\n");
        for (size_t nodeId = 0; nodeId < globalDistances.size(); nodeId++) {
            if (globalDistances[nodeId] == std::numeric_limits<float>::infinity()) {
                printf("Node %zu | Distance: INFINITY\n", nodeId);
            } else {
                printf("Node %zu | Distance: %f\n", nodeId, globalDistances[nodeId]);
            }
        }
        printf("==================================================\n");
    }
}

