#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string>
#include <unordered_map>
#include "GraphData.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

/*
	const int m_rankId;
	std::vector<int> m_nodeOwnership;
	std::unordered_map<int, std::vector<Edge>> m_adjList;
*/

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
		int ownerRank = part_json[node_key];
		m_nodeOwnership[i] = part_json[node_key];

		if (ownerRank == m_rankId) {
			for (const auto& edge : graph_json["adjacency_list"][node_key]) {
				int v = edge["v"];
				float w = edge["w"];
				m_adjList[i].push_back(Edge{v, w});
			}
		}
	}
}
