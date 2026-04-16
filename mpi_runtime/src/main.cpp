#include <iostream>
#include <memory>
#include <vector>
#include <mpi.h>
#include "DistributedAlgorithm.h"
#include "DistributedDijkstra.h"
#include "DistributedLeaderElection.h"
#include "GraphData.h"

int main(int argc, char** argv) {
	int world_rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	std::string graphFile = "outputs/graph.json";
	std::string partFile = "outputs/part.json";
	std::string algorithm = "dijkstra";

	int rounds = 200; // conditional if leader
	std::string source = "0"; // ocnditioanl if dijkstra

	std::vector<std::string> args(argv, argv+argc);
	for (int i = 1; i < args.size(); i++) {
		if (args[i] == "--graph" || args[i] == "-g") {
			graphFile = args[++i];
		} else if (args[i] == "--part" || args[i] == "-p") {
			partFile = args[++i];
		} else if (args[i] == "--rounds" || args[i] == "-r") {
			rounds = std::stoi(args[++i]);
		} else if (args[i] == "--algo" || args[i] == "--algorithm" || args[i] == "-a") {
			algorithm = args[++i];
		} else if (args[i] == "--source" || args[i] == "-s") {
			source = args[++i];
		} else {
			std::cerr << "Unknown argument: " << args[i] << std::endl;
		}
	}

	if (world_rank == 0) {
		std::cout << "Algorithm: " << algorithm << ", Rounds: " << rounds << ", Source: " << source << std::endl;
	}

	GraphData graph(world_rank, graphFile, partFile);
	std::unique_ptr<DistributedAlgorithm> algo;

	if (algorithm == "dijkstra") {
		algo = std::make_unique<DistributedDijkstra>(std::stoi(source));
	} else if (algorithm == "leader") {
		algo = std::make_unique<DistributedLeaderElection>(rounds);
	} else {
		std::cout << "Requested algorithm is unknown. Aborting." << std::endl;
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	algo->execute(graph);
	algo->reportMetrics();

	MPI_Finalize();
	return 0;
}
