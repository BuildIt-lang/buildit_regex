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
}

void test_star(MatchType type) {
    compare_result("(?Ga*)bc", "aaaabc", type);
    compare_result("(?Ga*b)c", "aaaabc", type);
}

void test_brackets(MatchType type) {
    compare_result("a(?G[abc]d)", "abd", type);
    compare_result("(?G[a-d])ef", "bef", type);
    compare_result("(?G[^a-d])ef", "bef", type);
    compare_result("(?G[^a-d])ef", "kef", type);
}

int main() {
    test_simple(MatchType::FULL);
    test_star(MatchType::FULL);
    test_brackets(MatchType::FULL);

    test_simple(MatchType::PARTIAL_SINGLE);
    test_star(MatchType::PARTIAL_SINGLE);
    test_brackets(MatchType::PARTIAL_SINGLE);
}
