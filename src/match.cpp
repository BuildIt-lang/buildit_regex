#include "match.h"

/*
The code below is taken from https://intimeand.space/docs/CGO2022-BuilDSL.pdf
*/


/**
It returns whether `re` is a valid regular expression.
It updates `states` such that if a character is inside brackets
the next character we should look at in `progress` is just after the brackets.
If it's outside brackets then look at the char right after the current one.
*/
bool process_re(const char* re, static_var<int> *states) {
    const int re_len = strlen(re);
    int idx = re_len - 1;
    while (idx >= 0) {
        if (re[idx] == ']') {
            int s = idx + 1;
            while (re[idx] != '[') {
                // the current char is inside brackets
                // the next state is after the closing bracket
                states[idx] = s;
                idx = idx - 1;
                // couldn't find a closing bracket => invalid regex
                if (idx < 0)
                    return false;
            }
        }
        // check for invalid characters
        // TODO: currently does not handle '.' inside brackets
        if (!(is_normal(re[idx]) || re[idx] == '[' || re[idx] == '*'))
            return false;
        // if the current char is not inside brackets,
        // the next state is after the current char
        states[idx] = idx + 1;
        idx = idx - 1;
    }
    return true;
}

bool is_normal(char m) {
    return (m >= 'a' && m <= 'z') || (m >= 'A' && m <= 'Z') || (m >= '0' && m <= '9');
}

void progress(const char *re, static_var<char> *next, static_var<int> *ns_arr, int p) {
    // unsigned int ns = p + 1;
    unsigned int ns = (unsigned int)ns_arr[p];
    if (strlen(re) == ns) {
        next[ns] = true;
    } else if (is_normal(re[ns]) || '.' == re[ns]) {
        next[ns] = true;
        if ('*' == re[ns+1]) {
        // We are allowed to skip this
        // so just progress again
        progress(re, next, ns_arr, ns+1);
        }
    } else if ('*' == re[ns]) {
        if (ns != 0 && re[ns-1] == ']') {
            // this * comes right after []
            // allow to match any of the chars from inside []
            int curr_idx = ns - 2;
            while (re[curr_idx] != '[') {
                next[curr_idx] = true;
                curr_idx = curr_idx - 1;
            }
        } else {
            // can match the char before *
            next[ns-1] = true;
        }
        progress(re, next, ns_arr, ns);
    } else if ('[' == re[ns]) {
        int curr_idx = ns + 1;
        while (re[curr_idx] != ']') {
            // allowed to match any of the chars inside []
            next[curr_idx] = true;
            curr_idx = curr_idx + 1;
        }
        if (curr_idx < (int)strlen(re) - 1 && '*' == re[curr_idx+1])
            // allowed to skip []
            progress(re, next, ns_arr, curr_idx+1);
    }

}

dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len) {
    // allocate two state vectors
    const int re_len = strlen(re);
    static_var<char> *current = new static_var<char>[re_len + 1];
    static_var<char> *next = new static_var<char>[re_len + 1];
    static_var<int> *progress_ns = new static_var<int>[re_len];
    bool re_valid = process_re(re, progress_ns);
    if (!re_valid) {
        printf("Invalid regex");
        return false;
    }

    for (static_var<int> i = 0; i < re_len + 1; i++)
        current[i] = next[i] = 0;
    progress(re, current, progress_ns, -1);
    // dyn_var<int> str_len = d_strlen(str);
    dyn_var<int> to_match = 0;
    while (to_match < str_len) {
        // Don’t do anything for $.
        static_var<int> early_break = -1;
        for (static_var<int> state = 0; state < re_len; ++state)
            if (current[state]) {
                static_var<char> m = re[state];
                if (is_normal(m)) {
                    if (-1 == early_break) {
                        // Normal character
                        if (str[to_match] == m) {
                            progress(re, next, progress_ns, state);
                            // If a match happens, it
                            // cannot match anything else
                            // Setting early break
                            // avoids unnecessary checks
                            early_break = m;
                        }
                    } else if (early_break == m) {
                        // The comparison has been done
                        // already, let us not repeat
                        progress(re, next, progress_ns, state);
                    }
                } else if ('.' == m) {
                    progress(re, next, progress_ns, state);
                } else {
                    printf("Invalid Character(%c)\n", (char)m);
                    return false;
                }
            }
        // All the states have been checked
        // Now swap the states and clear next
        static_var<int> count = 0;
        for (static_var<int> i = 0; i < re_len + 1; i++) {
            current[i] = next[i];
            next[i] = false;
            if (current[i])
                count++;
        }
        if (count == 0)
            return false;
        to_match = to_match + 1;
    }
    // Now that the string is done,
    // we should have $ in the state
    static_var<int> is_match = (char)current[re_len];
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        next[i] = 0;
        current[i] = 0;
    }
    return is_match;
}

