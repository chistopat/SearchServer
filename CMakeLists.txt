cmake_minimum_required(VERSION 3.10)
project(SearchServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Werror")

add_executable(
        search-server

        search-server/main.cpp
        search-server/search_server.cpp
        search-server/document.cpp
        search-server/read_input_functions.cpp
        search-server/string_processing.cpp
        search-server/request_queue.cpp

        search-server/paginator.h
)
