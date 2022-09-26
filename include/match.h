#include <iostream>
#include <fstream>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"

using builder::static_var;
using builder::dyn_var;

bool is_normal(char m);
void progress(const char *re, static_var<char> *next, int p);
dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len);
