#ifndef STATE_GROUPING_H
#define STATE_GROUPING_H

#include "match.h"

bool is_in_group(int index, int* groups, int re_len);

void update_groups_from_cache(dyn_var<char*> dyn_states, static_var<char>* static_states, int* groups, int* cache, int p, int re_len, bool update = true);

dyn_var<int> match_regex_with_groups(const char* re, int* groups, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, dyn_var<char*> dyn_current, dyn_var<char*> dyn_next, bool enable_partial, int* cache, int match_index, int n_threads, int ignore_case);
#endif
