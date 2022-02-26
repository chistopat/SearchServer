#pragma once

#include <iostream>
#include <utility>
#include <experimental/iterator>
#include <set>
#include <map>
#include <string>


using namespace std::string_literals;

template<typename Key, typename Value>
std::ostream &operator<<(std::ostream &output_stream, const std::pair<Key, Value> &container) {
    return output_stream << container.first << ": "s << container.second;
}

template<typename Container>
std::ostream &PrintContainer(std::ostream &output_stream, const Container &container, std::string &&prefix,
                             std::string &&suffix, std::string &&delimiter = ", "s) {
    using namespace std::experimental;

    output_stream << prefix;
    std::copy(std::begin(container), std::end(container), make_ostream_joiner(output_stream, delimiter));

    return output_stream << suffix;
}

template<typename Type>
std::ostream &operator<<(std::ostream &output_stream, const std::vector<Type> &container) {
    return PrintContainer(output_stream, container, "["s, "]"s);
}

template<typename Type>
std::ostream &operator<<(std::ostream &output_stream, const std::set<Type> &container) {
    return PrintContainer(output_stream, container, "{"s, "}"s);
}

template<typename Key, typename Value>
std::ostream &operator<<(std::ostream &output_stream, const std::map<Key, Value> &container) {
    return PrintContainer(output_stream, container, "{"s, "}"s);
}

template<typename Actual, typename Expected>
void AssertEqual(const Actual &actual, const Expected &expected,
                 const std::string &actual_string, const std::string &expected_string, const std::string &file,
                 const std::string &function, unsigned line, const std::string &hint) {
    if (!(actual == expected)) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << function << ": "s;
        std::cout << "Assertion ("s << actual_string << ", "s << expected_string << ") failed: "s;
        std::cout << actual << " != "s << expected << "."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

void Assert(bool value, const std::string &value_string, const std::string &file, const std::string &function,
            unsigned line, const std::string &hint) {
    AssertEqual(value, true, value_string, "true", file, function, line, hint);
}

template<typename TestFunc>
void RunTest(TestFunc function, const std::string &test_name) {
    try {
        function();
        std::cerr << test_name << " OK" << std::endl;
    } catch (std::exception &e) {
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

template<typename Exception, typename Function>
void CheckThrow(Function func) {
    try {
        func();
    } catch (const Exception &e) {
        ASSERT_HINT(true, e.what());
        return;
    } catch (...) {
        ASSERT_HINT(false, "unexpected exception");
    }
    ASSERT_HINT(false, "exception was not thrown");
}
