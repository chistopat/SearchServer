#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <experimental/iterator>
#include <cassert>

using namespace std;

namespace testing {
template<typename Key, typename Value>
std::ostream& operator<<(std::ostream& output_stream, const std::pair<Key, Value>& container) {
    return output_stream << container.first << ": "s << container.second;
}

template<typename Container>
std::ostream& PrintContainer(std::ostream& output_stream, const Container& container, std::string&& prefix,
                             std::string&& suffix, std::string&& delimiter = ", "s) {
    using namespace std::experimental;

    output_stream << prefix;
    std::copy(std::begin(container), std::end(container), make_ostream_joiner(output_stream, delimiter));

    return output_stream << suffix;
}

template<typename Type>
std::ostream& operator<<(std::ostream& output_stream, const std::vector<Type>& container) {
    return PrintContainer(output_stream, container, "["s, "]"s);
}

template<typename Type>
std::ostream& operator<<(std::ostream& output_stream, const std::set<Type>& container) {
    return PrintContainer(output_stream, container, "{"s, "}"s);
}

template<typename Key, typename Value>
std::ostream& operator<<(std::ostream& output_stream, const std::map<Key, Value>& container) {
    return PrintContainer(output_stream, container, "{"s, "}"s);
}

template<typename Actual, typename Expected>
void AssertEqual(const Actual& actual, const Expected& expected,
                 const string& actual_string, const string& expected_string, const string& file,
                 const string& function, unsigned line, const string& hint) {
    if (!(actual == expected)) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << function << ": "s;
        cout << "Assertion ("s << actual_string << ", "s << expected_string << ") failed: "s;
        cout << actual << " != "s << expected << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

void Assert(bool value, const string& value_string, const string& file, const string& function,
            unsigned line, const string& hint) {
    AssertEqual(value, true, value_string, "true", file, function, line, hint);
}

template<typename TestFunc>
void RunTest(TestFunc function, const std::string& test_name) {
    try {
        function();
        std::cerr << test_name << " OK" << std::endl;
    } catch (std::exception& e) {
        std::cerr << test_name << " Fail: " << e.what() << std::endl;
    }
}
} // testing namespace

#define ASSERT_EQUAL(left, right) \
::testing::AssertEqual((left), (right), #left, #right, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(left, right, hint) \
::testing::AssertEqual((left), (right), #left, #right, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT(expr) ::testing::Assert(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) ::testing::Assert(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func)  ::testing::RunTest((func), (#func))

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

    friend ostream& operator<<(ostream& os, const Document& document);

  public: // not private for compatibility with testing system
    int id = 0;
    double relevance = 0.;
    int rating = 0;
};

Document::Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating) {}

int Document::GetId() const {
    return id;
}

int Document::GetRating() const {
    return rating;
}

double Document::GetRelevance() const {
    return relevance;
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
        return left.GetRating() > right.GetRating();
    }
    return left.GetRelevance() > right.GetRelevance();
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
    static constexpr auto kMaxResultDocumentSize = 5U;
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
    void FilterByMinusWords(map<K, V>& relevance_map, const Query& query, Criteria) const {
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
    const double kInvertedWordCount = 1.0 / static_cast<double>(kWords.size());
    for (const string& word : kWords) {
        inverted_index_[word][document_id] += kInvertedWordCount;
    }

    storage_.insert({document_id, DocumentData{ComputeAverageRating(ratings), status}});
}

void SearchServer::CutDocuments(Documents& documents) {
    if (documents.size() > kMaxResultDocumentSize) {
        documents.resize(kMaxResultDocumentSize);
    }
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    const auto ByStatus = [&status](int, DocumentStatus document_status, int) {
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
    return log(static_cast<double>(GetDocumentCount()) / static_cast<double>(inverted_index_.at(word).size()));
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

        search_server.AddDocument(document_id, kDocument, static_cast<DocumentStatus>(status_raw), ratings);
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

// -------- Начало модульных тестов поисковой системы ----------
bool IsDoubleEqual(double left, double right) {
    return std::abs(left - right) < kEpsilon;
}

// Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из
// документа.
void TestSearchOnEmptyBase() {
    const auto kQuery = "foo"s;
    auto server = SearchServer{};
    ASSERT(server.FindTopDocuments(kQuery).empty());
}

void TestFoundAddedDocument() {
    const auto kQuery = "huge"s;
    const int kId = 42;
    auto server = SearchServer{};
    server.AddDocument(kId, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).front().id, kId);
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).size(), 1U);

}

void TestNotFoundAddedDocument() {
    const auto kQuery = "foo"s;
    auto server = SearchServer{};
    server.AddDocument(42, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    ASSERT(server.FindTopDocuments(kQuery).empty());
}

// Поддержка стоп-слов. Стоп-слова исключаются из текста документов.

void TestNotFoundByStopWord() {
    const auto kStopWords = "huge flying green cat"s;

    auto server = SearchServer{};
    server.SetStopWords(kStopWords);
    server.AddDocument(42, string{kStopWords}, DocumentStatus::ACTUAL, {});
    ASSERT(server.FindTopDocuments(kStopWords).empty());
}

void TestFoundByRegularWord() {
    const auto kStopWords = "huge flying green cat"s;
    const auto kRegularWord = "foo"s;
    const auto kId = 42;
    auto server = SearchServer{};
    server.SetStopWords(kStopWords);
    server.AddDocument(kId, string{kStopWords + ' ' + kRegularWord}, DocumentStatus::ACTUAL, {});
    ASSERT_EQUAL(server.FindTopDocuments(kRegularWord).front().id, kId);
    ASSERT_EQUAL(server.FindTopDocuments(kRegularWord).size(), 1U);

}

// Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.

void TestQuerySelfExcludedByMinusWords() {
    const auto kQuery = "huge -huge"s;
    auto server = SearchServer{};
    server.AddDocument(0, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    ASSERT(server.FindTopDocuments(kQuery).empty());
}

void TestFilterSearchResultsByMinusWords() {
    const auto kQuery = "cat -green"s;
    auto server = SearchServer{};
    const auto kId = 13;
    server.AddDocument(0, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    server.AddDocument(kId, string{"big red cat"}, DocumentStatus::ACTUAL, {});
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).front().id, kId);
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).size(), 1U);
}

// Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового
// запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться
// пустой список слов.

void TestDocumentMatchedByPlusWords() {
    const auto kQuery = "cat green"s;
    const auto kExpectedWords = vector<string>{"cat"s, "green"s};
    auto server = SearchServer{};
    const auto kId = 42;
    server.AddDocument(kId, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    const auto[kWords, kStatus] = server.MatchDocument(kQuery, kId);
    ASSERT_EQUAL(static_cast<int>(kStatus), static_cast<int>(DocumentStatus::ACTUAL));
    ASSERT_EQUAL(kWords, kExpectedWords);
}

void TestDocumentMatchedByMinusWords() {
    const auto kQuery = "cat -green"s;
    auto server = SearchServer{};
    const auto kId = 42;
    server.AddDocument(kId, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    const auto[kWords, kStatus] = server.MatchDocument(kQuery, kId);
    ASSERT_EQUAL(static_cast<int>(kStatus), static_cast<int>(DocumentStatus::ACTUAL));
    ASSERT(kWords.empty());
}

// Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты
// должны быть отсортированы в порядке убывания релевантности.

void TestDocumentsSortingByRelevance() {
    const auto kQuery = "oh my cat"s;
    auto server = SearchServer{};
    server.AddDocument(1, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    server.AddDocument(2, string{"big red cat on the cat"}, DocumentStatus::ACTUAL, {});
    server.AddDocument(3, string{"cats against dogs"}, DocumentStatus::ACTUAL, {});
    server.AddDocument(4, string{"my parrot"}, DocumentStatus::ACTUAL, {});
    server.AddDocument(5, string{"oh la la"}, DocumentStatus::ACTUAL, {});

    const auto kResults = server.FindTopDocuments(kQuery);

    ASSERT_HINT(is_sorted(kResults.cbegin(), kResults.cend(),
                          [](const auto& left, const auto& right) { return left.relevance > right.relevance; }),
                "documents must be sorted by relevance"s);
    ASSERT_EQUAL(kResults.size(), 4U);
}

// Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.

void TestRatingCalculation() {
    const auto kQuery = "huge"s;
    auto server = SearchServer{};
    server.AddDocument(42, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {1, 2, 3});
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).front().rating, (1 + 2 + 3) / 3.0);
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).size(), 1U);
}

//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.

void TestSearchByUserPredicate() {
    const auto kQuery = "oh my cat"s;
    auto server = SearchServer{};
    server.AddDocument(1, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {1});
    server.AddDocument(2, string{"big red cat on the cat"}, DocumentStatus::ACTUAL, {2});
    server.AddDocument(3, string{"cats against dogs"}, DocumentStatus::ACTUAL, {3});
    server.AddDocument(4, string{"my parrot"}, DocumentStatus::BANNED, {4});
    server.AddDocument(5, string{"oh la la"}, DocumentStatus::REMOVED, {5});

    const auto ByUserDefined = [](
        [[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]] int rating) {
      return document_status == DocumentStatus::ACTUAL && rating < 3 && document_id % 2 == 0;
    };

    ASSERT_EQUAL(server.FindTopDocuments(kQuery, ByUserDefined).front().id, 2);
    ASSERT_EQUAL(server.FindTopDocuments(kQuery, ByUserDefined).size(), 1U);

}

//Поиск документов, имеющих заданный статус.
void TestFoundAddedDocumentByStatus() {
    const std::string kDocumentBody = "foo";

    SearchServer search_server;
    search_server.AddDocument(static_cast<int>(DocumentStatus::ACTUAL), kDocumentBody, DocumentStatus::ACTUAL, {});
    search_server.AddDocument(static_cast<int>(DocumentStatus::REMOVED), kDocumentBody, DocumentStatus::REMOVED, {});
    search_server.AddDocument(static_cast<int>(DocumentStatus::BANNED), kDocumentBody, DocumentStatus::BANNED, {});
    search_server.AddDocument(static_cast<int>(DocumentStatus::IRRELEVANT), kDocumentBody, DocumentStatus::IRRELEVANT,
                              {});

    for (int status = 0; status < 4; ++status) {
        auto document = search_server.FindTopDocuments(kDocumentBody, static_cast<DocumentStatus>(status)).front();
        ASSERT_EQUAL_HINT(document.id, status, "the converted status must be equal to document id");
    }
}

//Корректное вычисление релевантности найденных документов.
void TestRelevanceCalculation() {
    const auto kQuery = "oh my cat"s;
    auto server = SearchServer{};
    server.SetStopWords("huge flying green cat");
    server.AddDocument(1, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {1});
    server.AddDocument(2, string{"my little red god with fire tail"}, DocumentStatus::ACTUAL, {2});
    server.AddDocument(3, string{"oh la la"}, DocumentStatus::ACTUAL, {3});

    const auto kResults = server.FindTopDocuments(kQuery);

    ASSERT(IsDoubleEqual(server.FindTopDocuments(kQuery)[0].relevance, 0.36620409));
    ASSERT(IsDoubleEqual(server.FindTopDocuments(kQuery)[1].relevance, 0.15694461));
    ASSERT_EQUAL(kResults.size(), 2U);
}

void TestSearchServer() {
    RUN_TEST(TestSearchOnEmptyBase);
    RUN_TEST(TestFoundAddedDocument);
    RUN_TEST(TestNotFoundAddedDocument);
    RUN_TEST(TestNotFoundByStopWord);
    RUN_TEST(TestFoundByRegularWord);
    RUN_TEST(TestQuerySelfExcludedByMinusWords);
    RUN_TEST(TestFilterSearchResultsByMinusWords);
    RUN_TEST(TestDocumentMatchedByPlusWords);
    RUN_TEST(TestDocumentMatchedByMinusWords);
    RUN_TEST(TestDocumentsSortingByRelevance);
    RUN_TEST(TestRatingCalculation);
    RUN_TEST(TestFoundAddedDocumentByStatus);
    RUN_TEST(TestSearchByUserPredicate);
    RUN_TEST(TestRelevanceCalculation);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
}