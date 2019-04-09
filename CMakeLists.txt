project(H5Benchmark C CXX)
cmake_minimum_required(VERSION 3.1)

set(HDF5_PREFER_PARALLEL true)
find_package(HDF5 REQUIRED C)
if (NOT HDF5_IS_PARALLEL)
  message(FATAL_ERROR "This package requires parallel HDF5")
endif()

find_package(ALPSCore REQUIRED params)

add_library(mydeps INTERFACE)
target_link_libraries(mydeps INTERFACE ${HDF5_LIBRARIES} ${ALPSCore_LIBRARIES})
target_include_directories(mydeps INTERFACE ${HDF5_INCLUDE_DIRS})

add_executable(single_proc single_proc.cpp)
target_link_libraries(single_proc mydeps)

add_executable(several_proc several_proc.cpp)
target_link_libraries(several_proc mydeps)

add_executable(several_proc_rows several_proc_rows.cpp)
target_link_libraries(several_proc_rows mydeps)
