clean:
	rm -rf build

build:
	cmake -B build -S mpi_runtime
	cmake --build build

clean_build: clean build

run_test:
	cmake -B build -S mpi_runtime && cmake --build build
	mpirun -n 2 ./build/run_test_dijkstra
	mpirun -n 2 ./build/run_test_leaderelection
