#pragma once

#include "test_framework.h"
#include "request_queue.h"


void TestRequestQueueGetNoResultRequests() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {});

    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }

    request_queue.AddFindRequest("curly dog"s);
    request_queue.AddFindRequest("big collar"s);
    request_queue.AddFindRequest("sparrow"s);

    ASSERT_EQUAL(request_queue.GetNoResultRequests(), 1437);
}

void TestRequestQueue() {
    RUN_TEST(TestRequestQueueGetNoResultRequests);
    std::cerr << std::endl;
}
