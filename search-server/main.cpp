#include "search_server.h"
#include "test_runner.h"
#include "paginator.h"

#include <cmath>
#include <numeric>

using namespace std;

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
    const auto[kWords, kStatus] = server.MatchDocument(kQuery, kId);

    ASSERT_EQUAL(kStatus, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(kWords, kExpectedWords);
}

void TestDocumentMatchedByMinusWords() {
    const string kQuery = "cat -green"s;
    SearchServer server;
    const int kId = 42;

    server.AddDocument(kId, string{"huge flying green cat"}, DocumentStatus::ACTUAL, {});
    const auto[kWords, kStatus] = server.MatchDocument(kQuery, kId);

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

// Корректное вычисление релевантности найденных документов.

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

// Конструктор должен выбрасывать исключение если стоп-слова содержат спец-символы

void TestConstuctorParametersValidation() {
    { [[maybe_unused]] SearchServer server; }
    { [[maybe_unused]] SearchServer server("alpha bravo charley delta"s); }
    { [[maybe_unused]] SearchServer server(vector<string>{"alfa"s, "bravo"s, "charley"s, "delta"s}); }
    { [[maybe_unused]] SearchServer server(set<string>{"alfa"s, "bravo"s, "charley"s, "delta"s}); }

    CheckThrow<invalid_argument>([]() { SearchServer{"al\x12pha bravo cha\x24rley delta"s}; });
}

// Валидация данных при добавлении документа

void TestAddDocumentValidation() {
    SearchServer server;

    CheckThrow<invalid_argument>([&server]() { server.AddDocument(-1, {}, DocumentStatus::ACTUAL, {}); });

    CheckThrow<invalid_argument>(
        [&server]() {
          server.AddDocument(1, {}, DocumentStatus::ACTUAL, {});
          server.AddDocument(1, {}, DocumentStatus::ACTUAL, {});
        }
    );

    CheckThrow<invalid_argument>(
        [&server]() { server.AddDocument(1, "al\x12pha bravo cha\x24rley delta"s, DocumentStatus::ACTUAL, {}); }
    );

    server.AddDocument(2, "alpha bravo charley delta"s, DocumentStatus::ACTUAL, {});
}

void TestSearchQueryValidation() {
    SearchServer server;
    server.AddDocument(2, "alpha bravo charley delta"s, DocumentStatus::ACTUAL, {});

    CheckThrow<invalid_argument>([&server]() { server.FindTopDocuments("al\x12pha"s); });
    CheckThrow<invalid_argument>([&server]() { server.FindTopDocuments("--alpha"s); });
    CheckThrow<invalid_argument>([&server]() { server.FindTopDocuments("-"s); });

    ASSERT_EQUAL(server.FindTopDocuments("alpha"s).size(), 1U);
}

void TestGetDocumentId() {
    SearchServer server;
    server.AddDocument(1, "alpha"s, DocumentStatus::ACTUAL, {});
    server.AddDocument(2, "bravo"s, DocumentStatus::ACTUAL, {});
    server.AddDocument(3, "charley"s, DocumentStatus::ACTUAL, {});

    ASSERT_EQUAL(server.GetDocumentId(2), 3);
    ASSERT_EQUAL(server.GetDocumentId(0), 1);

    CheckThrow<out_of_range>([&server]() { server.GetDocumentId(-1); });
    CheckThrow<out_of_range>([&server]() { server.GetDocumentId(42); });
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
    RUN_TEST(TestConstuctorParametersValidation);
    RUN_TEST(TestAddDocumentValidation);
    RUN_TEST(TestSearchQueryValidation);
    RUN_TEST(TestGetDocumentId);
}


void TestPageCounts() {
    vector<int> v(15);
    ASSERT_EQUAL(Paginate(v, 1).size(), v.size());
    ASSERT_EQUAL(Paginate(v, 3).size(), 5u);
    ASSERT_EQUAL(Paginate(v, 5).size(), 3u);
    ASSERT_EQUAL(Paginate(v, 4).size(), 4u);
    ASSERT_EQUAL(Paginate(v, 15).size(), 1u);
    ASSERT_EQUAL(Paginate(v, 150).size(), 1u);
    ASSERT_EQUAL(Paginate(v, 14).size(), 2u);
}

void TestLooping() {
    vector<int> v(15);
    iota(begin(v), end(v), 1);

    Paginator<vector<int>::iterator> paginate_v(v.begin(), v.end(), 6);
    ostringstream os;
    for (const auto& page : paginate_v) {
        for (int x : page) {
            os << x << ' ';
        }
        os << '\n';
    }
    string res = os.str();
    ASSERT_EQUAL(res, "1 2 3 4 5 6 \n7 8 9 10 11 12 \n13 14 15 \n");
}

void TestPageSizes() {
    string letters(26, ' ');

    Paginator letters_pagination(letters.begin(), letters.end(), 11);
    vector<size_t> page_sizes;
    for (const auto& page : letters_pagination) {
        page_sizes.push_back(page.size());
    }

    const vector<size_t> expected = {11, 11, 4};
    ASSERT_EQUAL(page_sizes, expected);
}


void TestConstContainer() {
    const string letters = "abcdefghijklmnopqrstuvwxyz";

    vector<string> pages;
    for (const auto& page : Paginate(letters, 10)) {
        pages.emplace_back(page.begin(), page.end());
    }

    const vector<string> expected = {"abcdefghij", "klmnopqrst", "uvwxyz"};
    ASSERT_EQUAL(pages, expected);
}

void TestPagePagination() {
    vector<int> v(22);
    iota(begin(v), end(v), 1);

    vector<vector<int>> lines;
    for (const auto& split_by_9 : Paginate(v, 9)) {
        for (const auto& split_by_4 : Paginate(split_by_9, 4)) {
            lines.emplace_back();
            for (int item : split_by_4) {
                lines.back().push_back(item);
            }
        }
    }

    const vector<vector<int>> expected = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9},
        {10, 11, 12, 13},
        {14, 15, 16, 17},
        {18},
        {19, 20, 21, 22}
    };
    ASSERT(lines == expected);
}

void TestPaginator() {
    RUN_TEST(TestPageCounts);
    RUN_TEST(TestLooping);
    RUN_TEST(TestPageSizes);
    RUN_TEST(TestConstContainer);
    RUN_TEST(TestPagePagination);
}

int main() {
    TestPaginator();
    TestSearchServer();
}