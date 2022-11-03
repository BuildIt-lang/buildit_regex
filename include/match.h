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
bool is_digit(char m);
void progress(const char *re, static_var<char> *next, int *ns_arr, int *brackets, int *helper_states, int p, char *cache, int *cache_states, bool use_cache, static_var<char> *temp);
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c);
dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool enable_partial, bool use_cache);
dyn_var<int> match_regex_full(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool use_cache);
dyn_var<int> match_regex_partial(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool use_cache);
bool process_re(const char *re, int *next_states, int *brackets, int *helper_states);
