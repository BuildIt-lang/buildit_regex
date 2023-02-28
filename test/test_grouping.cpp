#include "test_grouping.h"

void test_group_states(string re, vector<int> expected) {
    int re_len = re.length();
    int* groups = new int[re_len];
    group_states(re, groups);
    // check if indices match
    for (int i = 0; i < re_len; i++) {
        assert(groups[i] == expected[i]);    
    }
    delete[] groups;
    cout << re << ": passed" << endl;
}

void group_states_tests() {
    // group at start
    string re = "(?Gabc)de";
    vector<int> expected = {0, 1, 2, 3, 3, 3, 6, 7, 8};
    test_group_states(re, expected);
    
    // nested parentheses
    re = "ab(?Gde(abc|de))fg";
    expected = {0, 1, 2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 15, 16, 17};
    test_group_states(re, expected);

    re = "ab(de(?Gabc|de))fg";
    expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 8, 14, 15, 16, 17};
    test_group_states(re, expected);
    
    // multiple groups / group at end
    re = "ab(?Gde)(?Gfg)";
    expected = {0, 1, 2, 3, 4, 5, 5, 7, 8, 9, 10, 11, 11, 13};
    test_group_states(re, expected);

    // group in middle
    re = "ab(?Ghi)cd";
    expected = {0, 1, 2, 3, 4, 5, 5, 7, 8, 9};
    test_group_states(re, expected);

    // length 1 regex
    re = "b";
    expected = {0};
    test_group_states(re, expected);

    // the entire regex is a group
    re = "(?Gabcde)";
    expected = {0, 1, 2, 3, 3, 3, 3, 3, 8};
    test_group_states(re, expected);
}
string remove_special_chars(string regex, char special) {
    string chars = "(?G";
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
void compare_result(const char* pattern, const char* candidate, MatchType match_type, const char* flags) {
    string simple_regex = remove_special_chars(pattern, 'G');
    bool expected = 0;
    if (match_type == MatchType::FULL) {
    	expected = (strcmp(flags, "i") == 0) ? 
            regex_match(candidate, regex(simple_regex, regex_constants::icase)) :
            regex_match(candidate, regex(simple_regex));
    } else if (match_type == MatchType::PARTIAL_SINGLE) {
        expected = (strcmp(flags, "i") == 0) ? 
            regex_search(candidate, regex(simple_regex, regex_constants::icase)) :
            regex_search(candidate, regex(simple_regex));
    }
    int result = compile_and_run_groups(candidate, pattern, match_type, 1, flags);
    cout << "Matching " << pattern << " with " << candidate << " -> ";
    bool match = (result == expected);
    if (match) {
        cout << "ok. Result is: " << result << endl;
    } else {
        cout << "failed\nExpected: " << expected << ", got: " << result << endl;
    }
}

void test_simple(MatchType type) {
    compare_result("abcdef", "abcdef", type);
    compare_result("a(?Gbcd)ef", "abcdef", type);
    compare_result("(?Gab)cdef", "abcdef", type);
    compare_result("abc(?Gdef)", "abcdef", type);
    compare_result("a(?Gbcd)ef", "11abcdef111", type);
    compare_result("(?Gab)cdef", "xyzabcdefgh", type);
    compare_result("abc(?Gdef)", "xyzabcdefghi", type);
}

void test_star(MatchType type) {
    compare_result("(?Ga*)bc", "aaaabc", type);
    compare_result("(?Ga*b)c", "aaaabc", type);
    compare_result("(?Ga*)bc", "dddaaaabc", type);
    compare_result("(?Ga*b)c", "ccccaaaabcd", type);
}

void test_brackets(MatchType type) {
    compare_result("a(?G[abc]d)", "abd", type);
    compare_result("(?G[a-d])ef", "bef", type);
    compare_result("(?G[^a-d])ef", "bef", type);
    compare_result("(?G[^a-d])ef", "kef", type);
    compare_result("a(?G[abc]d)", "555abd1", type);
    compare_result("(?G[a-d])ef", "44bef3", type);
    compare_result("(?G[^a-d])ef", "33bef65", type);
    compare_result("(?G[^a-d])ef", "421kef43", type);
}

void test_repetition(MatchType type) {
    compare_result("(?Gabaaacd)e", "abaaacde", type);
    compare_result("(?Gaba{3}cd)e", "abaaacde", type);
    compare_result("(?Gaba{2,4}cd)e", "abaaacde", type);
    compare_result("(?Gaba{2,4})cde", "abaaacde", type);
    compare_result("ab(?Ga{2,4})cde", "abaaacde", type);
    compare_result("ab(?Ga{2,4}c)de", "abaaacde", type);
    compare_result("ab(?Ga{2,4}cde)", "abaaacde", type);
    compare_result("(?Gaba{2,4}cd)e", "dsafdabaaacde", type);
    compare_result("(?Gab.{2,4}cd)e", "abaaacde", type);
    compare_result("(?Gab.{2,4}cd)e", "abaaacdefdsa", type);
    compare_result("(?Gaba{2,4}cd)e", "fdasfabaaacdegf", type);
    compare_result("(?Gaba{2,4})cde", "asfdsabaaacdegf", type);
    compare_result("ab(?Ga{2,4})cde", "fasdabaaacdefds", type);
    compare_result("ab(?Ga{2,4}c)de", "asdabaaacdefd", type);
    compare_result("ab(?Ga{2,4}cde)", "abaaacde", type);
}

void test_or_groups(MatchType type) {
    compare_result("(?Gab|cd)ef", "abef", type);
    compare_result("(?Gab|cd)ef", "cdef", type);
    compare_result("(?Gab|cd)ef", "ef", type);
    compare_result("111(?Gab|cd)ef", "111abef", type);
    compare_result("11(?G1(ab|cd))ef", "111abef", type);
    compare_result("11(?G1(ab|cd))ef", "aa111abef", type);
    compare_result("(?Gab|cd)ef", "aaaabefaa", type);
    compare_result("(?Gab|cd)ef", "aacdef55", type);
    compare_result("(?Gab|cd)ef", "555efa", type);
    compare_result("111(?Gab|cd)ef", "111abefaa", type);
}

void test_combined(MatchType type) {
    // brackets and star
    compare_result("(?G[bcd]*)", "bbcdd", type);    
    compare_result("aa(?G5[bcd]*)", "aa5bbcdd", type);    
    compare_result("(?G[bcd]5*)", "d555", type);    
    compare_result("(?G[bcd]5*)", "bbcdd555", type);    
    compare_result("(?G[bcd]*)", "aaaabbcdd", type);    
    compare_result("aa(?G5[bcd]*)", "113aa5bbcdd11", type);    
    compare_result("a(?G(abcd)*)", "aabcdabcd", type);
    compare_result("(?G(abcd)*)", "abcdabcd", type);
    compare_result("(?G12(abcd)*)", "12abcdabcd", type);
    compare_result("(?G12(abcd)*)", "55512abcdabcd", type);
    compare_result("(?Gbdc)*", "bdcbdc", type);
    // ? and +
    compare_result("ab(?Gcd)?", "abcd", type); 
    compare_result("ab(?Gcd)?", "ab", type); 
    compare_result("a(?Gb(cd)?)", "abcd", type); 
    compare_result("a(?Gb(cd)?)", "ab", type); 
    compare_result("ab(?Gcd)+", "abcdcd", type); 
    compare_result("ab(?Gcd)+", "abcd", type); 
    compare_result("ab(?Gcd)?", "iafdsabcd", type); 
    compare_result("ab(?Gcd)?", "111abppp", type); 
    compare_result("a(?Gb(cd)?)", "123abcd123", type); 
    compare_result("a(?Gb(cd)?)", "a22ab22", type); 
    compare_result("ab(?Gcd)+", "21abcdcd21", type); 
    compare_result("ab(?Gcd)+", "333abcd", type); 
}

int main() {
    //compare_result("(?GTom.{10,15}r)iver", "dsafdasfdTomswimminginrivercdsvadsfd", MatchType::PARTIAL_SINGLE);
    cout << "--- FULL MATCHES ---" << endl;
    test_simple(MatchType::FULL);
    test_star(MatchType::FULL);
    test_brackets(MatchType::FULL);
    test_or_groups(MatchType::FULL);
    test_repetition(MatchType::FULL);
    test_combined(MatchType::FULL);

    cout << "--- PARTIAL MATCHES ---" << endl;
    test_simple(MatchType::PARTIAL_SINGLE);
    test_star(MatchType::PARTIAL_SINGLE);
    test_brackets(MatchType::PARTIAL_SINGLE);
    test_or_groups(MatchType::PARTIAL_SINGLE);    
    test_repetition(MatchType::PARTIAL_SINGLE);
    test_combined(MatchType::PARTIAL_SINGLE);
}
