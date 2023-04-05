#ifndef MATCH_H
#define MATCH_H

#include <iostream>
#include <fstream>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include "builder/lib/utils.h"
#include <stdlib.h>
#include <vector>

using builder::static_var;
using builder::dyn_var;
using namespace std;

bool is_normal(char m);
int is_alpha(char c);
dyn_var<int> is_equal(dyn_var<char> c1, char c2, int ignore_case);
dyn_var<int> match_char(dyn_var<char> dyn_c, char static_c, bool ignore_case, bool escaped=false);
bool match_class(dyn_var<char> c, const char* re, int state, bool ignore_case);
bool update_from_cache(static_var<char[]>& next, int* cache, int p, int re_len, bool reverse, bool update, bool read_only=false);
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c, int ignore_case);
bool check_state_updates(static_var<char[]>& next, int* cache, int p, int re_len);

#endif
