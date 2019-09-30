project(H5Benchmark C CXX)
cmake_minimum_required(VERSION 3.1)

find_package(MPI REQUIRED CXX)

set(HDF5_PREFER_PARALLEL true)
find_package(HDF5 REQUIRED C)
if (NOT HDF5_IS_PARALLEL)
  message(FATAL_ERROR "This package requires parallel HDF5")
endif()

# add_subdirectory("dependencies/ALPSCore_reduced")
include(ExternalProject)
ExternalProject_Add(ALPSCore
    SOURCE_DIR "${PROJECT_SOURCE_DIR}/dependencies/ALPSCore_reduced"
    CMAKE_ARGS "-DTesting=OFF" "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>")
ExternalProject_Get_property(ALPSCore INSTALL_DIR)
list(APPEND CMAKE_PREFIX_PATH "${INSTALL_DIR}")
find_package(ALPSCore REQUIRED params)

add_library(mydeps INTERFACE)
add_dependencies(mydeps ALPSCore)
target_link_libraries(mydeps INTERFACE ${HDF5_LIBRARIES} MPI::MPI_CXX ${ALPSCore_LIBRARIES})
target_include_directories(mydeps INTERFACE ${HDF5_INCLUDE_DIRS})

macro(add_my_exec tgt)
    add_executable(${tgt} "${tgt}.cpp")
    target_link_libraries(${tgt} mydeps)
endmacro()

foreach(tgt single_proc several_proc several_proc_rows several_proc_blocks)
    add_my_exec(${tgt})
endforeach()
