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

static constexpr auto kEpsilon = 1e-6;

class Document {
  public:
    Document() = default;
    Document(int id, double relevance, int rating);

  public:
    int GetId() const;
    double GetRelevance() const;
    int GetRating() const;

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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        const auto ByStatus = [&status](int document_id, DocumentStatus document_status, int rating) {
          return document_status == status;
        };
        return FindTopDocuments(raw_query, ByStatus);
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    auto GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query kQuery = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : kQuery.GetPlusWords()) {
            if (word_to_document_freqs_.count(word) == 0U) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id) == 1U) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : kQuery.GetMinusWords()) {
            if (word_to_document_freqs_.count(word) == 0U) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id) == 1U) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).GetStatus()};
    }

  private:
    class DocumentData {
      public:
        DocumentData(int rating, DocumentStatus status)
            : rating_(rating)
              , status_(status) {}

      public:
        int GetRating() const {
            return rating_;
        }

        DocumentStatus GetStatus() const {
            return status_;
        }

      private:
        int rating_;
        DocumentStatus status_;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0U;
    }

  private:
    static void CutDocuments(Documents& documents);

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : helpers::SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int kRating : ratings) {
            rating_sum += kRating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    class QueryWord {
      public:
        QueryWord(const string& data, bool is_minus, bool is_stop)
            : data_(data), is_minus_(is_minus), is_stop_(is_stop) {}

      public:
        const string& GetData() const {
            return data_;
        }

        bool IsMinus() const {
            return is_minus_;
        }

        bool IsStop() const {
            return is_stop_;
        }

      private:
        string data_;
        bool is_minus_;
        bool is_stop_;
    };

    auto ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0U] == '-') {
            is_minus = true;
            text = text.substr(1U);
        }
        return QueryWord{text, is_minus, IsStopWord(text)};
    }

    class Query {
      public:
        const set<string>& GetPlusWords() const {
            return plus_words_;
        }

        const set<string>& GetMinusWords() const {
            return minus_words_;
        }

        set<string>& GetPlusWords() {
            return plus_words_;
        }

        set<string>& GetMinusWords() {
            return minus_words_;
        }

      private:
        set<string> plus_words_;
        set<string> minus_words_;
    };

    Query ParseQuery(const string& text) const {
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

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(static_cast<double>(GetDocumentCount())
                       / static_cast<double >(word_to_document_freqs_.at(word).size()));

    }

    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.GetPlusWords()) {
            if (word_to_document_freqs_.count(word) == 0U) {
                continue;
            }
            const double
                kInverseDocumentFreq = ComputeWordInverseDocumentFreq(word);
            for (const auto[kDocumentId, kTermFreq] : word_to_document_freqs_.at(
                word)) {
                const auto& document_data = documents_.at(kDocumentId);
                if (document_predicate(kDocumentId,
                                       document_data.GetStatus(),
                                       document_data.GetRating())) {
                    document_to_relevance[kDocumentId] +=
                        kTermFreq * kInverseDocumentFreq;
                }
            }
        }

        for (const string& word : query.GetMinusWords()) {
            if (word_to_document_freqs_.count(word) == 0U) {
                continue;
            }
            for (const auto[kDocumentId, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(kDocumentId);
            }
        }

        vector<Document> matched_documents;
        matched_documents.reserve(document_to_relevance.size());
        for (const auto[kDocumentId, kRelevance] : document_to_relevance) {
            matched_documents.emplace_back(
                kDocumentId,
                kRelevance,
                documents_.at(kDocumentId).GetRating()
            );
        }
        return matched_documents;
    }
};

void SearchServer::SetStopWords(const string& text) {
    for (const string& word : helpers::SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id,
                               const string& document,
                               DocumentStatus status,
                               const vector<int>& ratings) {
    const vector<string> kWords = SplitIntoWordsNoStop(document);
    const double kInvWordCount = 1.0 / static_cast<double>(kWords.size());
    for (const string& word : kWords) {
        word_to_document_freqs_[word][document_id] += kInvWordCount;
    }
    documents_.emplace(document_id,
                       DocumentData{
                           ComputeAverageRating(ratings),
                           status
                       });
}

void SearchServer::CutDocuments(Documents& documents) {
    if (documents.size() > kMaxResultDocumentCount) {
        documents.resize(kMaxResultDocumentCount);
    }
}

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.GetId() << ", "s
         << "relevance = "s << document.GetRelevance() << ", "s
         << "rating = "s << document.GetRating()
         << " }"s << endl;
}

SearchServer CreateSearchServer() {
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

        for (int& rating : ratings) {
            cin >> rating;
        }

        search_server.AddDocument(document_id,
                                  kDocument,
                                  static_cast<DocumentStatus>(status_raw),
                                  ratings);
        helpers::ReadLine();
    }

    return search_server;
}

void PrintMatchDocumentResult(int document_id,
                              const vector<string>& words,
                              DocumentStatus status) {
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
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0,
                              "белый кот и модный ошейник"s,
                              DocumentStatus::ACTUAL,
                              {8, -3});
    search_server.AddDocument(1,
                              "пушистый кот пушистый хвост"s,
                              DocumentStatus::ACTUAL,
                              {7, 2, 7});
    search_server.AddDocument(2,
                              "ухоженный пёс выразительные глаза"s,
                              DocumentStatus::ACTUAL,
                              {5, -12, 2, 1});
    search_server.AddDocument(3,
                              "ухоженный скворец евгений"s,
                              DocumentStatus::BANNED,
                              {9});
    search_server.AddDocument(4, "something"s, DocumentStatus::REMOVED, {9});
    search_server.AddDocument(5, "nothing"s, DocumentStatus::IRRELEVANT, {0});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments(
        "пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments(
        "пушистый ухоженный кот"s,
        DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments(
        "пушистый ухоженный кот"s,
        [](int document_id,
           DocumentStatus status,
           int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}