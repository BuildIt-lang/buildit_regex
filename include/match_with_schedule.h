#ifndef MATCH_WITH_SCHEDULE_H
#define MATCH_WITH_SCHEDULE_H

#include "match.h"
#include <set>

struct Schedule {
    bool or_split = false;
    bool state_group = false;
    int interleaving_parts = 1;
    bool ignore_case = false;
    bool start_anchor = false;
    bool last_eom = false;
    bool reverse = false;
};

bool is_in_group(int index, const char* flags, int re_len);

bool update_groups_from_cache(dyn_var<char[]>& dyn_states, static_var<char[]>& static_states, const char* flags, int* cache, int p, int re_len, bool reverse, bool update, bool read_only=false);

bool update_states(Schedule options, dyn_var<char[]>& dyn_states, static_var<char[]>& static_states, const char* flags, int* cache, int p, int re_len, bool reverse, bool update, bool read_only=false);

dyn_var<int> spawn_matcher(dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> str_start, int start_state, std::set<int> &working_set, std::set<int> &done_set);

dyn_var<int> match_with_schedule(const char* re, int first_state, std::set<int> &working_set, std::set<int> &done_set, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, bool enable_partial, int* cache, int match_index, Schedule options, const char* flags);

#endif
