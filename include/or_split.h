#ifndef OR_SPLIT_H
#define OR_SPLIT_H

#include "match.h"
#include <set>

dyn_var<int> split(dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> str_start, int start_state, std::set<int> &working_set, std::set<int> &done_set);

dyn_var<int> split_and_match(const char* re, int first_state, std::set<int> &working_set, std::set<int> &done_set, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, bool enable_partial, int* cache, int match_index, int n_threads, int ignore_case);

#endif
