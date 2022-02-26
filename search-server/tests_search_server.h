#pragma once

#include "search_server.h"
#include "test_framework.h"

#include <cmath>

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

void TestIterateByServer() {
    SearchServer server;
    server.AddDocument(1, "alpha"s, DocumentStatus::ACTUAL, {});
    server.AddDocument(2, "bravo"s, DocumentStatus::ACTUAL, {});
    server.AddDocument(3, "charley"s, DocumentStatus::ACTUAL, {});

    int counter = 0;
    for (const auto kId : server) {
        ASSERT_EQUAL(kId, ++counter);
    }
}

void TestIterateByConstServer() {
    SearchServer server;
    server.AddDocument(1, "alpha"s, DocumentStatus::ACTUAL, {});
    server.AddDocument(2, "bravo"s, DocumentStatus::ACTUAL, {});
    server.AddDocument(3, "charley"s, DocumentStatus::ACTUAL, {});

    const auto& kServer = server;
    int counter = 0;
    for (const auto kId : kServer) {
        ASSERT_EQUAL(kId, ++counter);
    }
}

void TestGetWordFrequencies() {
    SearchServer server;
    server.AddDocument(2, "alpha bravo charley delta"s, DocumentStatus::ACTUAL, {});
    std::map<std::string, double> expected = {
            {"alpha", 0.25},
            {"bravo", 0.25},
            {"charley", 0.25},
            {"delta", 0.25}
    };
    const auto result = server.GetWordFrequencies(2);
    for (auto& [word, frequency] : result) {
        ASSERT(IsDoubleEqual(expected[word], frequency));
    }
}

void TestGetWordFrequenciesWrongId() {
    SearchServer server;
    ASSERT(server.GetWordFrequencies(2).empty());
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
    RUN_TEST(TestIterateByServer);
    RUN_TEST(TestIterateByConstServer);
    RUN_TEST(TestGetWordFrequenciesWrongId);
    RUN_TEST(TestGetWordFrequencies);
    std::cerr << std::endl;
}
