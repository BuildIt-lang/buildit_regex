#include "util.h"

tuple<string, int> expand_regex(string re, int start) {
    tuple<string, int> result(string(""), -1);
    int idx = start;
    if (idx < 0) return result;

    string expanded_re("");
    while (idx >= 0 && re[idx] != '(') {
        if (re[idx] == '}') {
            int idx_st = idx;
            // get the # of repetitions
            int factor = 1;
            int reps = 0;
            idx = idx - 1;
            while (re[idx] != '{') {
                int digit = re[idx] - '0';
                reps = reps + digit * factor;
                factor = factor * 10;
                idx = idx - 1;
            }
            cout << reps << endl;
            idx = idx - 1;
            if (re[idx] == ')') {
                tuple<string, int> mid = expand_regex(re, idx - 1);
                idx = get<1>(mid);
                for (int i = 0; i < reps; i = i + 1) {
                    expanded_re = string("(") + get<0>(mid) + string(")") + expanded_re;
                }
            } else {
                idx = idx_st;    
                expanded_re = re[idx] + expanded_re;
                idx = idx - 1;
            }
        } else {
            expanded_re = re[idx] + expanded_re;
            idx = idx - 1;
        }
    }
    return tuple<string, int> {expanded_re, idx-1};
}
