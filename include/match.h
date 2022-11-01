#include <iostream>
#include <fstream>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include <stdlib.h>
#include "types.h"
#include "runtime.h"

using builder::static_var;
using builder::dyn_var;
using namespace std;

bool is_normal(char m);
bool is_digit(char m);
void progress(const char *re, static_var<char> *next, int *ns_arr, int *brackets, int *helper_states, int p, std::set<int>* new_states);
void update_start_positions(dyn_var<set_t<int>*> next_start, dyn_var<set_t<int>*> current_start, dyn_var<int> to_match, int state, std::set<int> next_states, int re_len);
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c);
dyn_var<map_t<int, set_t<int>>> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len, bool enable_partial);
dyn_var<int> match_regex_full(const char* re, dyn_var<char*> str, dyn_var<int> str_len);
dyn_var<int> match_regex_partial(const char* re, dyn_var<char*> str, dyn_var<int> str_len);
bool process_re(const char *re, int *next_states, int *brackets, int *helper_states);
dyn_var<map_t<int, set_t<int>>> find_all_matches(const char* re, dyn_var<char*> str, dyn_var<int> str_len);
