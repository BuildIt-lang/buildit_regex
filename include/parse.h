#ifndef PARSE_H
#define PARSE_H

#include <iostream>
#include <tuple>
#include <vector>
#include <set>
#include <cstring>
#include <assert.h>

using namespace std;

bool is_special_internal(char c);
bool is_special_external(char c);
bool is_normal(char c);
bool is_digit(char c);
bool is_letter(char c);
bool is_word_char(char c);
bool never_active(char c);
int get_counters(string re, int idx, int *counters);
string escape_char(char c);
tuple<string, string> expand_regex(string re, string flags = "");
tuple<string, int, string> expand_sub_regex(string re, int start, string flags);
void get_or_positions(string &str, int* positions);
set<string> split_regex(string &str, int* or_positions, int start, int end);
tuple<char, int> convert_to_hx(string re, int start);
bool is_hx_symbol(char c);
char hx_to_int(char c);
#endif
