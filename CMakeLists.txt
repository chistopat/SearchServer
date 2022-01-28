cmake_minimum_required(VERSION 3.10)
project(SearchServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Werror")
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_executable(sprint1 search-server/main.cpp)
set_target_properties(sprint1 PROPERTIES
        COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
        INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR}
        )
target_link_libraries(sprint1 ${Boost_LIBRARIES})