#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer &search_server) {
    std::set<std::set<std::string>> hash_table;

    std::vector<int> bin;
    bin.reserve(search_server.GetDocumentCount());

    for (const int kId: search_server) {
        auto words_counter = search_server.GetWordFrequencies(kId);
        std::set<std::string> words;
        std::transform(words_counter.begin(), words_counter.end(),
                       std::inserter(words, words.end()),
                       [](const auto &pair) { return pair.first; });
        if (hash_table.count(words)) {
            bin.push_back(kId);
            continue;
        }
        hash_table.insert(words);
    }

    for (const auto kId: bin) {
        search_server.RemoveDocument(kId);
        std::cout << "Found duplicate document id " << kId << std::endl;
    }
}
