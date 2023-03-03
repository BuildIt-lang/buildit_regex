#ifndef PARSE_H
#define PARSE_H

#include <iostream>
#include <tuple>
#include <vector>
#include <set>
#include <cstring>

using namespace std;

int get_counters(string re, int idx, int *counters);
string escape_char(char c);
string expand_regex(string re);
tuple<string, int> expand_sub_regex(string re, int start);
void get_or_positions(string &str, int* positions);
set<string> split_regex(string &str, int* or_positions, int start, int end);
void group_states(const char* re, int* groups);
void mark_or_groups(const char* re, int* groups, int* or_positions, int* next_state);
#endif
