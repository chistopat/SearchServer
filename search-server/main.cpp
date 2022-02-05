#include "tests_search_server.h"
#include "tests_paginator.h"
#include "tests_request_queue.h"

int main() {
    TestPaginator();
    TestSearchServer();
    TestRequestQueue();
}