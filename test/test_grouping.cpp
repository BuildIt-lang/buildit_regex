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

void test_group_states_1() {
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

int main() {
    test_group_states_1();    
}
