#include "state_grouping.h"

/**
Update `next` with the reachable states from state `p`.
*/
void update_groups_from_cache(static_var<char>* next, int* cache, int p, int re_len, bool update) {
    if (!update)
        return;
    for (int i = 0; i < re_len + 1; i++) {
        static_var<char> cache_val = cache[(p+1) * (re_len + 1) + i];
        next[i] = (cache_val != 0) ? cache_val : next[i];
    }    
}

/**
Matches each character in `str` one by one.
*/
dyn_var<int> match_regex_with_groups(const char* re, int* groups, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, 
                            bool enable_partial, int* cache, int match_index, int n_threads, int ignore_case) {

    const int re_len = strlen(re);

    // allocate two state vectors
    std::unique_ptr<static_var<char>[]> current(new static_var<char>[re_len + 1]());
    std::unique_ptr<static_var<char>[]> next(new static_var<char>[re_len + 1]());
    
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        current[i] = next[i] = 0;
    }

    update_from_cache(current.get(), cache, -1, re_len);
    
    static_var<int> mc = 0;
    while (to_match < str_len) {
		if (enable_partial && current[re_len]) { // partial match stop early
			break;
		}

        // Don’t do anything for $.
        static_var<int> early_break = -1;
        for (static_var<int> state = 0; state < re_len; ++state) {
            // check if there is a match for this state
            static_var<int> state_match = 0;
            if (current[state]) {
                static_var<char> m = re[state];
                if (is_normal(m)) {
                    if (-1 == early_break) {
                        // Normal character
                        if (str[to_match] == m || (ignore_case && is_alpha(m) && str[to_match] == (m ^ 32))) {
                            update_from_cache(next.get(), cache, state, re_len);
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
                        update_from_cache(next.get(), cache, state, re_len);
                        state_match = 1;
                    }
                } else if ('.' == m) {
                    update_from_cache(next.get(), cache, state, re_len);
                    state_match = 1;
                } else if ('[' == m) {
                    // we are inside a [...] class
                    static_var<int> idx = state + 1;
					static_var<int> inverse = 0;
		            if ('^' == re[idx]) {
                        inverse = 1;
			            idx = idx + 1;
          		    }

					dyn_var<int> matches = 0;
                    // check if str[to_match] matches any of the chars in []
                    while (re[idx] != ']') {
                        if (re[idx] == '-') {
                            // this is used for ranges, e.g. [a-d]
                            bool in_range = is_in_range(re[idx-1], re[idx+1], str[to_match], ignore_case);
                            if (in_range) {
                                matches = 1;
                                break;
                            }
                        } else if (str[to_match] == re[idx] || (ignore_case && is_alpha(re[idx]) && str[to_match] == (re[idx] ^ 32))) {
                            matches = 1;
                            break;
                        }
                        idx = idx + 1;
                    }
		            if ((inverse == 1 && matches == 0) || (inverse == 0 && matches == 1)) {
                        state_match = 1;
                        update_from_cache(next.get(), cache, state, re_len);
                    }
                } else {
                    printf("Invalid Character(%c)\n", (char)m);
                    return false;
                }
            }
        }
        // All the states have been checked
		if (enable_partial) {
            // if partial add the first state as well
			if (mc == match_index)
				update_from_cache(next.get(), cache, -1, re_len);
			mc = (mc + 1) % n_threads;
		}
        // Now swap the states and clear next
        static_var<int> count = 0;
        for (static_var<int> i = 0; i < re_len + 1; i++) {
            current[i] = next[i];
            next[i] = false;
            if (current[i])
                count++;
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
    return is_match;
}

