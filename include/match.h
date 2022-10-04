#include <iostream>
#include <fstream>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"

using builder::static_var;
using builder::dyn_var;

bool is_normal(char m);
bool is_digit(char m);
bool is_repetition_char(char c);
bool process_re(const char* re, int *next_states, int *prev_states, int *brackets);
void progress(const char *re, static_var<char> *next, int *ns_arr, int *prev_arr, int *brackets, int p);
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c);
dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len);
