project(cmdline C CXX)
cmake_minimum_required(VERSION 3.1)


option(ENABLE_TESTING "Enable testing" ON)

add_library(cmdline INTERFACE)
target_include_directories(cmdline INTERFACE .)

if (ENABLE_TESTING)
    add_subdirectory(gtest-1.7.0)
    enable_testing()
    add_test(NAME cmdline_test COMMAND cmdline_test)
    add_executable(cmdline_test cmdline_test.cpp)
    target_include_directories(cmdline_test PRIVATE gtest-1.7.0/include)
    target_link_libraries(cmdline_test gtest_main cmdline)
endif()
