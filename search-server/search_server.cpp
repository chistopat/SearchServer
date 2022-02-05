//
// Created by Yegor Chistyakov on 05.02.2022.
//

#include "search_server.h"


const std::set<std::string>& SearchServer::Query::GetPlusWords() const {
    return plus_words_;
}

std::set<std::string>& SearchServer::Query::GetPlusWords() {
    return plus_words_;
}

const std::set<std::string>& SearchServer::Query::GetMinusWords() const {
    return minus_words_;
}

std::set<std::string>& SearchServer::Query::GetMinusWords() {
    return minus_words_;
}

void SearchServer::SetStopWords(const std::string& text) {
    for (const std::string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
                               const std::vector<int>& ratings) {
    CheckDocumentId(document_id);
    documents_.push_back(document_id);
    const std::vector<std::string> kWords = SplitIntoWordsNoStop(document);
    const double kInvertedWordCount = 1.0 / static_cast<double>(kWords.size());
    for (const std::string& word : kWords) {
        word_to_document_frequency_[word][document_id] += kInvertedWordCount;
    }

    storage_.insert({document_id, DocumentData{ComputeAverageRating(ratings), status}});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [&status](int, DocumentStatus document_status, int) {
      return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                                                 int document_id) const {
    const Query kQuery = ParseQuery(raw_query);
    std::vector<std::string> matched_words;

    for (const std::string& word : kQuery.GetPlusWords()) {
        if (word_to_document_frequency_.count(word) == 1U) {
            if (word_to_document_frequency_.at(word).count(document_id) == 1U) {
                matched_words.push_back(word);
            }
        }
    }

    for (const std::string& word : kQuery.GetMinusWords()) {
        if (word_to_document_frequency_.count(word) == 1U) {
            if (word_to_document_frequency_.at(word).count(document_id) == 1U) {
                matched_words.clear();
                break;
            }
        }
    }

    return {matched_words, storage_.at(document_id).status};

}

size_t SearchServer::GetDocumentCount() const {
    return storage_.size();
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0U;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
        if (!IsValidWord(word)) {
            throw std::invalid_argument("invalid word: " + word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int kRating : ratings) {
        rating_sum += kRating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    QueryWord result;

    bool is_minus = false;
    if (text.front() == kMinusWordPrefix) {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text.front() == kMinusWordPrefix || !IsValidWord(text)) {
        throw std::invalid_argument("invalid word " + text);
    }

    result = QueryWord{text, is_minus, IsStopWord(text)};
    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord kQueryWord = ParseQueryWord(word);
        if (!kQueryWord.is_stop) {
            if (kQueryWord.is_minus) {
                query.GetMinusWords().insert(kQueryWord.data);
            } else {
                query.GetPlusWords().insert(kQueryWord.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFrequency(const std::string& word) const {
    return log(
        static_cast<double>(GetDocumentCount()) / static_cast<double>(word_to_document_frequency_.at(word).size()));
}

std::vector<Document> SearchServer::MakeDocuments(const std::map<int, double>& document_to_relevance) const {
    std::vector<Document> documents;
    documents.reserve(document_to_relevance.size());

    for (const auto[kDocumentId, kRelevance] : document_to_relevance) {
        documents.emplace_back(Document{kDocumentId, kRelevance, storage_.at(kDocumentId).rating});
    }

    return documents;
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char ch) { return iscntrl(ch); });
}

void SearchServer::CheckDocumentId(int document_id) const {
    if (document_id < 0) {
        throw std::invalid_argument("document_id must not be negative");
    }
    if (storage_.count(document_id)) {
        throw std::invalid_argument("document_id already exists");
    }
}

int SearchServer::GetDocumentId(int index) const {
    return documents_.at(index);
}
