project(H5Benchmark C CXX)
cmake_minimum_required(VERSION 3.1)

find_package(HDF5 REQUIRED CXX)
find_package(ALPSCore REQUIRED params)

add_library(mydeps INTERFACE)
target_link_libraries(mydeps INTERFACE ${HDF5_LIBRARIES} ${ALPSCore_LIBRARIES})
target_include_directories(mydeps INTERFACE ${HDF5_INCLUDE_DIRS})

add_executable(single_proc single_proc.cpp)
target_link_libraries(single_proc mydeps)

add_executable(several_proc several_proc.cpp)
target_link_libraries(several_proc mydeps)
