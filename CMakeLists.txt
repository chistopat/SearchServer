cmake_minimum_required(VERSION 3.10)
project(SearchServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS "-fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector")

add_executable(sprint1 search-server/main.cpp)
