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
void progress(const char *re, static_var<char> *next, int *ns_arr, int *brackets, int *helper_states, int p, static_var<int> *counters);
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c);
dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len);
bool process_re(const char *re, int *next_states, int *brackets, int *helper_states, static_var<int> *counters);
void get_counters(const char *re, static_var<int> idx, static_var<int> *counters);
void reset_counters(const char *re, int curr_idx, static_var<int> *counters);
