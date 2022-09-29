#include <iostream>
#include "test.h"

/**
General function to compare results.
*/
void check_correctness(const char* pattern, const char* candidate) {
    bool expected = std::regex_match(candidate, std::regex(pattern));
    int len = sizeof(candidate) / sizeof(char);
    auto fptr = (int (*)(const char*, int))builder::compile_function(match_regex, pattern);
    int result = fptr((char*)candidate, len);
    std::cout << "Matching " << pattern << " with " << candidate << " -> ";
    if (result != expected) {
        std::cout << "failed\nExpected: " << expected << ", got: " << result << std::endl;
        assert(false);
    }
    std::cout << "ok" << std::endl;
}

void test_simple() {
    check_correctness("aaaa", "aaaa");
    check_correctness("", "");
    check_correctness("abc", "abc");
    check_correctness("aabc", "aabc");
}

void test_star() {
    check_correctness("a*bc", "abc");    
}


int main() {
    test_simple();
    test_star();

}


