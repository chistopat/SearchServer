#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>


using namespace std;

namespace praktikum {

static constexpr auto kEpsilon = 1e-6;

namespace helpers {

string ReadLine(std::istream& in = std::cin) {
    string s;
    getline(in, s);
    return s;
}

int ReadLineWithNumber(std::istream& in = std::cin) {
    int result;
    in >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;

    for (const char kSymbol : text) {
        if (kSymbol == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += kSymbol;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}
} // namespace helpers

class Document {
  public:
    Document() = default;

    Document(int id, double relevance, int rating);

  public:
    int GetId() const;

    double GetRelevance() const;

    int GetRating() const;

    friend ostream& operator<<(ostream& os, const Document& document);

  private:
    int id_ = 0;
    double relevance_ = 0.;
    int rating_ = 0;
};

Document::Document(int id, double relevance, int rating)
    : id_(id), relevance_(relevance), rating_(rating) {}

int Document::GetId() const {
    return id_;
}

int Document::GetRating() const {
    return rating_;
}

double Document::GetRelevance() const {
    return relevance_;
}

ostream& operator<<(ostream& os, const Document& document) {
    os << "{ "s
         << "document_id = "s << document.GetId() << ", "s
         << "relevance = "s << document.GetRelevance() << ", "s
         << "rating = "s << document.GetRating()
         << " }"s;
    return os;
}

bool operator<(const Document& left, const Document& right) {
    if (abs(left.GetRelevance() - right.GetRelevance()) < kEpsilon) {
        return left.GetRating() > left.GetRating();
    }
    return left.GetRelevance() > left.GetRelevance();
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
  public:
    using Documents = vector<Document>;

  public:
    static constexpr auto kMaxResultDocumentCount = 5U;
    static constexpr auto kMinusWordPrefix = '-';

  public:
    void SetStopWords(const string& text);

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings);

    template<typename Criteria>
    Documents FindTopDocuments(const string& raw_query, Criteria criteria) const {
        const Query kQuery = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(kQuery, criteria);
        sort(matched_documents.begin(), matched_documents.end());
        CutDocuments(matched_documents);

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(const string& raw_query) const;

    size_t GetDocumentCount() const;

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;

  private:
    class DocumentData {
      public:
        DocumentData(int rating, DocumentStatus status);

      public:
        int GetRating() const;

        DocumentStatus GetStatus() const;

      private:
        int rating_;
        DocumentStatus status_;
    };

    class QueryWord {
      public:
        QueryWord(const string& data, bool is_minus, bool is_stop);

      public:
        const string& GetData() const;

        bool IsMinus() const;

        bool IsStop() const;

      private:
        string data_;
        bool is_minus_;
        bool is_stop_;
    };

    QueryWord ParseQueryWord(string text) const;

    class Query {
      public:
        const set<string>& GetPlusWords() const;

        set<string>& GetPlusWords();

        const set<string>& GetMinusWords() const;

        set<string>& GetMinusWords();

      private:
        set<string> plus_words_;
        set<string> minus_words_;
    };

  private:
    bool IsStopWord(const string& word) const;

    static void CutDocuments(Documents& documents);

    vector<string> SplitIntoWordsNoStop(const string& text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    void MatchByPlusWords(const Query& query, int document_id, vector<string>& matched) const;

    void MatchByMinusWords(const Query& query, int document_id, vector<string>& matched) const;

    Query ParseQuery(const string& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const;

    template<typename Criteria>
    vector<Document> FindAllDocuments(const Query& query, Criteria criteria) const {
        map<int, double> document_to_relevance;

        RetrieveByPlusWords(document_to_relevance, query, criteria);
        FilterByMinusWords(document_to_relevance, query, criteria);

        return MakeDocuments(document_to_relevance);
    }

    template<typename Criteria, typename K = int, typename V = double>
    void RetrieveByPlusWords(map<K, V>& relevance_map, const Query& query, Criteria criteria) const {
        for (const string& word : query.GetPlusWords()) {
            if (inverted_index_.count(word) == 0U) {
                continue;
            }
            const double kInverseDocumentFreq = ComputeWordInverseDocumentFreq(word);
            for (const auto[kDocumentId, kTermFreq] : inverted_index_.at(word)) {
                const auto& kDocumentData = storage_.at(kDocumentId);
                if (criteria(kDocumentId, kDocumentData.GetStatus(), kDocumentData.GetRating())) {
                    relevance_map[kDocumentId] += kTermFreq * kInverseDocumentFreq;
                }
            }
        }
    }

    template<typename Criteria, typename K = int, typename V = double>
    void FilterByMinusWords(map<K, V>& relevance_map, const Query& query, Criteria criteria) const {
        for (const string& word : query.GetMinusWords()) {
            if (inverted_index_.count(word) == 1U) {
                for (const auto[kDocumentId, _] : inverted_index_.at(word)) {
                    relevance_map.erase(kDocumentId);
                }
            }
        }
    }

    vector<Document> MakeDocuments(const map<int, double>& document_to_relevance) const;

  private:
    set<string> stop_words_;
    map<string, map<int, double>> inverted_index_;
    map<int, DocumentData> storage_;
};

const set<string>& SearchServer::Query::GetPlusWords() const {
    return plus_words_;
}

set<string>& SearchServer::Query::GetPlusWords() {
    return plus_words_;
}

const set<string>& SearchServer::Query::GetMinusWords() const {
    return minus_words_;
}

set<string>& SearchServer::Query::GetMinusWords() {
    return minus_words_;
}

SearchServer::QueryWord::QueryWord(const string& data, bool is_minus, bool is_stop)
    : data_(data), is_minus_(is_minus), is_stop_(is_stop) {

}

const string& SearchServer::QueryWord::GetData() const {
    return data_;
}

bool SearchServer::QueryWord::IsMinus() const {
    return is_minus_;
}

bool SearchServer::QueryWord::IsStop() const {
    return is_stop_;
}

SearchServer::DocumentData::DocumentData(int rating, DocumentStatus status)
    : rating_(rating), status_(status) {

}

int SearchServer::DocumentData::GetRating() const {
    return rating_;

}

DocumentStatus SearchServer::DocumentData::GetStatus() const {
    return status_;
}

void SearchServer::SetStopWords(const string& text) {
    for (const string& word : helpers::SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status,
                               const vector<int>& ratings) {
    const vector<string> kWords = SplitIntoWordsNoStop(document);
    const double kInvWordCount = 1.0 / static_cast<double>(kWords.size());
    for (const string& word : kWords) {
        inverted_index_[word][document_id] += kInvWordCount;
    }

    storage_.insert({document_id, DocumentData{ComputeAverageRating(ratings), status}});
}

void SearchServer::CutDocuments(Documents& documents) {
    if (documents.size() > kMaxResultDocumentCount) {
        documents.resize(kMaxResultDocumentCount);
    }
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    const auto ByStatus = [&status](int document_id, DocumentStatus document_status, int rating) {
      return document_status == status;
    };
    return FindTopDocuments(raw_query, ByStatus);
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    const Query kQuery = ParseQuery(raw_query);
    vector<string> matched_words;

    MatchByPlusWords(kQuery, document_id, matched_words);
    MatchByMinusWords(kQuery, document_id, matched_words);

    return {matched_words, storage_.at(document_id).GetStatus()};

}

size_t SearchServer::GetDocumentCount() const {
    return storage_.size();
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0U;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : helpers::SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int kRating : ratings) {
        rating_sum += kRating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0U] == kMinusWordPrefix) {
        is_minus = true;
        text = text.substr(1U);
    }
    return QueryWord{text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query query;
    for (const string& word : helpers::SplitIntoWords(text)) {
        const QueryWord kQueryWord = ParseQueryWord(word);
        if (!kQueryWord.IsStop()) {
            if (kQueryWord.IsMinus()) {
                query.GetMinusWords().insert(kQueryWord.GetData());
            } else {
                query.GetPlusWords().insert(kQueryWord.GetData());
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(static_cast<double>(GetDocumentCount()) / static_cast<double >(inverted_index_.at(word).size()));
}

vector<Document> SearchServer::MakeDocuments(const map<int, double>& document_to_relevance) const {
        vector<Document> documents;
        documents.reserve(document_to_relevance.size());

        for (const auto[kDocumentId, kRelevance] : document_to_relevance) {
            documents.emplace_back(
                kDocumentId,
                kRelevance,
                storage_.at(kDocumentId).GetRating()
            );
        }

    return documents;
}

void SearchServer::MatchByPlusWords(const SearchServer::Query& query, int document_id, vector<string>& matched) const {
    for (const string& word : query.GetPlusWords()) {
        if (inverted_index_.count(word) == 1U) {
            if (inverted_index_.at(word).count(document_id) == 1U) {
                matched.push_back(word);
            }
        }
    }
}

void SearchServer::MatchByMinusWords(const SearchServer::Query& query, int document_id, vector<string>& matched) const {
    for (const string& word : query.GetMinusWords()) {
        if (inverted_index_.count(word) == 1U) {
            if (inverted_index_.at(word).count(document_id) == 1U) {
                matched.clear();
                break;
            }
        }
    }
}


[[maybe_unused]] void PrintDocument(const Document& document) {
    cout << document << endl;
}

[[maybe_unused]] SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(helpers::ReadLine());

    const int kDocumentCount = helpers::ReadLineWithNumber();
    for (int document_id = 0; document_id < kDocumentCount; ++document_id) {
        const string kDocument = helpers::ReadLine();

        int status_raw;
        cin >> status_raw;

        size_t ratings_size;
        cin >> ratings_size;

        vector<int> ratings(ratings_size, 0);

        for (auto& rating : ratings) {
            cin >> rating;
        }

        search_server.AddDocument(document_id,kDocument,static_cast<DocumentStatus>(status_raw), ratings);
        helpers::ReadLine();
    }

    return search_server;
}

[[maybe_unused]] void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}
} // namespace praktikum

int main() {
    using namespace praktikum;
    const auto server = CreateSearchServer();

    return 0;
}