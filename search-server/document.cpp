//
// Created by Yegor Chistyakov on 05.02.2022.
//

#include "document.h"


Document::Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating) {
}

std::ostream& operator<<(std::ostream& os, const Document& document) {
    os << "{ "s
       << "document_id = "s << document.id << ", "s
       << "relevance = "s << document.relevance << ", "s
       << "rating = "s << document.rating
       << " }"s;
    return os;
}

bool IsDoubleEqual(double left, double right) {
    const double kEpsilon = 1e-6;
    return std::abs(left - right) < kEpsilon;
}

bool operator<(const Document& left, const Document& right) {
    if (IsDoubleEqual(left.relevance, right.relevance)) {
        return left.rating > right.rating;
    }
    return left.relevance > right.relevance;
}

std::ostream& operator<<(std::ostream& os, DocumentStatus status) {
    return os << static_cast<int>(status);
}

[[maybe_unused]] void PrintDocument(const Document& document) {
    std::cout << document << std::endl;
}
