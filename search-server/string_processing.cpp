//
// Created by Yegor Chistyakov on 05.02.2022.
//

#include "string_processing.h"


std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream is(text);

    std::string word;
    while (getline(is, word, ' ')) {
        words.push_back(word);
    }

    return words;
}