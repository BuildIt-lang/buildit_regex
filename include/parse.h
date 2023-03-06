#ifndef PARSE_H
#define PARSE_H

#include <iostream>
#include <tuple>
#include <vector>
#include <set>
#include <cstring>
#include <assert.h>

using namespace std;

int get_counters(string re, int idx, int *counters);
string escape_char(char c);
tuple<string, string> expand_regex(string re, string flags = "");
tuple<string, int, string> expand_sub_regex(string re, int start, string flags);
void get_or_positions(string &str, int* positions);
set<string> split_regex(string &str, int* or_positions, int start, int end);
#endif
