#include "progress.h"

/**
Returns if `m` is an alphanumeric character.
*/
bool is_normal(char m) {
    return (m >= 'a' && m <= 'z') || (m >= 'A' && m <= 'Z') || (m >= '0' && m <= '9');
}

bool is_digit(char m) {
    return m >= '0' && m <= '9';
}


/**
It returns whether `re` is a valid regular expression.

`next_state` an array of length `strlen(re)`; for each char in `re`,
given that that char has just been matched against the input string,
it stores the index of the next char / state in `re` that should be processed

`brackets` is an array of the same length as `re` that holds the index
of a closing ( or [ for each opening one, and vice versa. 

`helper_states` is an array of the same length as `re` which holds the indices
of `|` chars inside each pair of (). The `(` location stores the index of the
first `|`, the location of the first `|` stores the index of the second `|`, and
so on, the last `|` has the index of `)`
It also stores 0 for '+' initially, which is changed to 1 when '+' sends a state back for
the first time.
*/
bool process_re(const char *re, int *next_states, int *brackets, int *helper_states) {
    int re_len = (int)strlen(re);
    vector<int> closed_parans; 
    int last_bracket = -1;
    vector<int> or_indices;
    int idx = re_len - 1;
    while (idx >= 0) {
        char c = re[idx];

        // keep track of () and [] pairs
        if (c == ']') last_bracket = idx;
        else if (c == ')') {
            closed_parans.push_back(idx);
            or_indices.push_back(idx);
        } else if (c == '[') {
            brackets[idx] = last_bracket;
            brackets[last_bracket] = idx;
            last_bracket = -1;
        } else if (c == '(') {
            int last_paran = closed_parans.back();
            closed_parans.pop_back();
            brackets[idx] = last_paran;
            brackets[last_paran] = idx;
            helper_states[idx] = or_indices.back();
            or_indices.pop_back();
        }

        if (c == '+') helper_states[idx] = 0;

        // if c has just been matched against the string
        // find the next character/state from re that should be matched
        if (idx == re_len - 1) {
            next_states[idx] = idx + 1;
        } else if (c == '|') {
            next_states[idx] = idx + 1;
            helper_states[idx] = or_indices.back();
            or_indices.pop_back();
            or_indices.push_back(idx);
        } else if (re[idx+1] == '|') {
            // we are right before |
            // map to the state just after the enclosing () that contain the |'s
            if (re[idx] == ')') {
                int last = closed_parans.back();
                closed_parans.pop_back();
                next_states[idx] = next_states[closed_parans.back()];
                closed_parans.push_back(last);
            } else {
                next_states[idx] = next_states[closed_parans.back()];
            }
        } else if (c != ']' && last_bracket != -1) {
            // we are inside brackets
            // all chars map to the same state as the closing bracket
            next_states[idx] = next_states[last_bracket];
        } else if (c == ']' || c == '[' || c == ')' ||  c == '(' || is_normal(c) || c == '*' || c == '.' || c == '?' || c == '+') {
            char next_c = re[idx + 1];
            if (is_normal(next_c) || next_c == '^' || next_c == '*' || next_c == '.' || next_c == '(' || next_c == '[' || next_c == '+') {
                next_states[idx] = idx + 1;   
            } else if (next_c == ')' || next_c == ']' || next_c == '?') {
                // if it's a `?` it means we've already had a match, so just skip it
                next_states[idx] = next_states[idx+1];    
            } else {
                cout << "Invalid character: " << next_c << endl;
                return false;
            }
        } else {
            cout << "Invalid character: " << c << endl;
            return false;
        }

        idx = idx - 1;
    }
    return true;  
}

void progress(const char *re, int *ns_arr, int *brackets, int *helper_states, int p, char *cache, int *cache_states, char *temp) {
    const int re_len = strlen(re);

    unsigned int ns = (p == -1) ? 0 : (unsigned int)ns_arr[p];
    /*if (cache[p+1]) {
        for (static_var<int> i = 0; i < re_len + 1; i++) {
            temp[i] = cache_states[(p+1) * (re_len + 1) + i];
        }
        return;
    }*/

    if (strlen(re) == ns) {
        temp[ns] = true;
    } else if (is_normal(re[ns]) || '.' == re[ns]) {
        if ('*' == re[ns+1] || '?' == re[ns+1] || ('+' == re[ns+1] && helper_states[ns+1] == 1)) {
            // we can also skip this char
            progress(re, ns_arr, brackets, helper_states, ns+1, cache, cache_states, temp);
        }
        temp[ns] = true;
    } else if ('*' == re[ns] || '+' == re[ns]) { // can match char p again
        helper_states[ns] = true;
        int prev_state = (re[ns-1] == ')' || re[ns-1] == ']') ? brackets[ns-1] : ns - 1;
        progress(re, ns_arr, brackets, helper_states, prev_state-1, cache, cache_states, temp);
    } else if ('[' == re[ns]) {
        int curr_idx = ns + 1;
        if (brackets[ns] < (int)strlen(re) - 1 && ('*' == re[brackets[ns]+1] || '?' == re[brackets[ns]+1] || ('+' == re[brackets[ns]+1] && helper_states[brackets[ns]+1] == 1)))
            // allowed to skip []
            progress(re, ns_arr, brackets, helper_states, brackets[ns]+1, cache, cache_states, temp);
        if (re[ns + 1] == '^') {
            // negative class - mark only '^' as true
            // the character matching is handled in `match_regex`
            temp[ns + 1] = true;
        } else {
            while (re[curr_idx] != ']') {
                // allowed to match any of the chars inside []
                temp[curr_idx] = true;
                curr_idx = curr_idx + 1;
            }
        }
    } else if ('(' == re[ns]) {
        progress(re, ns_arr, brackets, helper_states, ns, cache, cache_states, temp); // char right after (
        // start by trying to match the first char after each |
        int or_index = helper_states[ns];
        while (or_index != brackets[ns]) {
            progress(re, ns_arr, brackets, helper_states, or_index, cache, cache_states, temp);
            or_index = helper_states[or_index];
        } 
        // if () are followed by *, it's possible to skip the () group
        if (brackets[ns] < (int)strlen(re) - 1 && ('*' == re[brackets[ns]+1] || '?' == re[brackets[ns]+1] || ('+' == re[brackets[ns]+1] && helper_states[brackets[ns]+1] == 1)))
            progress(re, ns_arr, brackets, helper_states, brackets[ns]+1, cache, cache_states, temp);
    }

    if (!cache[p+1]) {
        cache[p+1] = true;
        for (int i = 0; i < re_len + 1; i++) {
            cache_states[(p+1) * (re_len + 1) + i] = temp[i];
        }
    }
}

void reset_array(char* arr, int len) {
    for (int i = 0; i < len; i++) {
        arr[i] = 0;    
    }
}

void cache_states(const char* re, Cache cache, int* ns, int* brackets, int* helper_states) {
    int re_len = (int)strlen(re);
    char* temp_states = new char[re_len + 1];
    reset_array(temp_states, re_len + 1);
    bool valid = process_re(re, ns, brackets, helper_states);
    if (!valid) {
        std::cout << "Invalid regex in process_re" << std::endl;    
        return;
    }
    for (int state = -1; state < re_len; state++) {
        progress(re, ns, brackets, helper_states, state, cache.is_cached, cache.next_states, temp_states);   
        reset_array(temp_states, re_len + 1);
    }
}


