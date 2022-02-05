#pragma once

#include "document.h"
#include "string_processing.h"

#include <vector>
#include <string>
#include <set>
#include <utility>
#include <map>
#include <cmath>
#include <algorithm>


class SearchServer {
  public:
    using Documents = std::vector<Document>;

  public:
    const size_t kMaxResultDocumentSize = 5U;
    const char kMinusWordPrefix = '-';

  public:
    SearchServer() = default;

    template<typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(stop_words.begin(), stop_words.end()) {
        CheckWords(stop_words_);
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {}

  public:
    void SetStopWords(const std::string& text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status,
                     const std::vector<int>& ratings);

    template<typename Predicate>
    Documents FindTopDocuments(const std::string& raw_query, Predicate predicate) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    size_t GetDocumentCount() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,
                                                                       int document_id) const;

    int GetDocumentId(int index) const;

  private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const;

    class Query {
      public:
        const std::set<std::string>& GetPlusWords() const;

        std::set<std::string>& GetPlusWords();

        const std::set<std::string>& GetMinusWords() const;

        std::set<std::string>& GetMinusWords();

      private:
        std::set<std::string> plus_words_;
        std::set<std::string> minus_words_;
    };

  private:
    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    Query ParseQuery(const std::string& text) const;

    double ComputeWordInverseDocumentFrequency(const std::string& word) const;

    template<typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const;

    std::vector<Document> MakeDocuments(const std::map<int, double>& document_to_relevance) const;

    static bool IsValidWord(const std::string& word);

    template<typename Container>
    static void CheckWords(Container words) {
        for (const auto& word : words) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("invalid word: " + word);
            }
        }
    }

    void CheckDocumentId(int document_id) const;

  private:
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_frequency_;
    std::map<int, DocumentData> storage_;
    std::vector<int> documents_;
};

template<typename Predicate>
SearchServer::Documents SearchServer::FindTopDocuments(const std::string& raw_query, Predicate predicate) const {
    const Query kQuery = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(kQuery, predicate);
    sort(matched_documents.begin(), matched_documents.end());

    if (matched_documents.size() > kMaxResultDocumentSize) {
        matched_documents.resize(kMaxResultDocumentSize);
    }

    return matched_documents;
}

template<typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query, Predicate predicate) const {
    std::map<int, double> document_to_relevance;

    for (const std::string& word : query.GetPlusWords()) {
        if (word_to_document_frequency_.count(word) == 0U) {
            continue;
        }
        const double kInverseDocumentFreq = ComputeWordInverseDocumentFrequency(word);
        for (const auto[kDocumentId, kTermFreq] : word_to_document_frequency_.at(word)) {
            const auto& kDocumentData = storage_.at(kDocumentId);
            if (predicate(kDocumentId, kDocumentData.status, kDocumentData.rating)) {
                document_to_relevance[kDocumentId] += kTermFreq * kInverseDocumentFreq;
            }
        }
    }

    for (const std::string& word : query.GetMinusWords()) {
        if (word_to_document_frequency_.count(word) == 1U) {
            for (const auto[kDocumentId, _] : word_to_document_frequency_.at(word)) {
                document_to_relevance.erase(kDocumentId);
            }
        }
    }

    return MakeDocuments(document_to_relevance);
}
