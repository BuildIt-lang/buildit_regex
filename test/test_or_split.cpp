#include <iostream>
#include "test.h"
#include "../include/parse.h"
#include "../include/progress.h"
#include "../include/frontend.h"

using namespace std::chrono;

string remove_special_chars(string regex, char special) {
    string chars = "(?" + special;
    int chars_len = chars.length();
    string to_replace = "(";
    string result = regex;
    while (true) {
        int idx = result.find(chars);
        if (idx == (int)string::npos)
            return result;
        result.replace(idx, chars_len, to_replace);
    }
    return result;
}
/**
General function to compare results.
*/
void check_split(const char* pattern, const char* candidate, int start_state, const char* flags) {
    
    string simple_pattern = remove_special_chars(pattern, 'S');
    bool expected = (strcmp(flags, "i") == 0) ? 
        regex_search(candidate, regex(simple_pattern, regex_constants::icase)) :
        regex_search(candidate, regex(simple_pattern));
    
    int result = compile_and_run_split(candidate, pattern, start_state, MatchType::PARTIAL_SINGLE, flags);
    std::cout << "Matching " << pattern << " with " << candidate << " -> ";
    bool match = (result == expected);
    if (match) {
        std::cout << "ok. Result is: " << result << std::endl;
    } else {
        std::cout << "failed\nExpected: " << expected << ", got: " << result << std::endl;
    }
}

void test_simple() {
    check_split("aaaa", "aaaa");
    check_split("", "");
    check_split("abc", "abc");
    check_split("aabc", "aabc");
    check_split("abcd", "bcd", 1);
    check_split("abcd", "abcd", 1);
}

void test_star() {
    check_split("a*bc", "abc");    
    check_split("a*bc", "aaaabc");
    check_split("a*b*c", "aaabbc");

}

void test_brackets() {
    check_split("a[bc]d", "abd");
    check_split("a[bc]d", "acd");
    check_split("a[bc]d", "abcd");
    check_split("a[]d", "ad");
    check_split("[amn]", "m");
    check_split("[amn]", "mn");
    check_split("a[bc]d", "abcd");
    check_split("a[vd][45]", "ad4");
    check_split("2[a-g]", "2d");
    check_split("2[a-g]*", "2dcag");
}

void test_negative_brackets() {
    check_split("3[^db]4", "3c4");    
    check_split("3[^db]4", "3d4");    
    check_split("3[^db]4", "3b4"); 
    check_split("[abd][^35]*", "a4555dsd");
    check_split("[abd]*[^35fds]", "abdd4");
    check_split("2[^a-g]", "25");
    check_split("2[^a-g]", "2h");
    check_split("2[^a-g]", "2a");
}

void test_brackets_and_star() {
    check_split("a[bc]*d", "abcd");
    check_split("a[bc]*d", "abbd");
    check_split("a[bc]*d", "ad");
    check_split("a[bc]*d", "abd");
    check_split("a[bc]*d", "abcdd");
    check_split("a[bc]*c", "ac");
    check_split("3[^db]*4", "3ca8a4");    
    check_split("3[^db]*", "3");    
}

void test_grouping() {
    check_split("a(bcd)*", "abcdbcd");
    check_split("a(bcd)", "abcdbcd");
    check_split("a(bcd)", "abcd");
    check_split("a(bcd)*", "a");
    check_split("a(bcd)*tr", "abcdtr");
    check_split("a(bcd)*(tr432)*", "abcdtr432tr432");
    check_split("a(bcd)*(tr432)*", "abctr432tr432");
    check_split("a[bcd]*(tr432)*", "abtr432tr432");
    check_split("(4324)*(abc)", "432443244324abc");
}

void test_nested_grouping() {
    check_split("a((bcd)*34)*", "abcdbcd34bcd34");
    check_split("a(a(bc|45))*", "aacaca4a5");
    check_split("a(a(bc|45))*", "aabcabca45");
    check_split("(a(bc|45)c)?d", "a45cd");
    check_split("(a(bc|45)c)?d", "d");
}

void test_or_groups() {
    check_split("ab(c|56|de)", "ab56");
    check_split("ab(c|56|de)k", "abck");
    check_split("ab(c|56|de)k", "ab56k");
    check_split("ab(c|56|de)k", "abdek");
    check_split("ab(c|56|de)k", "ab56dek");
    check_split("ab((c|56|de)k|ab)", "ab56abab");
    check_split("a(cbd|45)*", "acbd45cbd");
    check_split("([abc]|dc|4)2", "b2");
    check_split("([abc]|dc|4)2", "b2");
    check_split("(dc|[abc]|4)2", "b2");
    check_split("(dc|4|[abc])2", "b2");
    check_split("([^23]|2)abc", "1abc");
    check_split("([^23]|2)abc", "3abc");
    check_split("(a|[abc]|4)2", "ac42");
    check_split("(b|a*|d)aa", "aaaa");
    check_split("(bc|de|[23]*)", "22323");
    check_split("(bc|de|[23]*)", "bc");
    check_split("(bc|de|[23]*)", "bcbc");
    check_split("(bc|de|[23]*)", "bc23");
    check_split("a(bc|de|[23]*)", "a");
    check_split("a(bc|de|[23]+)", "a");
    check_split("a(bc|de|[23]+)", "a2332");
    check_split("a(bc|de|[23]?)", "a23");
    check_split("(ab|(cd|ef)|dd)", "efff");
    check_split("(ab|cd|ef)11(f|d|b)", "aaaa11fg");
}
void test_split_or_groups() {
    check_split("ab(?Sc|56|de)", "ab56");
    check_split("ab(?Sc|56|de)k", "abck");
    check_split("ab(?Sc|56|de)k", "ab56k");
    check_split("ab(?Sc|56|de)k", "abdek");
    check_split("ab(?Sc|56|de)k", "ab56dek");
    check_split("ab(?S(?Sc|56|de)k|ab)", "ab56abab");
    check_split("a(?Scbd|45)*", "acbd45cbd");
    check_split("(?S[abc]|dc|4)2", "b2");
    check_split("(?S[abc]|dc|4)2", "b2");
    check_split("(?Sdc|[abc]|4)2", "b2");
    check_split("(?Sdc|4|[abc])2", "b2");
    check_split("(?S[^23]|2)abc", "1abc");
    check_split("(?S[^23]|2)abc", "3abc");
    check_split("(?Sa|[abc]|4)2", "ac42");
    check_split("(?Sb|a*|d)aa", "aaaa");
    check_split("(?Sbc|de|[23]*)", "22323");
    check_split("(?Sbc|de|[23]*)", "bc");
    check_split("(?Sbc|de|[23]*)", "bcbc");
    check_split("(?Sbc|de|[23]*)", "bc23");
    check_split("a(?Sbc|de|[23]*)", "a");
    check_split("a(?Sbc|de|[23]+)", "a");
    check_split("a(?Sbc|de|[23]+)", "a2332");
    check_split("a(?Sbc|de|[23]?)", "a23");
    check_split("(ab|(?Scd|ef)|dd)", "efff");
    check_split("(?Sab|cd|ef)11(f|d|b)", "aaaa11fg");
    check_split("(ab|cd|ef)11(?Sf|d|b)", "aaaa11fg");
    check_split("(?Sab|cd|ef)11(?Sf|d|b)", "aaaa11fg");
}


void test_plus() {
    check_split("a+bc", "aaaabc");	
    check_split("a+bc", "bc");
    check_split("a+b+c", "aabbbc");
    check_split("[abc]+d", "abcd");
    check_split("[abc]+d", "bbbd");
    check_split("[abc]+d", "d");
    check_split("[abc]+c", "c");
    check_split("[abc]+c", "cc");
}

void test_question() {
    check_split("a?bc", "abc");    
    check_split("a?bc", "bc");
    check_split("a?bc", "aaaabc");
    check_split("a?b?c?", "ac");
    check_split("[abc]?de", "abcde");
    check_split("[abc]?de", "cde");
    check_split("[abc]?de", "de");
    check_split("[abc]?de", "abcabcde");
    check_split("(abc)*(ab)?", "abcabc");
    check_split("(abc)*(ab)?", "ab");
}

void test_repetition() {
    check_split("a{5}", "aaaaa");    
    check_split("a{5}", "aaaaaa");    
    check_split("a{5}", "aaaa");  
    check_split("ab{3}", "abbb");  
    check_split("ab{3}", "ababab");  
    check_split("ab{3}c", "abbbc");  
    check_split("ab{3}c", "abbc");  
    check_split("ab{3}c", "abbbbc");  
    check_split("ab{10}c", "abbbbbbbbbbc");
    check_split("4{2,4}", "44");
    check_split("4{2,4}", "444");
    check_split("4{2,4}", "4444");
    check_split("4{2,4}", "44444");
    check_split("4{2,4}", "4");
    check_split("a{1,4}b{2}", "aabb");
    check_split("(a{1,4}b{2}){2}", "aabbaaab");
    check_split("(a{1,4}b{2}){2}", "aabbaaabb");
}

void test_combined() {
    check_split("(45|ab?)", "ab");    
    check_split("(ab?|45)", "ab");    
    check_split("(ab?|45)", "ab45");    
    check_split("(ab?|45)", "a");
    check_split("(ab){4}", "abababab");
    check_split("[bde]{2}", "bd");
    check_split("((abcd){1}45{3}){2}", "abcd4555abcd4555");
    check_split("([abc]3){2}", "a3a3");
    check_split("([abc]3){2}", "c3b3");
    check_split("([abc]3){2}", "a3c3c3");
    check_split("([abc]3){2}", "ac");
}

void test_escaping() {
    check_split("a\\d", "texta0text");
    check_split("a\\d", "dsaad5f");
    check_split("a\\db", "a9b");
    check_split("a\\d{3}", "a912aaa");
    check_split("bcd\\d*", "bcda");
    check_split("bcd\\d*", "bcd567a");
    check_split("bcd\\d*", "bcd23");
    check_split("a[\\dbc]d", "3a5da");
    check_split("a[\\dbc]d", "acd");

    check_split("a\\w\\d", "aab5d");
    check_split("a\\w\\W", "fa_09");
    check_split("a\\w\\d", "aC9");

    check_split("\\ss", "sss s");
    check_split("\\Da\\Wbc\\S", "aa3bc_");
    check_split("\\Dabc\\S", "aabc_00");
    check_split("\\da\\wbc\\s", "a7a_bc cc");
}

void test_partial() {
	check_split("ab", "aab");
	check_split("ab", "aba");
	check_split("a?", "aaaa");
	check_split("c[ab]+", "abc");
	check_split("c[ab]+", "aaba");
	check_split("c[ab]+", "caaaabcc");
	check_split("123", "a123a");
	check_split("(123)*1", "112312311");
    check_split("Twain", "MarkTwainTomSawyer");
	//check_split("[a-q][^u-z]{5}x", "qax");
	//check_split("[a-q][^u-z]{5}x", "qabcdex");
}
void test_ignore_case() {
    check_split("abcd", "AbCd", 0, "i");
    check_split("a1a*", "a1aaAaA", 0, "i");
    check_split("b2[a-g]", "B2D", 0, "i");
    check_split("[^a-z]", "D", 0, "i");
    check_split("[^a-z][a-z]*", "4Df", 0, "i");
    check_split("(abc){3}", "aBcAbCabc", 0, "i");
    check_split("(1|2|k)*", "K2", 0, "i");
    check_split("z+a", "zZa", 0, "i");
    check_split("z?a", "Za", 0, "i");
    check_split("\\d\\w\\s", "5a ", 0, "i");
    check_split("\\D\\W\\S", "a k", 0, "i");
}

int main() {    
    check_split("(?STom.{10,15}river|river.{10,15}Tom)", "dsafdasfdTomswimminginrivercdsvadsfd");
    check_split("(?STom.{10,15}river|river.{10,15}Tom)", "dsafdasfdTomswimmingrivercdsvadsfd");

    auto start = high_resolution_clock::now();
    test_simple();
    test_star();
    test_brackets();
    test_negative_brackets();
    test_brackets_and_star();
    test_grouping();
    test_nested_grouping();
    test_or_groups();
    test_split_or_groups();
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
    //check_split("(?Sabc|de)");
}


