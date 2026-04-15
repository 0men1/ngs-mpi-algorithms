cmake_minimum_required(VERSION 3.14)

project(ngs_mpi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(MPI REQUIRED)
find_package(GTest REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(ngs_mpi)

target_sources(ngs_mpi PRIVATE
	src/main.cpp 
	src/GraphData.cpp
	src/DistributedDijkstra.cpp
	src/DistributedLeaderElection.cpp
)

target_include_directories(ngs_mpi PRIVATE include src)
target_link_libraries(ngs_mpi PRIVATE MPI::MPI_CXX nlohmann_json::nlohmann_json)

enable_testing()

# Dijkstra and GraphData tests
add_executable(run_test_dijkstra)
target_sources(run_test_dijkstra PRIVATE
	tests/test_dijkstra.cpp
	src/GraphData.cpp
	src/DistributedDijkstra.cpp
	src/DistributedLeaderElection.cpp
)
target_include_directories(run_test_dijkstra PRIVATE include src)
target_link_libraries(run_test_dijkstra PRIVATE GTest::GTest GTest::Main MPI::MPI_CXX nlohmann_json::nlohmann_json)
add_test(NAME DijkstraTests COMMAND run_test_dijkstra)

# Leader Election tests
add_executable(run_test_leaderelection)
target_sources(run_test_leaderelection PRIVATE
	tests/test_leaderelection.cpp
	src/GraphData.cpp
	src/DistributedDijkstra.cpp
	src/DistributedLeaderElection.cpp
)
target_include_directories(run_test_leaderelection PRIVATE include src)
target_link_libraries(run_test_leaderelection PRIVATE GTest::GTest GTest::Main MPI::MPI_CXX nlohmann_json::nlohmann_json)
add_test(NAME LeaderElectionTests COMMAND run_test_leaderelection)
