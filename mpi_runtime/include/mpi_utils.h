#ifndef MPI_UTILS_H
#define MPI_UTILS_H

#include <iostream>
#include <mpi.h>

inline int mpi_check(int errcode, const char* operation) {
    if (errcode != MPI_SUCCESS) {
        char error_string[MPI_MAX_ERROR_STRING];
        int len;
        MPI_Error_string(errcode, error_string, &len);
        std::cerr << "MPI Error during " << operation << ": " << error_string << std::endl;
        return 1;
    }
    return 0;
}

#define MPI_CHECK(call) if (mpi_check((call), #call)) { MPI_Abort(MPI_COMM_WORLD, 1); }

#endif
