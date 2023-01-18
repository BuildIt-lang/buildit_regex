#include <iostream>
#include "test.h"
#include "../include/parse.h"
#include "../include/progress.h"
#include "../include/frontend.h"

using namespace std::chrono;

/**
General function to compare results.
*/
void check_correctness(const char* pattern, const char* candidate, const char* flags) {
    bool expected = (strcmp(flags, "i") == 0) ? 
        regex_search(candidate, regex(pattern, regex_constants::icase)) :
        regex_search(candidate, regex(pattern));
    
    int result = compile_and_run(candidate, pattern, MatchType::PARTIAL_SINGLE, 1, flags);
    //int result = compile_and_run_decomposed(candidate, pattern, MatchType::PARTIAL_SINGLE, 1, flags);
    //int result = compile_and_run_partial(candidate, pattern, flags);

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
    check_correctness("4{2,4}", "44");
    check_correctness("4{2,4}", "444");
    check_correctness("4{2,4}", "4444");
    check_correctness("4{2,4}", "44444");
    check_correctness("4{2,4}", "4");
    check_correctness("a{1,4}b{2}", "aabb");
    check_correctness("(a{1,4}b{2}){2}", "aabbaaab");
    check_correctness("(a{1,4}b{2}){2}", "aabbaaabb");
}

void test_combined() {
    check_correctness("(45|ab?)", "ab");    
    check_correctness("(ab?|45)", "ab");    
    check_correctness("(ab?|45)", "ab45");    
    check_correctness("(ab?|45)", "a");
    check_correctness("(ab){4}", "abababab");
    check_correctness("[bde]{2}", "bd");
    check_correctness("((abcd){1}45{3}){2}", "abcd4555abcd4555");
    check_correctness("([abc]3){2}", "a3a3");
    check_correctness("([abc]3){2}", "c3b3");
    check_correctness("([abc]3){2}", "a3c3c3");
    check_correctness("([abc]3){2}", "ac");
}

void test_escaping() {
    check_correctness("a\\d", "texta0text");
    check_correctness("a\\d", "dsaad5f");
    check_correctness("a\\db", "a9b");
    check_correctness("a\\d{3}", "a912aaa");
    check_correctness("bcd\\d*", "bcda");
    check_correctness("bcd\\d*", "bcd567a");
    check_correctness("bcd\\d*", "bcd23");
    check_correctness("a[\\dbc]d", "3a5da");
    check_correctness("a[\\dbc]d", "acd");

    check_correctness("a\\w\\d", "aab5d");
    check_correctness("a\\w\\W", "fa_09");
    check_correctness("a\\w\\d", "aC9");

    check_correctness("\\ss", "sss s");
    check_correctness("\\Da\\Wbc\\S", "aa3bc_");
    check_correctness("\\Dabc\\S", "aabc_00");
    check_correctness("\\da\\wbc\\s", "a7a_bc cc");
}

void test_partial() {
	check_correctness("ab", "aab");
	check_correctness("ab", "aba");
	check_correctness("a?", "aaaa");
	check_correctness("c[ab]+", "abc");
	check_correctness("c[ab]+", "aaba");
	check_correctness("c[ab]+", "caaaabcc");
	check_correctness("123", "a123a");
	check_correctness("(123)*1", "112312311");
    check_correctness("Twain", "MarkTwainTomSawyer");
	//check_correctness("[a-q][^u-z]{5}x", "qax");
	//check_correctness("[a-q][^u-z]{5}x", "qabcdex");
}
void test_ignore_case() {
    check_correctness("abcd", "AbCd", "i");
    check_correctness("a1a*", "a1aaAaA", "i");
    check_correctness("b2[a-g]", "B2D", "i");
    check_correctness("[^a-z]", "D", "i");
    check_correctness("[^a-z][a-z]*", "4Df", "i");
    check_correctness("(abc){3}", "aBcAbCabc", "i");
    check_correctness("(1|2|k)*", "K2", "i");
    check_correctness("z+a", "zZa", "i");
    check_correctness("z?a", "Za", "i");
    check_correctness("\\d\\w\\s", "5a ", "i");
    check_correctness("\\D\\W\\S", "a k", "i");
}

int main() {    
    auto start = high_resolution_clock::now();
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
    test_escaping();
	test_partial();
    test_ignore_case();
    auto end = high_resolution_clock::now();
    auto dur = (duration_cast<seconds>(end - start)).count();
    cout << "time: " << dur << "s" << endl;
}


