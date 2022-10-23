#include <iostream>
#include <tuple>
#include "builder/static_var.h"

using builder::static_var;
using namespace std;

int get_counters(string re, int idx, int *counters);
string expand_regex(string re);
tuple<string, int> expand_sub_regex(string re, int start);
