#include "request_queue.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    auto result =  search_server_.FindTopDocuments(raw_query, status);
    CollectMetrics(result);
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    auto result = search_server_.FindTopDocuments(raw_query);
    CollectMetrics(result);
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_results_metric_;
}

RequestQueue::RequestQueue(const SearchServer& search_server, int time_window, int default_metric_value)
    : search_server_(search_server)
      , time_window_(time_window)
      , empty_results_metric_(default_metric_value) {

}

void RequestQueue::CollectMetrics(const std::vector<Document>& result) {
    // enqueue
    if (result.empty()) {
        ++empty_results_metric_;
    }
    timeline_.push_back(static_cast<int>(result.size()));

    // dequeue
    while(static_cast<int>(timeline_.size()) > time_window_) {
        if (timeline_.front() == 0) {
            --empty_results_metric_;
        }
        timeline_.pop_front();
    }
}
