#include "match_with_schedule.h"


// is the state at index a part of a group of states
bool is_in_group(int index, const char* flags, int re_len) {
    return index < re_len && (flags[index] == 'g');
}

// same as update_from_cache but updates a dynamic array in case of grouped states
void update_groups_from_cache(dyn_var<char[]>& dyn_states, static_var<char[]>& static_states, const char* flags, int* cache, int p, int re_len, bool update) {
    if (!update)
        return;
    for (static_var<int> i = 0; i < re_len + 1; i = i + 1) {
        static_var<char> cache_val = cache[(p+1) * (re_len + 1) + i];
        if (!cache_val)
            continue;
        bool grouped = is_in_group(i, flags, re_len);
        if (grouped) {
            dyn_states[i] = cache_val;
        } else {
            static_states[i] = cache_val;    
        }
    }    
}

// generic version - chooses between grouped and normal version of cache update
void update_states(Schedule options, dyn_var<char[]>& dyn_states, static_var<char[]>& static_states, const char* flags, int* cache, int p, int re_len, bool update) {
    if (!update)
        return;
    if (options.state_group) {
        update_groups_from_cache(dyn_states, static_states, flags, cache, p, re_len, update);    
    } else {
        update_from_cache(static_states, cache, p, re_len, update);    
    }
}

// adds a new function to be generated to the working set
dyn_var<int> spawn_matcher(dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> str_start, int start_state, std::set<int> &working_set, std::set<int> &done_set) {
    
    if (done_set.find(start_state) == done_set.end()) {
        working_set.insert(start_state);
    }

    std::string name = "match_" + std::to_string(start_state);
    dyn_var<int(char*, int, int)> branch_func = builder::with_name(name);
    return branch_func(str, str_len, str_start);
}

// the main matching function that is used to generate code
dyn_var<int> match_with_schedule(const char* re, int first_state, std::set<int> &working_set, std::set<int> &done_set, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, bool enable_partial, int* cache, int match_index, Schedule options, const char* flags) {
    const int re_len = strlen(re);
    bool ignore_case = options.ignore_case;
    int n_threads = options.interleaving_parts;

    // allocate two state vectors
    //std::unique_ptr<static_var<char>[]> current(new static_var<char>[re_len + 1]());
    //std::unique_ptr<static_var<char>[]> next(new static_var<char>[re_len + 1]());
    
    static_var<char[]> current;
    static_var<char[]> next;
    current.resize(re_len + 1);
    next.resize(re_len + 1);

    // initialize the state vector
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        current[i] = next[i] = 0;
    }

    // allocate and initialize the dynamic state vectors
    dyn_var<char[]> dyn_current;
    dyn_var<char[]> dyn_next;
    resize_arr(dyn_current, re_len + 1);
    resize_arr(dyn_next, re_len + 1);
    
    // use this in the loops that reset the dyn state vectors
    dyn_var<int> dyn_idx = 0;

    if (options.state_group) {
        for (dyn_idx = 0; dyn_idx < re_len + 1; dyn_idx = dyn_idx + 1) {
            dyn_current[dyn_idx] = 0;
            dyn_next[dyn_idx] = 0;
        }
        
    }

    // activate the initial states
    update_states(options, dyn_current, current, flags, cache, first_state-1, re_len, true);
   
    // keep the dyn declarations here to avoid
    // generating too many variables
    dyn_var<int> char_matched;
    dyn_var<char> str_to_match;

    static_var<int> mc = 0;
    while (to_match < str_len) {
		if (enable_partial && current[re_len]) { // partial match stop early
            break;
		}

        // Don’t do anything for $.
        static_var<int> early_break = -1;
        for (static_var<int> state = 0; state < re_len; ++state) {
           
            // only update current from the cache if update is set to 1
            // if current[state] == '|' we don't want to update from this
            // state, instead we are going to spawn a new function
            bool grouped = options.state_group && is_in_group(state, flags, re_len);
            //bool update = !(options.or_split && current[state] == '|');
            bool update = !(options.or_split && (flags[state] & 2) == 2);
            // check if there is a match for this state
            static_var<int> state_match = 0;
            if ((!grouped && current[state]) || (grouped && (bool)dyn_current[state])) {
                static_var<char> m = re[state];
                if (is_normal(m)) {
                    if (-1 == early_break) {
                        // Normal character
                        str_to_match = str[to_match];
                        char_matched = match_char(str_to_match, m, ignore_case);
                        if (char_matched) {
                            update_states(options, dyn_next, next, flags, cache, state, re_len, update);
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
                        update_states(options, dyn_next, next, flags, cache, state, re_len, update);
                        state_match = 1;
                    }
                } else if ('.' == m) {
                    update_states(options, dyn_next, next, flags, cache, state, re_len, update);
                    state_match = 1;
                } else if ('[' == m) {
		            dyn_var<int> matched = match_class(str[to_match], re, state, ignore_case);
                    if (matched) {
                        state_match = 1;
                        update_states(options, dyn_next, next, flags, cache, state, re_len, update);
                    }
                } else {
                    //printf("Invalid Character(%c)\n", (char)m);
                    return false;
                }
            }
            // if this state matches the current char in string
            // and it is the first state in one of the options in an or group
            // create and call a new function
            //if (options.or_split && state_match && current[state] == '|') {
            if (options.or_split && state_match && (flags[state] & 2) == 2) {
                if (spawn_matcher(str, str_len, to_match+1, state+1, working_set, done_set)) {
                    return 1;
                }
            }

        }

        // All the states have been checked
		if (enable_partial && first_state == 0) {
			// in case of first_state != 0 we need a partial match
            // that starts from the beginning of the string;
            // no other partial match will do
			if (mc == match_index) {
                update_states(options, dyn_next, next, flags, cache, -1, re_len, true);
            }
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
        if (options.state_group) {
            for (dyn_idx = 0; dyn_idx < re_len + 1; dyn_idx = dyn_idx +1) {
                dyn_current[dyn_idx] = dyn_next[dyn_idx];
                dyn_next[dyn_idx] = 0;
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
