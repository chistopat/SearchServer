#pragma once

#include <iostream>

using namespace std::string_literals;

struct Document {
    explicit Document(int id = 0, double relevance = 0.0, int rating = 0);

    int id;
    double relevance;
    int rating;
};

std::ostream& operator<<(std::ostream& os, const Document& document);

bool operator<(const Document& left, const Document& right);

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

std::ostream& operator<<(std::ostream& os, DocumentStatus status);

void PrintDocument(const Document& document);

bool IsDoubleEqual(double left, double right);
