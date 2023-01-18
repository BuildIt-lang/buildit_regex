#ifndef MATCH_H
#define MATCH_H

#include <iostream>
#include <fstream>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include <stdlib.h>
#include <vector>

using builder::static_var;
using builder::dyn_var;
using namespace std;

bool is_normal(char m);
int is_alpha(char c);
dyn_var<int> is_equal(dyn_var<char> c1, char c2, int ignore_case);
void update_from_cache(static_var<char>* next, int* cache, int p, int re_len);
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c, int ignore_case);
dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, bool enable_partial, int* cache_states, int match_index, int n_threads, int ignore_case);
dyn_var<int> match_regex_full(const char* re, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> start_idx, int* cache_states, int ignore_case);
dyn_var<int> match_regex_partial(const char* re, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> start_idx, int* cache_states, int ignore_case);

#endif
