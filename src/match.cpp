#include "match.h"

/*
Parts of the code below are taken from https://intimeand.space/docs/CGO2022-BuilDSL.pdf
*/

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
                printf("Invalid character: %c\n", next_c);
                return false;
            }
        } else {
            printf("Invalid character: %c\n", c);
            return false;
        }

        idx = idx - 1;
    }
/*    printf("----\n");
    for (static_var<int> i = 0; i < re_len; i++) {
        printf("%d, ", (int)helper_states[i]);
    }
    printf("\n----\n");
    */
    return true;  
}

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
Returns if `c` is between the chars `left` and `right`.
*/
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c) {
    if (!(is_normal(left) && is_normal(right))) {
        printf("Invalid Characters %c %c\n", left, right);
        return 0; 
    }
    return left <= c && c <= right;
}

/**
Given that the character `re[p]` has just been matched, finds all the characters
in `re` that can be matched next and sets their corresponding locations in `next` to `true`.
*/
void progress(const char *re, static_var<char> *next, int *ns_arr, int *brackets, int *helper_states, int p, char *cache, int *cache_states, bool use_cache, static_var<char> *temp, dyn_var<set_t<int>*> next_start, dyn_var<set_t<int>> current_start) {
    const int re_len = strlen(re);

    int ns = (p == -1) ? 0 : (unsigned int)ns_arr[p];
    if (use_cache && cache[p+1]) {
        for (static_var<int> i = 0; i < re_len + 1; i++) {
            next[i] = cache_states[(p+1) * (re_len + 1) + i] || next[i];
            temp[i] = cache_states[(p+1) * (re_len + 1) + i];
        }
        return;
    }

    if ((int)strlen(re) == ns) {
        next[ns] = true;
        temp[ns] = true;
        next_start[ns] = set_t_union(next_start[ns], current_start);
    } else if (is_normal(re[ns]) || '.' == re[ns]) {
        next[ns] = true;
        next_start[ns] = set_t_union(next_start[ns], current_start);
        if ('*' == re[ns+1] || '?' == re[ns+1] || ('+' == re[ns+1] && helper_states[ns+1] == 1)) {
            // we can also skip this char
            progress(re, next, ns_arr, brackets, helper_states, ns+1, cache, cache_states, use_cache, temp, next_start, current_start);
        }
        temp[ns] = true;
    } else if ('*' == re[ns] || '+' == re[ns]) { // can match char p again
        helper_states[ns] = 1;
        int prev_state = (re[ns-1] == ')' || re[ns-1] == ']') ? brackets[ns-1] : ns - 1;
        progress(re, next, ns_arr, brackets, helper_states, prev_state-1, cache, cache_states, use_cache, temp, next_start, current_start);
    } else if ('[' == re[ns]) {
        static_var<int> curr_idx = ns + 1;
        if (brackets[ns] < (int)strlen(re) - 1 && ('*' == re[brackets[ns]+1] || '?' == re[brackets[ns]+1] || ('+' == re[brackets[ns]+1] && helper_states[brackets[ns]+1] == 1)))
            // allowed to skip []
            progress(re, next, ns_arr, brackets, helper_states, brackets[ns]+1, cache, cache_states, use_cache, temp, next_start, current_start);
        if (re[ns + 1] == '^') {
            // negative class - mark only '^' as true
            // the character matching is handled in `match_regex`
            next[ns + 1] = true;
            next_start[ns+1] = set_t_union(next_start[ns+1], current_start);
            temp[ns + 1] = true;
        } else {
            while (re[curr_idx] != ']') {
                // allowed to match any of the chars inside []
                next[curr_idx] = true;
                next_start[curr_idx] = set_t_union(next_start[curr_idx], current_start);
                temp[curr_idx] = true;
                curr_idx = curr_idx + 1;
            }
        }
    } else if ('(' == re[ns]) {
        progress(re, next, ns_arr, brackets, helper_states, ns, cache, cache_states, use_cache, temp, next_start, current_start); // char right after (
        // start by trying to match the first char after each |
        int or_index = helper_states[ns];
        while (or_index != brackets[ns]) {
            progress(re, next, ns_arr, brackets, helper_states, or_index, cache, cache_states, use_cache, temp, next_start, current_start);
            or_index = helper_states[or_index];
        } 
        // if () are followed by *, it's possible to skip the () group
        if (brackets[ns] < (int)strlen(re) - 1 && ('*' == re[brackets[ns]+1] || '?' == re[brackets[ns]+1] || ('+' == re[brackets[ns]+1] && helper_states[brackets[ns]+1] == 1)))
            progress(re, next, ns_arr, brackets, helper_states, brackets[ns]+1, cache, cache_states, use_cache, temp, next_start, current_start);
    }
    if (use_cache && !cache[p+1]) {
        cache[p+1] = true;
        for (static_var<int> i = 0; i < re_len + 1; i++) {
            cache_states[(p+1) * (re_len + 1) + i] = temp[i];
        }
    }
}


/**
Tries to match each character in `str` one by one.
It relies on `progress` to get the possible states we can transition to
from the current state.
*/
dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool enable_partial, bool use_cache, char* cache, int* cache_states, int* next_state, int* brackets, int* helper_states) {
    const int re_len = strlen(re);

    // allocate two state vectors
    static_var<char> *current = new static_var<char>[re_len + 1];
    static_var<char> *next = new static_var<char>[re_len + 1];
    static_var<char> *temp = new static_var<char>[re_len + 1];
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        current[i] = next[i] = temp[i] = 0;
    }
    // arrays to store starting positions of patial matches
    dyn_var<set_t<int>[]> current_start;
    resize(current_start, (int)re_len + 1);
    dyn_var<set_t<int>[]> next_start;
    resize(next_start, (int)re_len + 1);

    dyn_var<map_t<int, set_t<int>>> all_matches;
    bool re_valid = process_re(re, next_state, brackets, helper_states);
    if (!re_valid) {
        printf("Invalid regex");
        return all_matches;
    }

    dyn_var<set_t<int>> next_pos;
    next_pos.insert(0);
    progress(re, current, next_state, brackets, helper_states, -1, cache, cache_states, use_cache, temp, current_start, next_pos);
    for (static_var<int> k = 0; k < re_len + 1; k++) {
        temp[k] = 0;
    }
    
    dyn_var<int> to_match = 0;
    while (to_match < str_len) {
		if (enable_partial && current[re_len]) { // partial match stop early
            // adds the elements from the second set to the first one
            all_matches = map_t_update(all_matches, to_match, current_start[re_len]);
		}

        // Don’t do anything for $.
        static_var<int> early_break = -1;
        static_var<int> open_bracket = 0;
        static_var<int> bracket_match = 0;
        for (static_var<int> state = 0; state < re_len; ++state) {
            // flags for early skipping in case of a bracket match
            if (re[state] == '[') open_bracket = 1;
            else if (re[state] == ']') open_bracket = bracket_match = 0;
            // we are still inside [], but we already found a match
            // => skip iters up to the closing bracket
            if (bracket_match == 1) continue;
            // check if there is a match for this state
            static_var<int> state_match = 0;
            if (current[state]) {
                static_var<char> m = re[state];
                //std::set<int> next_start_pos;
                if (is_normal(m)) {
                    if (-1 == early_break) {
                        // Normal character
                        if (str[to_match] == m) {
                            progress(re, next, next_state, brackets, helper_states, state, cache, cache_states, use_cache, temp, next_start, current_start[state]);
                            // If a match happens, it
                            // cannot match anything else
                            // Setting early break
                            // avoids unnecessary checks
                            early_break = m;
                            state_match = 1;
                        }
                    } else if (early_break == m) {
                        // The comparison has been done
                        // already, let us not repeat
                        progress(re, next, next_state, brackets, helper_states, state, cache, cache_states, use_cache, temp, next_start, current_start[state]);
                        state_match = 1;
                    }
                } else if ('.' == m) {
                    progress(re, next, next_state, brackets, helper_states, state, cache, cache_states, use_cache, temp, next_start, current_start[state]);
                    state_match = 1;
                } else if ('^' == m) {
                    // we are inside a [^] class
                    static_var<int> idx = state + 1;
                    dyn_var<int> matches = 1;
                    // check if str[to_match] matches any of the chars in []
                    while (re[idx] != ']') {
                        if (re[idx] == str[to_match]) {
                            matches = 0;
                            break;
                        } else if (re[idx] == '-') {
                            // this is used for ranges, e.g. [a-d]
                            matches = !is_in_range(re[idx-1], re[idx+1], str[to_match]);
                            if (!matches)
                                break;
                        }
                        idx = idx + 1;
                    }
                    if (matches == 1) {
                        state_match = 1;
                        progress(re, next, next_state, brackets, helper_states, state, cache, cache_states, use_cache, temp, next_start, current_start[state]);
                    }
                } else if ('-' == m) {
                    static_var<char> left = re[state - 1];
                    static_var<char> right = re[state + 1];
                    if (is_in_range(left, right, str[to_match])) {
                        progress(re, next, next_state, brackets, helper_states, state, cache, cache_states, use_cache, temp, next_start, current_start[state]);
                        state_match = 1;
                    }
                } else {
                    printf("Invalid Character(%c)\n", (char)m);
                    return all_matches;
                }

                //update_start_positions(next_start, current_start, to_match, state, next_start_pos, re_len);
                if (state_match == 1 && open_bracket == 1) bracket_match = 1;

                for (static_var<int> k = 0; k < re_len + 1; k++) {
                    temp[k] = 0;
                }
            }
        }
        // All the states have been checked
		if (enable_partial) {
            // if partial add the first state as well
            dyn_var<set_t<int>> next_start_pos;
            next_start_pos.insert(to_match+1);
			progress(re, next, next_state, brackets, helper_states, -1, cache, cache_states, use_cache, temp, next_start, next_start_pos); // partial match match from start again
            if (use_cache) {
                for (static_var<int> k = 0; k < re_len + 1; k++) {
                    temp[k] = 0;
                }
            }
		}
        // Now swap the states and clear next
        static_var<int> count = 0;
        for (static_var<int> i = 0; i < re_len + 1; i++) {
            current[i] = next[i];
            next[i] = false;
            temp[i] = 0;
            if (current[i])
                count++;
        }
        if (count == 0)
            return all_matches;

        // update start positions
        for (dyn_var<int> i = 0; i < re_len + 1; i=i+1) {
            current_start[i] = next_start[i];
            dyn_var<set_t<int>> emp_set;
            next_start[i] = emp_set;
        }
        to_match = to_match + 1;

    }
    // Now that the string is done,
    // we should have $ in the state
    static_var<int> is_match = (char)current[re_len];
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        next[i] = 0;
        current[i] = 0;
        temp[i] = 0;
    }
    all_matches = map_t_update(all_matches, str_len, current_start[re_len]);
    return all_matches;
}

dyn_var<int> match_regex_full(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool use_cache, char* cache, int* cache_states, int* next_state, int* brackets, int* helper_states) {
	dyn_var<map_t<int, set_t<int>>> all_matches = match_regex(re, str, str_len, false, use_cache, cache, cache_states, next_state, brackets, helper_states);
    return !all_matches.empty();
}

dyn_var<int> match_regex_partial(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool use_cache, char* cache, int* cache_states, int* next_state, int* brackets, int* helper_states) {
	dyn_var<map_t<int, set_t<int>>> all_matches = match_regex(re, str, str_len, true, use_cache, cache, cache_states, next_state, brackets, helper_states);
    return !all_matches.empty();
}

dyn_var<map_t<int, set_t<int>>> find_all_matches(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool use_cache, char* cache, int* cache_states, int* next_state, int* brackets, int* helper_states) {
    return match_regex(re, str, str_len, true, use_cache, cache, cache_states, next_state, brackets, helper_states);
}



