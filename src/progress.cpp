#include "progress.h"

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
    vector<int> closed_parens;
    vector<int> closed_brackets;
    vector<int> or_indices;
    int idx = re_len - 1;
    while (idx >= 0) {
        if (idx > 0 && re[idx - 1] == '\\') {
            // only mark '\\' with 1 / ignore the current char
            idx--;
            continue;
        }        
        char c = re[idx];
        bool inside_brackets = closed_brackets.size() > 0;
        // keep track of () and [] pairs
        if (c == ']') closed_brackets.push_back(idx);
        else if (c == ')' && !inside_brackets) {
            closed_parens.push_back(idx);
            or_indices.push_back(idx);
        } else if (c == '[') {
            int last_bracket = closed_brackets.back();
            re_states.brackets[idx] = last_bracket;
            re_states.brackets[last_bracket] = idx;
        } else if (c == '(' && !inside_brackets) {
            int last_paren = closed_parens.back();
            closed_parens.pop_back();
            re_states.brackets[idx] = last_paren;
            re_states.brackets[last_paren] = idx;
            re_states.helper_states[idx] = or_indices.back();
            or_indices.pop_back();
        }

        bool escape = (c == '\\');
        if (escape && (idx == re_len - 1)) {
            cout << "\\ has to be followed by a character" << endl;
            return false;
        }

        // if c has just been matched against the string
        // find the next character/state from re that should be matched
        if (idx == re_len - 1) {
            re_states.next[idx] = idx + 1;
        } else if (c != ']' && closed_brackets.size() > 0) {
            // we are inside brackets
            // all chars map to the same state as the closing bracket
            // all chars inside brackets are treated as literals e.g. \. is turned into [.]
            re_states.next[idx] = re_states.next[closed_brackets.back()];
			if (c == '[') {
                closed_brackets.pop_back();
			}
        } else if (c == '|') {
            re_states.next[idx] = idx + 1;
            re_states.helper_states[idx] = or_indices.back();
            or_indices.pop_back();
            or_indices.push_back(idx);
        } else if (is_special_internal(c) || is_normal(c)) {
            int next_i = (escape) ? idx + 2 : idx + 1;
            char next_c = re[next_i];
            string normal_next = "([^*.\\";
            if (is_normal(next_c) || normal_next.find(next_c) != std::string::npos) {
                re_states.next[idx] = next_i;   
            } else if (next_c == ')' || next_c == ']' || next_c == '?') {
                // if it's a `?` it means we've already had a match, so just skip it
                re_states.next[idx] = re_states.next[next_i];    
            } else if (next_c == '|') {
                // we are right before |
                // map to the state just after the enclosing () that contain the |'s
                if (re[idx] == ')') {
                    int last = closed_parens.back();
                    closed_parens.pop_back();
                    re_states.next[idx] = re_states.next[closed_parens.back()];
                    closed_parens.push_back(last);
                } else {
                    re_states.next[idx] = re_states.next[closed_parens.back()];
                }
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
    
    // if the state is a part of | group mark it with |
    // this helps later when splitting the matching into mupltiple functions
    
    if (strlen(re) == ns) {
        cache.temp_states[ns] = true;
    } else if (ns > 0 && re[ns-1] == '\\') {
        // do nothing
    } else if (re[ns] == '\\') {
        // mark pnly '\\' as active
        // for repeatition enclose in () -> (\\.)*
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
    } else if ('(' == re[ns]) {
        int or_index = re_states.helper_states[ns];
        progress(re, re_states, ns, cache); // char right after (
        // start by trying to match the first char after each |
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
        if ((state > -1 && never_active(re[state])) || (state > 0 && re[state-1] == '\\')) {
            for (int i = 0; i < re_len + 1; i++) {
                cache.next_states[(state + 1) * (re_len + 1) + i] = 0;
            }
        } else {
            progress(re, re_states, state, cache);   
            reset_array(cache.temp_states, re_len + 1);
        }
    }

    delete[] re_states.next;
    delete[] re_states.brackets;
    delete[] re_states.helper_states;
    delete[] cache.temp_states;
    delete[] cache.is_cached;
}


