#include "match_with_schedule.h"

bool is_in_group(int index, const char* flags, int re_len) {
    return index < re_len && (flags[index] == 'g');
}

void update_groups_from_cache(dyn_var<char*> dyn_states, static_var<char>* static_states, const char* flags, int* cache, int p, int re_len, bool update) {
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

void update_states(Schedule options, dyn_var<char*> dyn_states, static_var<char>* static_states, const char* flags, int* cache, int p, int re_len, bool update) {
    if (!update)
        return;
    if (options.state_group) {
        update_groups_from_cache(dyn_states, static_states, flags, cache, p, re_len, update);    
    } else {
        update_from_cache(static_states, cache, p, re_len, update);    
    }
}

dyn_var<int> spawn_matcher(dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> str_start, dyn_var<char*> dyn_current, dyn_var<char*> dyn_next, int start_state, std::set<int> &working_set, std::set<int> &done_set) {
    
    if (done_set.find(start_state) == done_set.end()) {
        working_set.insert(start_state);
    }

    std::string name = "match_" + std::to_string(start_state);
    dyn_var<int(char*, int, int, char*, char*)> branch_func = builder::with_name(name);
    return branch_func(str, str_len, str_start, dyn_current, dyn_next);
}

dyn_var<int> match_with_schedule(const char* re, int first_state, std::set<int> &working_set, std::set<int> &done_set, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, bool enable_partial, int* cache, int match_index, Schedule options, const char* flags, dyn_var<char*> dyn_current, dyn_var<char*> dyn_next) {

    const int re_len = strlen(re);
    bool ignore_case = options.ignore_case;
    int n_threads = options.interleaving_parts;

    // allocate two state vectors
    std::unique_ptr<static_var<char>[]> current(new static_var<char>[re_len + 1]());
    std::unique_ptr<static_var<char>[]> next(new static_var<char>[re_len + 1]());
    
    // initialize the state vector
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        current[i] = next[i] = 0;
        if (options.state_group)
            dyn_current[i] = dyn_next[i] = 0;
    }

    // activate the initial states
    update_states(options, dyn_current, current.get(), flags, cache, first_state-1, re_len, true);
    
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
                        if (match_char(str[to_match], m, ignore_case)) {
                            update_states(options, dyn_next, next.get(), flags, cache, state, re_len, update);
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
                        update_states(options, dyn_next, next.get(), flags, cache, state, re_len, update);
                        state_match = 1;
                    }
                } else if ('.' == m) {
                    update_states(options, dyn_next, next.get(), flags, cache, state, re_len, update);
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
                        } else if (match_char(str[to_match], re[idx], ignore_case)) {
                            matches = 1;
                            break;
                        }
                        idx = idx + 1;
                    }
		            if ((inverse == 1 && matches == 0) || (inverse == 0 && matches == 1)) {
                        state_match = 1;
                        update_states(options, dyn_next, next.get(), flags, cache, state, re_len, update);
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
                if (spawn_matcher(str, str_len, to_match+1, dyn_current, dyn_next, state+1, working_set, done_set)) {
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
                update_states(options, dyn_next, next.get(), flags, cache, -1, re_len, true);
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
            for (static_var<int> i = 0; i < re_len + 1; i++) {
                if (is_in_group(i, flags, re_len)) {
                    dyn_current[i] = dyn_next[i];
                    dyn_next[i] = 0;
                }    
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
