run_test:
	cmake -B build && cmake --build build
	mpirun -n 2 ./build/run_test_dijkstra
	mpirun -n 2 ./build/run_test_leaderelection
