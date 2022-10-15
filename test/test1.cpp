#include <iostream>
#include "test.h"
#include "../include/util.h"

/**
General function to compare results.
*/
void check_correctness(const char* pattern, const char* candidate) {
    bool expected = std::regex_match(candidate, std::regex(pattern));
    int len = strlen(candidate); 
	builder::builder_context context;
	context.feature_unstructured = true;
	context.run_rce = true;
    auto fptr = (int (*)(const char*, int))builder::compile_function_with_context(context, match_regex, pattern);
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
    check_correctness("a[bc]d", "abcd");
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
    check_correctness("a[bc]*c", "ac");
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

void test_nested_grouping() {
    check_correctness("a((bcd)*34)*", "abcdbcd34bcd34");
    check_correctness("a(a(bc|45))*", "aacaca4a5");
    check_correctness("a(a(bc|45))*", "aabcabca45");
    check_correctness("(a(bc|45)c)?d", "a45cd");
    check_correctness("(a(bc|45)c)?d", "d");
}

void test_or_groups() {
    check_correctness("ab(c|56|de)", "ab56");
    check_correctness("ab(c|56|de)k", "abck");
    check_correctness("ab(c|56|de)k", "ab56k");
    check_correctness("ab(c|56|de)k", "abdek");
    check_correctness("ab(c|56|de)k", "ab56dek");
    check_correctness("a(cbd|45)*", "acbd45cbd");
    check_correctness("([abc]|dc|4)2", "b2");
    check_correctness("([abc]|dc|4)2", "b2");
    check_correctness("(dc|[abc]|4)2", "b2");
    check_correctness("(dc|4|[abc])2", "b2");
    check_correctness("([^23]|2)abc", "1abc");
    check_correctness("([^23]|2)abc", "3abc");
    check_correctness("(a|[abc]|4)2", "ac42");
    check_correctness("(bc|de|[23]*)", "22323");
    check_correctness("(bc|de|[23]*)", "bc");
    check_correctness("(bc|de|[23]*)", "bcbc");
    check_correctness("(bc|de|[23]*)", "bc23");
    check_correctness("a(bc|de|[23]*)", "a");
    check_correctness("a(bc|de|[23]+)", "a");
    check_correctness("a(bc|de|[23]+)", "a2332");
    check_correctness("a(bc|de|[23]?)", "a23");
}

void test_plus() {
    check_correctness("a+bc", "aaaabc");	
    check_correctness("a+bc", "bc");
    check_correctness("a+b+c", "aabbbc");
    check_correctness("[abc]+d", "abcd");
    check_correctness("[abc]+d", "bbbd");
    check_correctness("[abc]+d", "d");
    check_correctness("[abc]+c", "c");
    check_correctness("[abc]+c", "cc");
}

void test_question() {
    check_correctness("a?bc", "abc");    
    check_correctness("a?bc", "bc");
    check_correctness("a?bc", "aaaabc");
    check_correctness("a?b?c?", "ac");
    check_correctness("[abc]?de", "abcde");
    check_correctness("[abc]?de", "cde");
    check_correctness("[abc]?de", "de");
    check_correctness("[abc]?de", "abcabcde");
    check_correctness("(abc)*(ab)?", "abcabc");
    check_correctness("(abc)*(ab)?", "ab");
}

void test_repetition() {
    check_correctness("a{5}", "aaaaa");    
    check_correctness("a{5}", "aaaaaa");    
    check_correctness("a{5}", "aaaa");  
    check_correctness("ab{3}", "abbb");  
    check_correctness("ab{3}", "ababab");  
    check_correctness("ab{3}c", "abbbc");  
    check_correctness("ab{3}c", "abbc");  
    check_correctness("ab{3}c", "abbbbc");  
    check_correctness("ab{10}c", "abbbbbbbbbbc");  
}

void test_combined() {
    check_correctness("(45|ab?)", "ab");    
    check_correctness("(ab?|45)", "ab");    
    check_correctness("(ab?|45)", "ab45");    
    check_correctness("(ab?|45)", "a");
    check_correctness("(ab){4}", "abababab");
    check_correctness("[bde]{2}", "bd");
    check_correctness("((abcd){1}45{3}){2}", "abcd4555abcd4555"); 
}

int main() {

    test_simple();
    test_star();
    test_brackets();
    test_negative_brackets();
    test_brackets_and_star();
    test_grouping();
    test_nested_grouping();
    test_or_groups();
    test_plus();
    test_question();
    test_repetition();
    test_combined();
}


