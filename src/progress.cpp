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

`next` ia an array of length `strlen(re)`; for each char in `re`,
given that that char has just been matched against the input string,
it stores the index of the next char / state in `re` that should be processed

`brackets` is an array of the same length as `re` that holds the index
of a closing ( or [ for each opening one, and vice versa. 

`helper_states` is an array of the same length as `re` which holds the indices
of `|` chars inside each pair of (). The `(` location stores the index of the
first `|`, the location of the first `|` stores the index of the second `|`, and
so on, the last `|` has the index of `)`
*/
bool process_re(const char *re, ReStates re_states) {
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
            re_states.brackets[idx] = last_bracket;
            re_states.brackets[last_bracket] = idx;
        } else if (c == '(') {
            int last_paran = closed_parans.back();
            closed_parans.pop_back();
            re_states.brackets[idx] = last_paran;
            re_states.brackets[last_paran] = idx;
            re_states.helper_states[idx] = or_indices.back();
            or_indices.pop_back();
        }

        // if c has just been matched against the string
        // find the next character/state from re that should be matched
        if (idx == re_len - 1) {
            re_states.next[idx] = idx + 1;
        } else if (c == '|') {
            re_states.next[idx] = idx + 1;
            re_states.helper_states[idx] = or_indices.back();
            or_indices.pop_back();
            or_indices.push_back(idx);
        } else if (re[idx+1] == '|') {
            // we are right before |
            // map to the state just after the enclosing () that contain the |'s
            if (re[idx] == ')') {
                int last = closed_parans.back();
                closed_parans.pop_back();
                re_states.next[idx] = re_states.next[closed_parans.back()];
                closed_parans.push_back(last);
            } else {
                re_states.next[idx] = re_states.next[closed_parans.back()];
            }
        } else if (c != ']' && last_bracket != -1) {
            // we are inside brackets
            // all chars map to the same state as the closing bracket
            re_states.next[idx] = re_states.next[last_bracket];
			if (c == '[') {
				last_bracket = -1;
			}
        } else if (c == ']' || c == '[' || c == ')' ||  c == '(' || is_normal(c) || c == '*' || c == '.' || c == '?') {
            char next_c = re[idx + 1];
            if (is_normal(next_c) || next_c == '^' || next_c == '*' || next_c == '.' || next_c == '(' || next_c == '[') {
                re_states.next[idx] = idx + 1;   
            } else if (next_c == ')' || next_c == ']' || next_c == '?') {
                // if it's a `?` it means we've already had a match, so just skip it
                re_states.next[idx] = re_states.next[idx+1];    
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

/**
Updates the cache with all the reachable states from the state p.
*/
void progress(const char *re, ReStates re_states, int p, Cache cache) {
    const int re_len = strlen(re);

    unsigned int ns = (p == -1) ? 0 : (unsigned int)re_states.next[p];

    if (strlen(re) == ns) {
        cache.temp_states[ns] = true;
    } else if (is_normal(re[ns]) || '.' == re[ns]) {
        if ('*' == re[ns+1] || '?' == re[ns+1]) {
            // we can also skip this char
            progress(re, re_states, ns+1, cache);
        }
        cache.temp_states[ns] = true;
    } else if ('*' == re[ns]) { // can match char p again
        int prev_state = (re[ns-1] == ')' || re[ns-1] == ']') ? re_states.brackets[ns-1] : ns - 1;
        progress(re, re_states, prev_state-1, cache);
    } else if ('[' == re[ns]) {
//        int curr_idx = ns + 1;
        if (re_states.brackets[ns] < (int)strlen(re) - 1 && ('*' == re[re_states.brackets[ns]+1] || '?' == re[re_states.brackets[ns]+1]))
            // allowed to skip []
            progress(re, re_states, re_states.brackets[ns]+1, cache);
        cache.temp_states[ns] = true;
        /*if (re[ns + 1] == '^') {
            // negative class - mark only '^' as true
            // the character matching is handled in `match_regex`
            cache.temp_states[ns + 1] = true;
        } else {
            while (re[curr_idx] != ']') {
                // allowed to match any of the chars inside []
                cache.temp_states[curr_idx] = true;
                curr_idx = curr_idx + 1;
            }
        }*/
    } else if ('(' == re[ns]) {
        progress(re, re_states, ns, cache); // char right after (
        // start by trying to match the first char after each |
        int or_index = re_states.helper_states[ns];
        while (or_index != re_states.brackets[ns]) {
            progress(re, re_states, or_index, cache);
            or_index = re_states.helper_states[or_index];
        } 
        // if () are followed by *, it's possible to skip the () group
        if (re_states.brackets[ns] < (int)strlen(re) - 1 && ('*' == re[re_states.brackets[ns]+1] || '?' == re[re_states.brackets[ns]+1]))
            progress(re, re_states, re_states.brackets[ns]+1, cache);
    }

    // only update the cache at the bottom recursive call
    if (!cache.is_cached[p+1]) {
        cache.is_cached[p+1] = true;
        for (int i = 0; i < re_len + 1; i++) {
            cache.next_states[(p+1) * (re_len + 1) + i] = cache.temp_states[i];
        }
    }
}

void reset_array(char* arr, int len) {
    for (int i = 0; i < len; i++) {
        arr[i] = 0;    
    }
}

/**
`next` is a (re_len+1)*(re_len+1) sized array that holds
a (re_len+1) vector of all reachable states from each one of
the re_len+1 states corresponding to the characters in `re`

First, extracts useful information about the regex like bracket or |
locations, and then fills `next` by calling progress for each state.
*/
void cache_states(const char* re, int* next) {
    int re_len = (int)strlen(re);
    
    // initialize arrays for caching
    Cache cache;
    cache.next_states = next; // the array storing the reachable states
    // auxiliary array used to update cache.next_states in progress
    cache.temp_states = new char[re_len + 1];
    reset_array(cache.temp_states, re_len + 1);
    // is_cached: have we already cached the reachable states for each position?
    // each element of is_cached should be 1 at the end
    cache.is_cached = new char[re_len + 1];
    reset_array(cache.is_cached, re_len + 1);

    // initialize helper arrays for the regex
    ReStates re_states;
    re_states.next = new int[re_len]; 
    re_states.brackets = new int[re_len];
    re_states.helper_states = new int[re_len];

    bool valid = process_re(re, re_states);
    if (!valid) {
        std::cout << "Invalid regex in process_re" << std::endl;    
        return;
    }
    // for each state, fill the cache with the states
    // that can be reached starting from that state
    for (int state = -1; state < re_len; state++) {
        progress(re, re_states, state, cache);   
        reset_array(cache.temp_states, re_len + 1);
    }
    delete[] re_states.next;
    delete[] re_states.brackets;
    delete[] re_states.helper_states;
    delete[] cache.temp_states;
    delete[] cache.is_cached;
}


