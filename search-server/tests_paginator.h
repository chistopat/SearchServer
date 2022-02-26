#pragma once

#include "test_framework.h"
#include "paginator.h"

#include <numeric>
#include <sstream>


using namespace std;

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
    for (const auto &page: paginate_v) {
        for (int x: page) {
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
    for (const auto &page: letters_pagination) {
        page_sizes.push_back(page.size());
    }

    const vector<size_t> expected = {11, 11, 4};
    ASSERT_EQUAL(page_sizes, expected);
}

void TestConstContainer() {
    const string letters = "abcdefghijklmnopqrstuvwxyz";

    vector<string> pages;
    for (const auto &page: Paginate(letters, 10)) {
        pages.emplace_back(page.begin(), page.end());
    }

    const vector<string> expected = {"abcdefghij", "klmnopqrst", "uvwxyz"};
    ASSERT_EQUAL(pages, expected);
}

void TestPagePagination() {
    vector<int> v(22);
    iota(begin(v), end(v), 1);

    vector<vector<int>> lines;
    for (const auto &split_by_9: Paginate(v, 9)) {
        for (const auto &split_by_4: Paginate(split_by_9, 4)) {
            lines.emplace_back();
            for (int item: split_by_4) {
                lines.back().push_back(item);
            }
        }
    }

    const vector<vector<int>> expected = {
            {1,  2,  3,  4},
            {5,  6,  7,  8},
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
    std::cerr << std::endl;
}
