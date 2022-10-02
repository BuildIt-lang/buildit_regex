#include <iostream>
#include "test.h"

/**
General function to compare results.
*/
void check_correctness(const char* pattern, const char* candidate) {
    bool expected = std::regex_match(candidate, std::regex(pattern));
    int len = strlen(candidate);
    auto fptr = (int (*)(const char*, int))builder::compile_function(match_regex, pattern);
    int result = fptr((char*)candidate, len);
    std::cout << "Matching " << pattern << " with " << candidate << " -> ";
    bool match = (result == expected);
    if (match) {
        std::cout << "ok. Result is: " << result << std::endl;
    } else {
        std::cout << "failed\nExpected: " << expected << ", got: " << result << std::endl;
    }
}

void test_simple() {
    check_correctness("aaaa", "aaaa");
    check_correctness("", "");
    check_correctness("abc", "abc");
    check_correctness("aabc", "aabc");
}

void test_star() {
    check_correctness("a*bc", "abc");    
    check_correctness("a*bc", "aaaabc");
    check_correctness("a*b*c", "aaabbc");
}

void test_brackets() {
    check_correctness("a[bc]d", "abd");
    check_correctness("a[bc]d", "acd");
    check_correctness("a[]d", "ad");
    check_correctness("[amn]", "m");
    check_correctness("[amn]", "mn");
    check_correctness("a[bc]d", "abcd");
    check_correctness("a[.]d", "akd");
}

void test_brackets_and_star() {
    check_correctness("a[bc]*d", "abcd");
    check_correctness("a[bc]*d", "abbd");
    check_correctness("a[bc]*d", "ad");
    check_correctness("a[bc]*d", "abd");
    check_correctness("a[bc]*d", "abcdd");
}


int main() {
    test_simple();
    test_star();
    test_brackets();
    test_brackets_and_star();
}


