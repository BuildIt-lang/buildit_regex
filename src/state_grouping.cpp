#include "state_grouping.h"

bool is_in_group(int index, int* groups, int re_len) {
    if (index >= re_len)
        return false;
    if (groups[index] != index)
        return true;
    return index + 1 < re_len && groups[index+1] == index; 
}

void update_groups_from_cache(dyn_var<char*> dyn_states, static_var<char>* static_states, int* groups, int* cache, int p, int re_len, bool update) {
    if (!update)
        return;
    for (static_var<int> i = 0; i < re_len + 1; i = i + 1) {
        static_var<char> cache_val = cache[(p+1) * (re_len + 1) + i];
        if (!cache_val)
            continue;
        if (is_in_group(i, groups, re_len)) {
            // we are inside of a group
            // update static states - activate group
            int group_i = groups[i];
            // set only the first state of the group to true
            static_states[group_i] = true;
            dyn_states[i] = true;
            //static_states[group_i] = cache_val || static_states[group_i];
            //dyn_states[i] = cache_val || dyn_states[i];
        } else {
            static_states[i] = cache_val || static_states[i];    
        }
    }    
}

/**
Matches each character in `str` one by one.
*/
dyn_var<int> match_regex_with_groups(const char* re, int* groups, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, dyn_var<char*> dyn_current, dyn_var<char*> dyn_next, bool enable_partial, int* cache, int match_index, int n_threads, int ignore_case) {
    const int re_len = strlen(re);
    // allocate two state vectors
    std::unique_ptr<static_var<char>[]> current(new static_var<char>[re_len + 1]());
    std::unique_ptr<static_var<char>[]> next(new static_var<char>[re_len + 1]());
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        current[i] = next[i] = 0;
    }

    update_groups_from_cache(dyn_current, current.get(), groups, cache, -1, re_len);
    
    static_var<int> mc = 0;
    static_var<int> any_group = 0; // are any states grouped?
    
    while (to_match < str_len) {
		if (enable_partial && current[re_len]) { // partial match stop early
            break;
		}

        // Don’t do anything for $.
        static_var<int> early_break = -1;
        for (static_var<int> state = 0; state < re_len; ++state) {
            // check if there is a match for this state
            bool grouped = is_in_group(state, groups, re_len);
            any_group = any_group || grouped;
            if ((!grouped && current[state]) || (grouped && (bool)dyn_current[state])) {
            //if (cond(grouped, current[state], dyn_current[state])) {
                static_var<char> m = re[state];
                if (is_normal(m)) {
                    if (-1 == early_break) {
                        // Normal character
                        if (str[to_match] == m || (ignore_case && is_alpha(m) && str[to_match] == (m ^ 32))) {
                            update_groups_from_cache(dyn_next, next.get(), groups, cache, state, re_len);
                            // If a match happens, it
                            // cannot match anything else
                            // Setting early break
                            // avoids unnecessary checks
                            early_break = m;
                        }
                    } else if (early_break == m) {
                        // The comparison has been done
                        // already, let us not repeat
                        update_groups_from_cache(dyn_next, next.get(), groups, cache, state, re_len);
                    }
                } else if ('.' == m) {
                    update_groups_from_cache(dyn_next, next.get(), groups, cache, state, re_len);
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
                        update_groups_from_cache(dyn_next, next.get(), groups, cache, state, re_len);
                    }
                } else {
                    // prints during generation as well...
                    //printf("Invalid Character(%c)\n", (char)m);
                    return false;
                }
            }
        }
        // All the states have been checked
		if (enable_partial) {
            // if partial add the first state as well
			if (mc == match_index)
				update_groups_from_cache(dyn_next, next.get(), groups, cache, -1, re_len);
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
        if (any_group) {
            for (static_var<int> i = 0; i < re_len + 1; i=i+1) {
                dyn_current[i] = dyn_next[i];
                dyn_next[i] = false;
            }
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
