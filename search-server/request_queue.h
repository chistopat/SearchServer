#pragma once

#include "search_server.h"

#include <deque>

class RequestQueue {
  public:
    explicit RequestQueue(const SearchServer& search_server, int time_window = 1440, int default_metric_value = 0);

  public:
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

  public:
    void CollectMetrics(const std::vector<Document>& result);

  private:
    const SearchServer& search_server_;
    std::deque<int> timeline_;
    const int time_window_;
    int empty_results_metric_ = 0;
};

template<typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    CollectMetrics(result);
    return result;
}
