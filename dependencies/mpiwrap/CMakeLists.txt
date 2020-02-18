project(mpiwrap C CXX)
cmake_minimum_required(VERSION 3.1)

# option(ENABLE_TESTING "Enable testing" ON)
# if (ENABLE_TESTING)
#     add_subdirectory(gtest-1.7.0)
#     enable_testing()
# endif()

add_library(mpiwrap INTERFACE)
target_include_directories(mpiwrap INTERFACE .)

# add_test(NAME mpiwrap_test COMMAND mpiwrap_test)
# add_executable(mpiwrap_test mpiwrap_test.cpp)
# target_include_directories(mpiwrap_test PRIVATE gtest-1.7.0/include)
# target_link_libraries(mpiwrap_test gtest_main mpiwrap)
