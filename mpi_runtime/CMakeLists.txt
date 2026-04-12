cmake_minimum_required(VERSION 3.14)

project(ngs_mpi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(MPI REQUIRED)
find_package(GTest REQUIRED)

add_executable(ngs_mpi)

target_sources(ngs_mpi PRIVATE
	src/main.cpp 
	src/GraphData.cpp
	src/DistributedDijkstra.cpp
	src/DistributedLeaderElection.cpp
)

target_include_directories(ngs_mpi PRIVATE include)
target_link_libraries(ngs_mpi PRIVATE MPI::MPI_CXX)

enable_testing()

add_executable(run_test)

target_sources(run_test PRIVATE
	tests/test_main.cpp
)

target_include_directories(run_test PRIVATE include)
target_link_libraries(run_test PRIVATE GTest::GTest GTest::Main MPI::MPI_CXX)

add_test(NAME run_test COMMAND run_test)
