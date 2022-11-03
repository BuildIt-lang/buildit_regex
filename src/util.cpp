

#include "util.h"


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

tuple<string, int> expand_sub_regex(string re, int start) {
    if (start < 0) {
        return tuple<string, int>{"", -1};
    }
    if (re[start] == ')' || re[start] == ']') {
        char end = (re[start] == ')') ? '(' : '[';
        string s = "";
        int idx = start - 1;
        while (re[idx] != end) {
            tuple<string, int> sub_s = expand_sub_regex(re, idx);
            s = get<0>(sub_s) + s;
            idx = get<1>(sub_s);
        }
        return tuple<string, int>{end + s + re[start], idx - 1};
    } else if (re[start] == '}') {
        int counters[2] = {0, 0};
        int idx = get_counters(re, start - 1, counters);
        tuple<string, int> sub_s = expand_sub_regex(re, idx);
        string s = "";
        for (int i = 0; i < counters[0]; i++) {
            s += get<0>(sub_s);
            counters[1] -= 1;
        }
        for (int i = 0; i < counters[1]; i++) {
            s += get<0>(sub_s) + "?";        
        }
        return tuple<string, int>{s, get<1>(sub_s)};
    } else {
        string s = ""; 
        return tuple<string, int>{s + re[start], start - 1};    
    }

}

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
