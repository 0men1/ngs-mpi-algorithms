#include <iostream>
#include <memory>
#include <vector>
#include "DistributedAlgorithm.h"
#include "DistributedDijkstra.h"
#include "DistributedLeaderElection.h"
#include "GraphData.h"
#include "mpi_utils.h"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Required Options:" << std::endl;
    std::cout << "  -g, --graph FILE      Path to graph JSON file" << std::endl;
    std::cout << "  -p, --part FILE     Path to partition JSON file" << std::endl;
    std::cout << "  -a, --algo NAME     Algorithm to run (dijkstra or leader)" << std::endl;
    std::cout << std::endl;
    std::cout << "Algorithm-Specific Options:" << std::endl;
    std::cout << "  dijkstra:" << std::endl;
    std::cout << "    -s, --source NODE  Source node for shortest path" << std::endl;
    std::cout << "  leader:" << std::endl;
    std::cout << "    -r, --rounds NUM   Number of election rounds" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << programName << " --graph graph.json --part part.json --algo dijkstra --source 0" << std::endl;
    std::cout << "  " << programName << " --graph graph.json --part part.json --algo leader --rounds 30" << std::endl;
}

int main(int argc, char** argv) {
	int world_rank;
	MPI_CHECK(MPI_Init(&argc, &argv));
	MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &world_rank));

	std::string graphFile;
	std::string partFile;
	std::string algorithm;
	std::string source;
	int rounds = 0;

	std::vector<std::string> args(argv, argv+argc);
	bool has_graph = false, has_part = false, has_algo = false, has_source = false, has_rounds = false;
	
	for (int i = 1; i < args.size(); i++) {
		if (args[i] == "--graph" || args[i] == "-g") {
			graphFile = args[++i];
			has_graph = true;
		} else if (args[i] == "--part" || args[i] == "-p") {
			partFile = args[++i];
			has_part = true;
		} else if (args[i] == "--rounds" || args[i] == "-r") {
			rounds = std::stoi(args[++i]);
			has_rounds = true;
		} else if (args[i] == "--algo" || args[i] == "--algorithm" || args[i] == "-a") {
			algorithm = args[++i];
			has_algo = true;
		} else if (args[i] == "--source" || args[i] == "-s") {
			source = args[++i];
			has_source = true;
		} else if (args[i] == "-h" || args[i] == "--help") {
			printUsage(argv[0]);
			MPI_CHECK(MPI_Finalize());
			return 0;
		} else {
			std::cerr << "Unknown argument: " << args[i] << std::endl;
			printUsage(argv[0]);
			MPI_CHECK(MPI_Finalize());
			return 1;
		}
	}

	if (!has_graph || !has_part || !has_algo) {
		if (world_rank == 0) {
			std::cerr << "Error: Missing required arguments." << std::endl;
			printUsage(argv[0]);
		}
		MPI_CHECK(MPI_Finalize());
		return 1;
	}

	if (algorithm == "dijkstra" && !has_source) {
		if (world_rank == 0) {
			std::cerr << "Error: Dijkstra requires --source" << std::endl;
			printUsage(argv[0]);
		}
		MPI_CHECK(MPI_Finalize());
		return 1;
	}

	if (algorithm == "leader" && !has_rounds) {
		if (world_rank == 0) {
			std::cerr << "Error: Leader Election requires --rounds" << std::endl;
			printUsage(argv[0]);
		}
		MPI_CHECK(MPI_Finalize());
		return 1;
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

	MPI_CHECK(MPI_Finalize());
	return 0;
}
