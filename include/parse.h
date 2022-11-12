#include <iostream>
#include <tuple>

using namespace std;

int get_counters(string re, int idx, int *counters);
string expand_regex(string re);
tuple<string, int> expand_sub_regex(string re, int start);
