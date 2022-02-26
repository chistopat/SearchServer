#include "read_input_functions.h"


std::string ReadLine(std::istream &in) {
    std::string s;
    getline(in, s);
    return s;
}

int ReadLineWithNumber(std::istream &in) {
    int result;
    in >> result;
    ReadLine();
    return result;
}