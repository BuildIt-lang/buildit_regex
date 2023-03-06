#include <iostream>
#include "test.h"
#include "../include/parse.h"
#include "../include/progress.h"
#include "../include/frontend.h"

using namespace std::chrono;

void test_simple(MatchType type) {
    compare_result("aaaa", "aaaa", "", type);
    compare_result("", "", "", type);
    compare_result("abc", "abc", "", type);
    compare_result("aabc", "aabc", "", type);
}

void test_star(MatchType type) {
    compare_result("a*bc", "abc", "", type);    
    compare_result("a*bc", "aaaabc", "", type);
    compare_result("a*b*c", "aaabbc", "", type);
}

void test_brackets(MatchType type) {
	compare_result("[bc]", "b", "", type);
	compare_result("[bc]", "c", "", type);
	compare_result("[bc]d", "bd", "", type);
	compare_result("[b-f]d", "dd", "", type);
	compare_result("a[bc]", "ab", "", type);
    compare_result("a[bc]d", "abd", "", type);
    compare_result("a[bc]d", "acd", "", type);
    compare_result("a[bc]d", "abcd", "", type);
    compare_result("a[]d", "ad", "", type);
    compare_result("[amn]", "m", "", type);
    compare_result("[amn]", "mn", "", type);
    compare_result("a[bc]d", "abcd", "", type);
    compare_result("a[vd][45]", "ad4", "", type);
    compare_result("2[a-g]", "2d", "", type);
    compare_result("2[a-g]*", "2dcag", "", type);
}

void test_negative_brackets(MatchType type) {
    compare_result("3[^db]4", "3c4", "", type);    
    compare_result("3[^db]4", "3d4", "", type);    
    compare_result("3[^db]4", "3b4", "", type); 
    compare_result("[abd][^35]*", "a4555dsd", "", type);
    compare_result("[abd]*[^35fds]", "abdd4", "", type);
    compare_result("2[^a-g]", "25", "", type);
    compare_result("2[^a-g]", "2h", "", type);
    compare_result("2[^a-g]", "2a", "", type);
}

void test_brackets_and_star(MatchType type) {
    compare_result("a[bc]*d", "abcd", "", type);
    compare_result("a[bc]*d", "abbd", "", type);
    compare_result("a[bc]*d", "ad", "", type);
    compare_result("a[bc]*d", "abd", "", type);
    compare_result("a[bc]*d", "abcdd", "", type);
    compare_result("a[bc]*c", "ac", "", type);
    compare_result("3[^db]*4", "3ca8a4", "", type);    
    compare_result("3[^db]*", "3", "", type);    
}

void test_grouping(MatchType type) {
    compare_result("a(bcd)*", "abcdbcd", "", type);
    compare_result("a(bcd)", "abcdbcd", "", type);
    compare_result("a(bcd)", "abcd", "", type);
    compare_result("a(bcd)*", "a", "", type);
    compare_result("a(bcd)*tr", "abcdtr", "", type);
    compare_result("a(bcd)*(tr432)*", "abcdtr432tr432", "", type);
    compare_result("a(bcd)*(tr432)*", "abctr432tr432", "", type);
    compare_result("a[bcd]*(tr432)*", "abtr432tr432", "", type);
    compare_result("(4324)*(abc)", "432443244324abc", "", type);
}

void test_nested_grouping(MatchType type) {
    compare_result("a((bcd)*34)*", "abcdbcd34bcd34", "", type);
    compare_result("a(a(bc|45))*", "aacaca4a5", "", type);
    compare_result("a(a(bc|45))*", "aabcabca45", "", type);
    compare_result("(a(bc|45)c)?d", "a45cd", "", type);
    compare_result("(a(bc|45)c)?d", "d", "", type);
}

void test_or_groups(MatchType type) {
    compare_result("ab(c|56|de)", "ab56", "", type);
    compare_result("ab(c|56|de)k", "abck", "", type);
    compare_result("ab(c|56|de)k", "ab56k", "", type);
    compare_result("ab(c|56|de)k", "abdek", "", type);
    compare_result("ab(c|56|de)k", "ab56dek", "", type);
    compare_result("a(cbd|45)*", "acbd45cbd", "", type);
    compare_result("([abc]|dc|4)2", "b2", "", type);
    compare_result("([abc]|dc|4)2", "b2", "", type);
    compare_result("(dc|[abc]|4)2", "b2", "", type);
    compare_result("(dc|4|[abc])2", "b2", "", type);
    compare_result("([^23]|2)abc", "1abc", "", type);
    compare_result("([^23]|2)abc", "3abc", "", type);
    compare_result("(a|[abc]|4)2", "ac42", "", type);
    compare_result("(bc|de|[23]*)", "22323", "", type);
    compare_result("(bc|de|[23]*)", "bc", "", type);
    compare_result("(bc|de|[23]*)", "bcbc", "", type);
    compare_result("(bc|de|[23]*)", "bc23", "", type);
    compare_result("a(bc|de|[23]*)", "a", "", type);
    compare_result("a(bc|de|[23]+)", "a", "", type);
    compare_result("a(bc|de|[23]+)", "a2332", "", type);
    compare_result("a(bc|de|[23]?)", "a23", "", type);
}

void test_plus(MatchType type) {
    compare_result("a+bc", "aaaabc", "", type);	
    compare_result("a+bc", "bc", "", type);
    compare_result("a+b+c", "aabbbc", "", type);
    compare_result("[abc]+d", "abcd", "", type);
    compare_result("[abc]+d", "bbbd", "", type);
    compare_result("[abc]+d", "d", "", type);
    compare_result("[abc]+c", "c", "", type);
    compare_result("[abc]+c", "cc", "", type);
}

void test_question(MatchType type) {
    compare_result("a?bc", "abc", "", type);    
    compare_result("a?bc", "bc", "", type);
    compare_result("a?bc", "aaaabc", "", type);
    compare_result("a?b?c?", "ac", "", type);
    compare_result("[abc]?de", "abcde", "", type);
    compare_result("[abc]?de", "cde", "", type);
    compare_result("[abc]?de", "de", "", type);
    compare_result("[abc]?de", "abcabcde", "", type);
    compare_result("(abc)*(ab)?", "abcabc", "", type);
    compare_result("(abc)*(ab)?", "ab", "", type);
}

void test_repetition(MatchType type) {
    compare_result("a{5}", "aaaaa", "", type);    
    compare_result("a{5}", "aaaaaa", "", type);    
    compare_result("a{5}", "aaaa", "", type);  
    compare_result("ab{3}", "abbb", "", type);  
    compare_result("ab{3}", "ababab", "", type);  
    compare_result("ab{3}c", "abbbc", "", type);  
    compare_result("ab{3}c", "abbc", "", type);  
    compare_result("ab{3}c", "abbbbc", "", type);  
    compare_result("ab{10}c", "abbbbbbbbbbc", "", type);
    compare_result("4{2,4}", "44", "", type);
    compare_result("4{2,4}", "444", "", type);
    compare_result("4{2,4}", "4444", "", type);
    compare_result("4{2,4}", "44444", "", type);
    compare_result("4{2,4}", "4", "", type);
    compare_result("a{1,4}b{2}", "aabb", "", type);
    compare_result("(a{1,4}b{2}){2}", "aabbaaab", "", type);
    compare_result("(a{1,4}b{2}){2}", "aabbaaabb", "", type);
    compare_result("ba{0,2}", "b", "", type);
    compare_result("ba{0,2}", "ba", "", type);
    compare_result("ba{0,2}", "baa", "", type);
    compare_result("ba{0,2}", "baaaa", "", type);
    compare_result("ba{0}", "b", "", type);
    compare_result("c(a|b){0,2}", "cba", "", type);
    compare_result("c(a|b){0,2}", "c", "", type);
    compare_result("c[a-d]{0,2}", "cad", "", type);
    compare_result("c[a-d]{0,2}", "c", "", type);
}

void test_combined(MatchType type) {
    compare_result("(45|ab?)", "ab", "", type);    
    compare_result("(ab?|45)", "ab", "", type);    
    compare_result("(ab?|45)", "ab45", "", type);    
    compare_result("(ab?|45)", "a", "", type);
    compare_result("(ab){4}", "abababab", "", type);
    compare_result("[bde]{2}", "bd", "", type);
    compare_result("((abcd){1}45{3}){2}", "abcd4555abcd4555", "", type);
    compare_result("([abc]3){2}", "a3a3", "", type);
    compare_result("([abc]3){2}", "c3b3", "", type);
    compare_result("([abc]3){2}", "a3c3c3", "", type);
    compare_result("([abc]3){2}", "ac", "", type);
    compare_result("[a-q][^u-z]{3}x", "q444x", "", type);
    compare_result("(ab|(cd|ef){2}|4)", "cdcd", "", type);
    compare_result("(ab|(cd|ef){2}|4)", "cdef", "", type);
    compare_result("(ab|(cd|ef){2}|4)", "ab", "", type);
    compare_result("(ab|(cd|ef){2}|4)", "4", "", type);
}

void test_escaping(MatchType type) {
    compare_result("a\\d", "a0", "", type);
    compare_result("a\\d", "ad", "", type);
    compare_result("a\\d", "a0d", "", type);
    compare_result("a\\db", "a9b", "", type);
    compare_result("a\\d{3}", "a912", "", type);
    compare_result("bcd\\d*", "bcd", "", type);
    compare_result("bcd\\d*", "bcd23", "", type);
    compare_result("a[\\dbc]d", "a5d", "", type);
    compare_result("a[\\dbc]d", "acd", "", type);

    compare_result("a\\w\\d", "ab5", "", type);
    compare_result("a\\w\\W", "a_0", "", type);
    compare_result("a\\w\\d", "aC9", "", type);

    compare_result("\\ss", " s", "", type);
    compare_result("\\Da\\Wbc\\S", "aa3bc_", "", type);
    compare_result("\\Dabc\\S", "aabc_", "", type);
    compare_result("\\da\\wbc\\s", "7a_bc ", "", type);
    compare_result("ab[\\Dcd]", "abc", "", type);
}

void test_ignore_case(MatchType type) {
    compare_result("abcd", "AbCd", "", type, "i");
    compare_result("a1a*", "a1aaAaA", "", type, "i");
    compare_result("b2[a-g]", "B2D", "", type, "i");
    compare_result("[^a-z]", "D", "", type, "i");
    compare_result("[^a-z][a-z]*", "4Df", "", type, "i");
    compare_result("(abc){3}", "aBcAbCabc", "", type, "i");
    compare_result("(1|2|k)*", "K2", "", type, "i");
    compare_result("z+a", "zZa", "", type, "i");
    compare_result("z?a", "Za", "", type, "i");
    compare_result("\\d\\w\\s", "5a ", "", type, "i");
    compare_result("\\D\\W\\S", "a k", "", type, "i");
}

void test_expand_regex() {
    string res = get<0>(expand_regex(string("abc")));
    cout << res << " " << res.compare(string("abc")) << endl;
    string res1 = get<0>(expand_regex(string("a{5}")));
    cout << res1 << " " << res1.compare(string("aaaaa")) << endl;
    string res2 = get<0>(expand_regex(string("(abc){5}")));
    cout << res2 << " " << res2.compare(string("(abc)(abc)(abc)(abc)(abc)")) << endl;
    string res3 = get<0>(expand_regex(string("((abc){3}4){2}")));
    cout << res3 << " " << res3.compare(string("((abc)(abc)(abc)4)((abc)(abc)(abc)4)")) << endl;
    string res4 = get<0>(expand_regex(string("((3|4){1}[bc]{5}){2}")));
    cout << res4 << " " << res4.compare("((3|4)[bc][bc][bc][bc][bc])((3|4)[bc][bc][bc][bc][bc])") << endl;
    string res5 = get<0>(expand_regex(string("a(bc)*")));
    cout << res5 << " " << res5.compare(string("a(bc)*")) << endl;
    string res6 = get<0>(expand_regex(string("a(bc){2,5}")));
    cout << res6 << " " << res6.compare(string("a(bc)(bc)(bc)?(bc)?(bc)?")) << endl;
    string res7 = get<0>(expand_regex(string("(ab|(cd|ef){2}|4)")));
    cout << res7 << " " << res7.compare("(ab|(cd|ef)(cd|ef)|4)") << endl;
}

int main() {
    auto start = high_resolution_clock::now();
    MatchType type = MatchType::FULL;
    test_simple(type);
    test_star(type);
    test_brackets(type);
    test_negative_brackets(type);
    test_brackets_and_star(type);
    test_grouping(type);
    test_nested_grouping(type);
    test_or_groups(type);
    test_plus(type);
    test_question(type);
    test_repetition(type);
    test_combined(type);
    test_escaping(type);
    test_ignore_case(type);
    auto end = high_resolution_clock::now();
    auto dur = (duration_cast<seconds>(end - start)).count();
    cout << "time: " << dur << "s" << endl;
}


