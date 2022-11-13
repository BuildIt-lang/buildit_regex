#include "parse.h"

/**
Extracts the counters from bounded repetition
into the `counters` array. e.g. "{2,5}" results in
an array [2,5].
We process `re` from right to left starting from `idx`.
*/
int get_counters(string re, int idx, int *counters) {
    int result = 0;
    int factor = 1;
    while (re[idx] != '{') {
        if (re[idx] == ',') {
            counters[1] = result;
            result = 0;
            factor = 1;
            idx = idx - 1;
        } else {
            int digit = re[idx] - '0';
            result = result + digit * factor;
            factor = factor * 10;
            idx = idx - 1;
        }
    }
    counters[0] = result;
    return idx - 1;
}

/**
Based on the value of `re[start]` recursively parses the
corresponding group and returns a tuple of the parsed group
and the index at which the group starts in `re`. It parses
`re` from right to left.

re[start] = ) or ] => it returns a substring corresponding
to the () or [] group, and the index just before ( or [.

re[start] = } => extracts the counters from {} and repeats the
group before {} as controlled by the counters. 
e.g. "a{3}" becomes "aaa"; "a{2,4}" becomes "aaa?a?"

re[start] = + => "a+" becomes "aa*"

re[start] = any other char => returns that char and start - 1
*/
tuple<string, int> expand_sub_regex(string re, int start) {
    if (start < 0) {
        // base case: we've processed the entire regex
        return tuple<string, int>{"", -1};
    }
    if (re[start] == ')' || re[start] == ']') {
        char end = (re[start] == ')') ? '(' : '[';
        string s = "";
        int idx = start - 1;
        // repeatedly parse the expression until we
        // reach the opening bracket
        while (re[idx] != end) {
            tuple<string, int> sub_s = expand_sub_regex(re, idx);
            s = get<0>(sub_s) + s;
            idx = get<1>(sub_s);
        }
        return tuple<string, int>{end + s + re[start], idx - 1};
    } else if (re[start] == '}') {
        int counters[2] = {0, 0};
        // extract the counters
        int idx = get_counters(re, start - 1, counters);
        // parse the group that needs to be repeated
        tuple<string, int> sub_s = expand_sub_regex(re, idx);
        // replicate the group
        string s = "";
        for (int i = 0; i < counters[0]; i++) {
            s += get<0>(sub_s);
            counters[1] -= 1;
        }
        for (int i = 0; i < counters[1]; i++) {
            s += get<0>(sub_s) + "?";        
        }
        return tuple<string, int>{s, get<1>(sub_s)};
    } else if (re[start] == '+') {
        tuple<string, int> sub_s = expand_sub_regex(re, start-1);
        string s = get<0>(sub_s);
        return tuple<string, int>{s + s + "*", get<1>(sub_s)};
    } else {
        // normal chracter
        string s = ""; 
        return tuple<string, int>{s + re[start], start - 1};    
    }

}

/**
Combines the results from each group found by
`expand_sub_regex` into a single pattern.
*/
string expand_regex(string re) {
    int start = re.length() - 1;
    string s = "";
    while (start >= 0) {
        tuple<string, int> sub_s = expand_sub_regex(re, start);
        s = get<0>(sub_s) + s;
        start = get<1>(sub_s);
    }
    return s;

}

