#include <iostream>
#include <tuple>
#include "builder/static_var.h"

using builder::static_var;
using namespace std;

tuple<string, int> expand_regex(string re, int start);
