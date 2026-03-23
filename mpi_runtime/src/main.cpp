#include <mpi.h>
#include <vector>
#include <iostream>

#include "GraphData.h"

int main(int argc, char* argv[]) {

	std::string graphFile = "outputs/graph.json";
	std::string partFile = "outputs/part.json";
	std::string algorithm = "dijkstra";

	int rounds = 200; // conditional if leader
	std::string source = "0"; // ocnditioanl if dijkstra

	std::vector<std::string> args(argv, argv+argc);
	for (int i = 1; i < args.size(); i++) {
		if (args[i] == "--graph") {
			graphFile = args[++i];
		} else if (args[i] == "--part") {
			partFile = args[++i];
		} else if (args[i] == "--rounds") {
			rounds = std::stoi(args[++i]);
		} else if (args[i] == "--algorithm") {
			algorithm = args[++i];
		} else if (args[i] == "--source") {
			source = args[++i];
		}
	}

	GraphData gd;
	gd.loadFiles(graphFile, partFile);

	std::cout << "Graph: " << graphFile << std::endl;
	std::cout << "Partition: " << partFile << std::endl;
	std::cout << "Algorithm: " << algorithm << std::endl;
	std::cout << "Rounds: " << rounds << std::endl;
	std::cout << "Source: " << source << std::endl;

	std::cout << "Finished running MPI runtime" << std::endl;

	return 0;
}
