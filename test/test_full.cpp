#include <iostream>
#include "test.h"
#include "../include/util.h"

using namespace std::chrono;

/**
General function to compare results.
*/
void check_correctness(const char* pattern, const char* candidate) {
    bool expected = regex_match(candidate, regex(pattern));
    int len = strlen(candidate);
    
    string processed_re = expand_regex(pattern);
    
    const int re_len = processed_re.length();
    std::unique_ptr<char> cache_ptr(new char[re_len + 1]);
    
    const int cache_size = (re_len + 1) * (re_len + 1);

    std::unique_ptr<int> cache_states_ptr(new int[cache_size]);
    char* cache = cache_ptr.get();
    for (int i = 0; i < re_len + 1; i++) cache[i] = 0;
    int* cache_states = cache_states_ptr.get();
    std::unique_ptr<int> next_state_ptr(new int[re_len]);
    int *next_state = next_state_ptr.get();
    std::unique_ptr<int> brackets_ptr(new int[re_len]);
    int *brackets = brackets_ptr.get(); // hold the opening and closing indices for each bracket pair
    std::unique_ptr<int> helper_states_ptr(new int[re_len]);
    int *helper_states = helper_states_ptr.get();

	builder::builder_context context;
	context.dynamic_use_cxx = true;
    context.dynamic_header_includes = "#include <set>\n#include <map>\n#include \"../../include/runtime.h\"";
	context.feature_unstructured = true;
	context.run_rce = true;
    auto fptr = (int (*)(const char*, int))builder::compile_function_with_context(context, match_regex_full, processed_re.c_str(), true, cache, cache_states, next_state, brackets, helper_states);
    int result = fptr((char*)candidate, len);
    cout << "Matching " << pattern << " with " << candidate << " -> ";
    bool match = (result == expected);
    if (match) {
        cout << "ok. Result is: " << result << endl;
    } else {
        cout << "failed\nExpected: " << expected << ", got: " << result << endl;
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
    check_correctness("ba{0,2}", "b");
    check_correctness("ba{0,2}", "ba");
    check_correctness("ba{0,2}", "baa");
    check_correctness("ba{0,2}", "baaaa");
    check_correctness("ba{0}", "b");
    check_correctness("c(a|b){0,2}", "cba");
    check_correctness("c(a|b){0,2}", "c");
    check_correctness("c[a-d]{0,2}", "cad");
    check_correctness("c[a-d]{0,2}", "c");
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
    check_correctness("[a-q][^u-z]{3}x", "q444x");
    check_correctness("(ab|(cd|ef){2}|4)", "cdcd");
    check_correctness("(ab|(cd|ef){2}|4)", "cdef");
    check_correctness("(ab|(cd|ef){2}|4)", "ab");
    check_correctness("(ab|(cd|ef){2}|4)", "4");
}

void test_expand_regex() {
    string res = expand_regex(string("abc"));
    cout << res << " " << res.compare(string("abc")) << endl;
    string res1 = expand_regex(string("a{5}"));
    cout << res1 << " " << res1.compare(string("aaaaa")) << endl;
    string res2 = expand_regex(string("(abc){5}"));
    cout << res2 << " " << res2.compare(string("(abc)(abc)(abc)(abc)(abc)")) << endl;
    string res3 = expand_regex(string("((abc){3}4){2}"));
    cout << res3 << " " << res3.compare(string("((abc)(abc)(abc)4)((abc)(abc)(abc)4)")) << endl;
    string res4 = expand_regex(string("((3|4){1}[bc]{5}){2}"));
    cout << res4 << " " << res4.compare("((3|4)[bc][bc][bc][bc][bc])((3|4)[bc][bc][bc][bc][bc])") << endl;
    string res5 = expand_regex(string("a(bc)*"));
    cout << res5 << " " << res5.compare(string("a(bc)*")) << endl;
    string res6 = expand_regex(string("a(bc){2,5}"));
    cout << res6 << " " << res6.compare(string("a(bc)(bc)(bc)?(bc)?(bc)?")) << endl;
    string res7 = expand_regex(string("(ab|(cd|ef){2}|4)"));
    cout << res7 << " " << res7.compare("(ab|(cd|ef)(cd|ef)|4)") << endl;
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
    auto end = high_resolution_clock::now();
    auto dur = (duration_cast<seconds>(end - start)).count();
    cout << "time: " << dur << "s" << endl;
}


