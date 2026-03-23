cmake_minimum_required(VERSION 3.10)

project(ngs_mpi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(MPI REQUIRED)

add_executable(ngs_mpi)

target_sources(ngs_mpi PRIVATE
	src/main.cpp 
	src/GraphData.cpp
)

target_link_libraries(ngs_mpi PRIVATE MPI::MPI_CXX)
