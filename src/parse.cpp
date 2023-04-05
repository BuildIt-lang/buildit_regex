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

// special characters from the user's perspective
bool is_special_external(char c) {
    string specials = "[](){}|.*+?^\\";
    return specials.find(c) != std::string::npos;
}

// special characters in our internal representation
// e.g. {} and + are not internally special
// ^ is special only when inside []
bool is_special_internal(char c) {
    string specials = "[]()|.*?^\\";
    return specials.find(c) != std::string::npos;
}

bool is_normal(char c) {
    return !is_special_external(c);    
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';    
}

bool is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_word_char(char c) {
    return is_digit(c) || is_letter(c) || c == '_';
}

// returns true if c is a special character
// that can never be activated as a valid state
bool never_active(char c) {
    string special = "](){}|*+?^";
    return special.find(c) != std::string::npos;
}

bool is_hx_symbol(char c) {
    return ('0' <= c && c <='9') || ('A' <= c && c <= 'F') || ('a' <= c && c <='f');    
}

char hx_to_int(char c) {
    if (is_digit(c))
        return c - '0';
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return '0';
}

tuple<char, int> convert_to_hx(string re, int start) {
    tuple<char, int> not_hx('\x0', start);
    if (start < 2 || !is_hx_symbol(re[start]))
        return not_hx;
    char num = '\x0';
    char second_char = hx_to_int(re[start]);
    start--;
    // '\x5' type of numbers
    if (start > 0 && re[start] == 'x' && re[start-1] == '\\')
        return tuple<char, int>{num + second_char, start-2};

    // '\x53' type of numbers
    if (!is_hx_symbol(re[start]))
        return not_hx;
    char first_char = hx_to_int(re[start]);
    num = num + first_char;
    num = (num << 4) + second_char;
    start--;
    if (start > 0 && re[start] == 'x' && re[start-1] == '\\')
        return tuple<char, int>{num, start-2};

   return not_hx;
}

/**
Transform the escaped character into the corresponding char class.
*/
string escape_char(char c) {
    if (c == 'n') return "\n";
    else if (c == 'r') return "\r";
    else return "(\\" + string(1, c) + ")";
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
e.g. "a{3}" becomes "aaa"; "a{2,4}" becomes "aa(a(a)?)?"

re[start] = + => "a+" becomes "aa*"

re[start] = any other char => returns that char and start - 1
*/
tuple<string, int, string> expand_sub_regex(string re, int start, string flags) {
    if (start < 0) {
        // base case: we've processed the entire regex
        return tuple<string, int, string>{"", -1, ""};
    }

    tuple<char, int> possible_hx = convert_to_hx(re, start);
    char hx = get<0>(possible_hx);
    int new_start = get<1>(possible_hx);
    if (new_start < start) {
        // we have a hx conversion
        string s = string(1, hx);
        if (is_special_external(hx))
            s = escape_char(hx);
        string curr_flags = "";
        for (int si = 0; si < (int)s.length(); si++) {
            curr_flags += flags[start];   
        }
        return tuple<string, int, string>{s, new_start, curr_flags};
    }
    if (start > 0 && re[start-1] == '\\') {
        // character escaping
        string s = escape_char(re[start]);
        string curr_flags = "";
        for (int si = 0; si < (int)s.length(); si++) {
            curr_flags += flags[start];   
        }
        return tuple<string, int, string>{s, start - 2, curr_flags};
    } else if (re[start] == ')' || re[start] == ']') {
        char end = (re[start] == ')') ? '(' : '[';
        string s = "";
	    string curr_flags = "";
        int idx = start - 1;
        // repeatedly parse the expression until we
        // reach the opening bracket
        while (re[idx] != end) {
            tuple<string, int, string> sub_s = expand_sub_regex(re, idx, flags);
            string ss = get<0>(sub_s);
	        string sub_flags = get<2>(sub_s);
            s = ss + s;
	        curr_flags = sub_flags + curr_flags;
            idx = get<1>(sub_s);
        }
	    curr_flags = flags[idx] + curr_flags + flags[start];
        return tuple<string, int, string>{end + s + re[start], idx - 1, curr_flags};
    } else if (re[start] == '}') {
        int counters[2] = {0, 0};
        // extract the counters
        int idx = get_counters(re, start - 1, counters);
        // parse the group that needs to be repeated
        tuple<string, int, string> sub_s = expand_sub_regex(re, idx, flags);
        // replicate the group
        string s = "";
	    string curr_flags = "";
        string repeat_flag = curr_flags + flags[start];
        // repetition from the first counter
        for (int i = 0; i < counters[0]; i++) {
            s += get<0>(sub_s);
	        curr_flags += get<2>(sub_s);
            counters[1] -= 1;
        }
        if (re[start-1] == ',') { // {m,} = m or more
            s += get<0>(sub_s) + "*";
            curr_flags += get<2>(sub_s) + repeat_flag;
        } else { // {m} or {m,n}
            string rest = "";
            string rest_flags = "";
            // repetition from the second counter
            for (int i = 0; i < counters[1]; i++) {
                rest = "(" + get<0>(sub_s) + rest + ")?";
                rest_flags = repeat_flag + get<2>(sub_s) + rest_flags + repeat_flag + repeat_flag;
            }
            s += rest;
            curr_flags += rest_flags;
        }
        return tuple<string, int, string>{s, get<1>(sub_s), curr_flags};
    } else if (re[start] == '+') {
        tuple<string, int, string> sub_s = expand_sub_regex(re, start-1, flags);
        string s = get<0>(sub_s);
        string curr_flags = get<2>(sub_s);
        return tuple<string, int, string>{s + s + "*", get<1>(sub_s), curr_flags + curr_flags + flags[start]};
    } else {
        // normal chracter
        string s = ""; 
        return tuple<string, int, string>{s + re[start], start - 1, s + flags[start]};
    }

}

/**
Combines the results from each group found by
`expand_sub_regex` into a single pattern.
*/
tuple<string, string> expand_regex(string re, string flags) {
    int start = re.length() - 1;
    // if there are no flags passed initialize them
    // to be the same length as the string
    if (flags.length() == 0) {
        flags = "";
        for (int si = 0; si < (int)re.length(); si++)
            flags = "." + flags;
    }
    assert(re.length() == flags.length());
    string s = "";
    string f = "";
    while (start >= 0) {
        if (start == 0 && re[start] == '^') {
            // ^ is converted into the start_anchor flag
            // we don't need it to be a part of the regex
            break;
        }
        tuple<string, int, string> sub_s = expand_sub_regex(re, start, flags);
        s = get<0>(sub_s) + s;
        f = get<2>(sub_s) + f;
        start = get<1>(sub_s);
    }
    assert(s.length() == f.length());
    return tuple<string, string>{s, f};

}

/**
Fills `positions` with the indices of the | chars s.t.
a |'s position holds the index of the next |, or ) if there
are no more | chars in the group. The ('s position holds the
index of the first | in the group, or ) if it's not an or group.
`positions` size = `str` length
*/
void get_or_positions(string &str, int* positions) {
    int idx = str.length() - 1;
    vector<int> brackets;
    vector<int> or_indices;
    while (idx >= 0) {
        if (str[idx] == ')') {
            brackets.push_back(idx);
            or_indices.push_back(idx);
        } else if (str[idx] == '(') {
            positions[idx] = or_indices.back();
            or_indices.pop_back();
            brackets.pop_back();
        } else if (str[idx] == '|') {
            positions[idx] = or_indices.back();
            or_indices.pop_back();
            or_indices.push_back(idx);
        }
        idx--;
    }
}

/**
Splits a regex into separate patterns based on |
e.g. "ab(cd|ef)" is split into "abcd" and "abef".
*/
set<string> split_regex(string &str, int* or_positions, int start, int end) {
    int idx = start;
    set<string> result;
    while (idx <= end) {
        set<string> options;
        if (str[idx] != '(') {
            string s = "";
            while (idx <= end && str[idx] != '(') {
                s += str[idx];
                idx++;
            }
            options.insert(s);
        } else {
            // parse the (..) groups with or without |
            bool need_parens = (str[or_positions[idx]] == ')');
            while (str[idx] != ')') {
                set<string> sub_result = split_regex(str, or_positions, idx+1, or_positions[idx]-1);
                idx = or_positions[idx];
                for (string s: sub_result) {
                    if (need_parens)
                        s = '(' + s + ')';
                    options.insert(s);
                }
            }
            idx++;
        }
        // join the parts
        if (result.size() == 0) {
            result = options;
        } else {
            // e.g. {ab} is combined with {cd, ef}
            // into {abcd, abef}
            set<string> new_result;
            for (string s2: options) {
                for (string s1: result) {
                    new_result.insert(s1 + s2);
                }
            }
            result = new_result;
        }
    }
    return result;

}

