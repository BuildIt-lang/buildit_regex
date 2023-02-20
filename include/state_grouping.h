#ifndef STATE_GROUPING_H
#define STATE_GROUPING_H

#include "match.h"

void update_groups_from_cache(static_var<char>* next, int* cache, int p, int re_len, bool update);

dyn_var<int> match_regex_with_groups(const char* re, int* groups, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, bool enable_partial, int* cache, int match_index, int n_threads, int ignore_case);

#endif
