#include <iostream>
#include <fstream>
#include <mpi.h>
#include "GraphData.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

void GraphData::loadGraphData(std::string &graphFile) {
	std::ifstream graph_file(graphFile);
	if (!graph_file.is_open()) {
		std::cout << "Failed to open graph file. Aborting." << std::endl;
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	json graph_json;
	graph_file >> graph_json;

	int numNodes = graph_json["metadata"]["num_nodes"];

	std::cout << "number of nodes: " << numNodes << std::endl;

}

void GraphData::loadPartitionData(std::string &graphFile) {

}
