#include "match.h"

/*
Parts of the code below are taken from https://intimeand.space/docs/CGO2022-BuilDSL.pdf
*/

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


void update_start_positions(dyn_var<int> start_loc, int state, int* next_states, dyn_var<set_t<int>*> current_start, dyn_var<set_t<int>*> next_start, int re_len) {
    for (static_var<int> i = 0; i < re_len + 1; i = i + 1) {
        // TODO: cannot index into a static array with dyn variable
        if (next_states[(state+1) * (re_len + 1) + i]) {
            if (state == -1) {
                //next_start[i].insert(start_loc);
                set_t_insert(next_start[i], start_loc);
            } else {
                next_start[i] = set_t_union(next_start[i], current_start[state]);
            }
        }
    }
        
} 

/**
Update `next` with the reachable states from state `p`.
*/

void update_from_cache(static_var<char>* next, int* cache, int p, int re_len) {
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        next[i] = cache[(p+1) * (re_len + 1) + i] || next[i];
    }    
}


/**
Matches each character in `str` one by one.
*/
dyn_var<map_t<int, set_t<int>>> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool enable_partial, int* cache) {
    const int re_len = strlen(re);

    // allocate two state vectors
    static_var<char> *current = new static_var<char>[re_len + 1];
    static_var<char> *next = new static_var<char>[re_len + 1];
    
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        current[i] = next[i] = 0;
    }
    
    // arrays to store starting positions of patial matches
    dyn_var<set_t<int>[]> current_start;
    resize(current_start, (int)re_len + 1);
    dyn_var<set_t<int>[]> next_start;
    resize(next_start, (int)re_len + 1);

    update_from_cache(current, cache, -1, re_len);
    update_start_positions(0, -1, cache, current_start, current_start, re_len);

    dyn_var<map_t<int, set_t<int>>> all_matches;
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
                            update_from_cache(next, cache, state, re_len);
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
                        update_from_cache(next, cache, state, re_len);
                        state_match = 1;
                    }
                } else if ('.' == m) {
                    update_from_cache(next, cache, state, re_len);
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
                        update_from_cache(next, cache, state, re_len);
                    }
                } else if ('-' == m) {
                    static_var<char> left = re[state - 1];
                    static_var<char> right = re[state + 1];
                    if (is_in_range(left, right, str[to_match])) {
                        update_from_cache(next, cache, state, re_len);
                        state_match = 1;
                    }
                } else {
                    printf("Invalid Character(%c)\n", (char)m);
                    return all_matches;
                }

                if (state_match == 1 && open_bracket == 1) bracket_match = 1;

                if (state_match) {
                    update_start_positions(-1, state, cache, current_start, next_start, re_len);    
                }
            }
        }
        // All the states have been checked
		if (enable_partial) {
            // if partial add the first state as well
			update_from_cache(next, cache, -1, re_len);
            update_start_positions(to_match+1, -1, cache, current_start, next_start, re_len);
		}
        // Now swap the states and clear next
        static_var<int> count = 0;
        for (static_var<int> i = 0; i < re_len + 1; i++) {
            current[i] = next[i];
            next[i] = false;
            if (current[i])
                count++;
        }
        if (count == 0)
            return all_matches;

        // update start positions
        for (dyn_var<int> i = 0; i < re_len + 1; i=i+1) {
            set_t_update(current_start[i], next_start[i]);
        }
        to_match = to_match + 1;

    }
    // Now that the string is done,
    // we should have $ in the state
    static_var<int> is_match = (char)current[re_len];
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        next[i] = 0;
        current[i] = 0;
    }
    if (is_match) {
        all_matches = map_t_update(all_matches, str_len, current_start[re_len]);
    }
    return all_matches;
}

dyn_var<int> match_regex_full(const char* re, dyn_var<char*> str, dyn_var<int> str_len, int* cache) {
	dyn_var<map_t<int, set_t<int>>> all_matches = match_regex(re, str, str_len, false, cache);
    return !all_matches.empty();
}

dyn_var<int> match_regex_partial(const char* re, dyn_var<char*> str, dyn_var<int> str_len, int* cache) {
	dyn_var<map_t<int, set_t<int>>> all_matches = match_regex(re, str, str_len, true, cache);
    return !all_matches.empty();
}

dyn_var<map_t<int, set_t<int>>> find_all_matches(const char* re, dyn_var<char*> str, dyn_var<int> str_len, int* cache) {
    return match_regex(re, str, str_len, true, cache);
}



