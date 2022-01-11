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

#define ASSERT_EQUAL(left, right) \
AssertEqual((left), (right), #left, #right, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(left, right, hint) \
AssertEqual((left), (right), #left, #right, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT(expr) Assert(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) Assert(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func)  RunTest((func), (#func))


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


static constexpr auto kEpsilon = 1e-6;

struct Document {
    int id = 0;
    double relevance = 0.;
    int rating = 0;
};

ostream& operator<<(ostream& os, const Document& document) {
    os << "{ "s
       << "document_id = "s << document.id << ", "s
       << "relevance = "s << document.relevance << ", "s
       << "rating = "s << document.rating
       << " }"s;
    return os;
}

bool operator<(const Document& left, const Document& right) {
    if (abs(left.relevance - right.relevance) < kEpsilon) {
        return left.rating > right.rating;
    }
    return left.relevance > right.relevance;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

ostream& operator<<(ostream& os, DocumentStatus status) {
    return os << static_cast<int>(status);
}

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

        if (matched_documents.size() > kMaxResultDocumentSize) {
            matched_documents.resize(kMaxResultDocumentSize);
        }

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const;

    vector<Document> FindTopDocuments(const string& raw_query) const;

    size_t GetDocumentCount() const;

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;

  private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
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

    vector<string> SplitIntoWordsNoStop(const string& text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    void MatchByPlusWords(const Query& query, int document_id, vector<string>& matched) const;

    void MatchByMinusWords(const Query& query, int document_id, vector<string>& matched) const;

    Query ParseQuery(const string& text) const;

    double ComputeWordInverseDocumentFrequency(const string& word) const;

    template<typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {
        map<int, double> document_to_relevance;

        for (const string& word : query.GetPlusWords()) {
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

        for (const string& word : query.GetMinusWords()) {
            if (word_to_document_frequency_.count(word) == 1U) {
                for (const auto[kDocumentId, _] : word_to_document_frequency_.at(word)) {
                    document_to_relevance.erase(kDocumentId);
                }
            }
        }

        return MakeDocuments(document_to_relevance);
    }


    vector<Document> MakeDocuments(const map<int, double>& document_to_relevance) const;

  private:
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_frequency_;
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

void SearchServer::SetStopWords(const string& text) {
    for (const string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status,
                               const vector<int>& ratings) {
    const vector<string> kWords = SplitIntoWordsNoStop(document);
    const double kInvertedWordCount = 1.0 / static_cast<double>(kWords.size());
    for (const string& word : kWords) {
        word_to_document_frequency_[word][document_id] += kInvertedWordCount;
    }

    storage_.insert({document_id, DocumentData{ComputeAverageRating(ratings), status}});
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [&status](int, DocumentStatus document_status, int) {
      return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    const Query kQuery = ParseQuery(raw_query);
    vector<string> matched_words;

    MatchByPlusWords(kQuery, document_id, matched_words);
    MatchByMinusWords(kQuery, document_id, matched_words);

    return {matched_words, storage_.at(document_id).status};

}

size_t SearchServer::GetDocumentCount() const {
    return storage_.size();
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0U;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
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
    for (const string& word : SplitIntoWords(text)) {
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

double SearchServer::ComputeWordInverseDocumentFrequency(const string& word) const {
    return log(
        static_cast<double>(GetDocumentCount()) / static_cast<double>(word_to_document_frequency_.at(word).size()));
}

vector<Document> SearchServer::MakeDocuments(const map<int, double>& document_to_relevance) const {
    vector<Document> documents;
    documents.reserve(document_to_relevance.size());

    for (const auto[kDocumentId, kRelevance] : document_to_relevance) {
        documents.push_back({kDocumentId, kRelevance, storage_.at(kDocumentId).rating});
    }

    return documents;
}

void SearchServer::MatchByPlusWords(const SearchServer::Query& query, int document_id, vector<string>& matched) const {
    for (const string& word : query.GetPlusWords()) {
        if (word_to_document_frequency_.count(word) == 1U) {
            if (word_to_document_frequency_.at(word).count(document_id) == 1U) {
                matched.push_back(word);
            }
        }
    }
}

void SearchServer::MatchByMinusWords(const SearchServer::Query& query, int document_id, vector<string>& matched) const {
    for (const string& word : query.GetMinusWords()) {
        if (word_to_document_frequency_.count(word) == 1U) {
            if (word_to_document_frequency_.at(word).count(document_id) == 1U) {
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
    search_server.SetStopWords(ReadLine());

    const int kDocumentCount = ReadLineWithNumber();
    for (int document_id = 0; document_id < kDocumentCount; ++document_id) {
        const string kDocument = ReadLine();

        int status_raw;
        cin >> status_raw;

        size_t ratings_size;
        cin >> ratings_size;

        vector<int> ratings(ratings_size, 0);

        for (auto& rating : ratings) {
            cin >> rating;
        }

        search_server.AddDocument(document_id, kDocument, static_cast<DocumentStatus>(status_raw), ratings);
        ReadLine();
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
    const string kQuery = "foo"s;
    SearchServer server;

    ASSERT(server.FindTopDocuments(kQuery).empty());
}

void TestFoundAddedDocument() {
    const string kQuery = "huge"s;
    const int kId = 42;
    SearchServer server;

    server.AddDocument(kId, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});

    ASSERT_EQUAL(server.FindTopDocuments(kQuery).front().id, kId);
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).size(), 1U);
}

void TestNotFoundAddedDocument() {
    const string kQuery = "foo"s;
    SearchServer server;

    server.AddDocument(42, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});

    ASSERT(server.FindTopDocuments(kQuery).empty());
}

// Поддержка стоп-слов. Стоп-слова исключаются из текста документов.

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int kDocId = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(kDocId, content, DocumentStatus::ACTUAL, ratings);
        const vector<Document> kFoundDocs = server.FindTopDocuments("in"s);

        ASSERT_EQUAL(kFoundDocs.size(), 1U);
        ASSERT_EQUAL(kFoundDocs.front().id, kDocId);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(kDocId, content, DocumentStatus::ACTUAL, ratings);

        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

// Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.

void TestQuerySelfExcludedByMinusWords() {
    const string kQuery = "huge -huge"s;
    SearchServer server;

    server.AddDocument(0, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});

    ASSERT(server.FindTopDocuments(kQuery).empty());
}

void TestSearchResultsByMinusWords() {
    const string kQuery = "cat -green"s;
    SearchServer server;
    const int kId = 13;

    server.AddDocument(0, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    server.AddDocument(kId, string{"big red cat"}, DocumentStatus::ACTUAL, {});

    ASSERT_EQUAL(server.FindTopDocuments(kQuery).front().id, kId);
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).size(), 1U);
}

// Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового
// запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться
// пустой список слов.

void TestDocumentMatchedByPlusWords() {
    const string kQuery = "cat green"s;
    const vector<string> kExpectedWords{"cat"s, "green"s};
    SearchServer server;
    const int kId = 42;

    server.AddDocument(kId, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    const auto [kWords, kStatus] = server.MatchDocument(kQuery, kId);

    ASSERT_EQUAL(kStatus, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(kWords, kExpectedWords);
}

void TestDocumentMatchedByMinusWords() {
    const string kQuery = "cat -green"s;
    SearchServer server;
    const int kId = 42;

    server.AddDocument(kId, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    const auto [kWords, kStatus] = server.MatchDocument(kQuery, kId);

    ASSERT_EQUAL(static_cast<int>(kStatus), static_cast<int>(DocumentStatus::ACTUAL));
    ASSERT(kWords.empty());
}

// Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты
// должны быть отсортированы в порядке убывания релевантности.

void TestDocumentsSortingByRelevance() {
    const string kQuery = "oh my cat"s;
    SearchServer server;
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
    const string kQuery = "huge"s;
    SearchServer server;
    server.AddDocument(42, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {1, 2, 3});
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).front().rating, (1 + 2 + 3) / 3.0);
    ASSERT_EQUAL(server.FindTopDocuments(kQuery).size(), 1U);
}

//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.

void TestSearchByUserPredicate() {
    const string kQuery = "oh my cat"s;
    SearchServer server;
    server.AddDocument(1, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {1});
    server.AddDocument(2, string{"big red cat on the cat"}, DocumentStatus::ACTUAL, {2});
    server.AddDocument(3, string{"cats against dogs"}, DocumentStatus::ACTUAL, {3});
    server.AddDocument(4, string{"my parrot"}, DocumentStatus::BANNED, {4});
    server.AddDocument(5, string{"oh la la"}, DocumentStatus::REMOVED, {5});

    const auto kByUserDefined = [](int document_id, DocumentStatus document_status, int rating) {
      return document_status == DocumentStatus::ACTUAL && rating < 3 && document_id % 2 == 0;
    };

    ASSERT_EQUAL(server.FindTopDocuments(kQuery, kByUserDefined).front().id, 2);
    ASSERT_EQUAL(server.FindTopDocuments(kQuery, kByUserDefined).size(), 1U);

}

//Поиск документов, имеющих заданный статус.

void TestFoundAddedDocumentByStatus() {
    const std::string kDocumentBody = "foo";

    SearchServer search_server;
    search_server.AddDocument(0, kDocumentBody, DocumentStatus::ACTUAL, {});
    search_server.AddDocument(1, kDocumentBody, DocumentStatus::IRRELEVANT, {});
    search_server.AddDocument(2, kDocumentBody, DocumentStatus::BANNED, {});
    search_server.AddDocument(3, kDocumentBody, DocumentStatus::REMOVED, {});

    const int kDocumentStatusCount = 4;
    for (int i = 0; i < kDocumentStatusCount; ++i) {
        auto document = search_server.FindTopDocuments(kDocumentBody, static_cast<DocumentStatus>(i)).front();
        ASSERT_EQUAL_HINT(document.id, i, "the converted status must be equal to document id");
    }
}

//Корректное вычисление релевантности найденных документов.

void TestRelevanceCalculation() {
    const string kQuery = "dog"s;
    SearchServer server;
    server.SetStopWords("huge flying green cat");
    server.AddDocument(1, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {1});
    server.AddDocument(2, string{"my little red dog with fire tail"}, DocumentStatus::ACTUAL, {2});
    server.AddDocument(3, string{"oh la la"}, DocumentStatus::ACTUAL, {3});

    const vector<Document> kResults = server.FindTopDocuments(kQuery);
    double relevance = 1.0 / 7.0 * log(3.0 / 1.0);

    ASSERT(IsDoubleEqual(relevance, kResults.front().relevance));
}

void TestSearchServer() {
    RUN_TEST(TestSearchOnEmptyBase);
    RUN_TEST(TestFoundAddedDocument);
    RUN_TEST(TestNotFoundAddedDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestQuerySelfExcludedByMinusWords);
    RUN_TEST(TestSearchResultsByMinusWords);
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