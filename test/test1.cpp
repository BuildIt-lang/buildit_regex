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
    //check_correctness("a[.]d", "akd");
    check_correctness("a[vd][45]", "ad4");
    check_correctness("2[a-g]", "2d");
    check_correctness("2[a-g]*", "2dcag");
}

void test_negative_brackets() {
    check_correctness("3[^db]4", "3c4");    
    check_correctness("3[^db]4", "3d4");    
    check_correctness("3[^db]4", "3b4"); 
    check_correctness("[abd][^35]*", "a4555dsd");
    check_correctness("[abd]*[^35fds]", "abdd4");
    check_correctness("2[^a-g]", "25");
    check_correctness("2[^a-g]", "2h");
    check_correctness("2[^a-g]", "2a");
}

void test_brackets_and_star() {
    check_correctness("a[bc]*d", "abcd");
    check_correctness("a[bc]*d", "abbd");
    check_correctness("a[bc]*d", "ad");
    check_correctness("a[bc]*d", "abd");
    check_correctness("a[bc]*d", "abcdd");
    check_correctness("3[^db]*4", "3ca8a4");    
    check_correctness("3[^db]*", "3");    
}

void test_grouping() {
    check_correctness("a(bcd)*", "abcdbcd");
    check_correctness("a(bcd)", "abcdbcd");
    check_correctness("a(bcd)", "abcd");
    check_correctness("a(bcd)*", "a");
    check_correctness("a(bcd)*tr", "abcdtr");
    check_correctness("a(bcd)*(tr432)*", "abcdtr432tr432");
    check_correctness("a(bcd)*(tr432)*", "abctr432tr432");
    check_correctness("a[bcd]*(tr432)*", "abtr432tr432");
    check_correctness("(4324)*(abc)", "432443244324abc");
}


int main() {
    test_simple();
    test_star();
    test_brackets();
    test_negative_brackets();
    test_brackets_and_star();
    test_grouping();
}


