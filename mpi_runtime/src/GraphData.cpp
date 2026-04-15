#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string>
#include <unordered_map>
#include "GraphData.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

void GraphData::loadData(std::string &graphFile, std::string &partFile) {
	std::ifstream graph_file(graphFile);
	std::ifstream part_file(partFile);
	if (!graph_file.is_open() || !part_file.is_open()) {
		std::cout << "Failed to open files. Aborting." << std::endl;
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	json graph_json, part_json;
	graph_file >> graph_json;
	part_file >> part_json;

	int numNodes = graph_json["metadata"]["num_nodes"];
	m_nodeOwnership.resize(numNodes);

	for (int i = 0; i < numNodes; i++) {
		std::string node_key = std::to_string(i);
		m_nodeOwnership[i] = part_json[node_key];
		if (m_nodeOwnership[i] == m_rankId) {
			m_ownedNodes.insert(i);
		}
	}

	for (int src = 0; src < numNodes; src++) {
		std::string src_key = std::to_string(src);
		for (const auto& edge : graph_json["adjacency_list"][src_key]) {
			int dst = edge["v"];
			float w = edge["w"];

			if (m_nodeOwnership[src] == m_rankId) {
				m_adjList[src].push_back(Edge{dst, w});
			}

			if (m_nodeOwnership[dst] == m_rankId) {
				m_incomingEdges[dst].push_back(Edge{src, w});
			}
		}
	}
}
