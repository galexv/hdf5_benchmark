project(H5Benchmark C CXX)
cmake_minimum_required(VERSION 3.1)

set(HDF5_PREFER_PARALLEL true)
find_package(HDF5 REQUIRED C)
if (NOT HDF5_IS_PARALLEL)
  message(FATAL_ERROR "This package requires parallel HDF5")
endif()

add_subdirectory(dependencies/cmdline)
add_subdirectory(dependencies/mpiwrap)

add_library(mydeps INTERFACE)
target_link_libraries(mydeps INTERFACE ${HDF5_LIBRARIES} cmdline mpiwrap)
target_include_directories(mydeps INTERFACE ${HDF5_INCLUDE_DIRS})

macro(add_my_exec tgt)
    add_executable(${tgt} "${tgt}.cpp")
    target_link_libraries(${tgt} mydeps)
    install(TARGETS ${tgt} RUNTIME DESTINATION "./bin")
endmacro()

foreach(tgt single_proc several_proc several_proc_rows several_proc_blocks)
    add_my_exec(${tgt})
endforeach()
