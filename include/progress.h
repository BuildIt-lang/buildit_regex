#ifndef PROGRESS_H
#define PROGRESS_H

#include <cstring>
#include <vector>
#include <iostream>
#include "parse.h"

using namespace std;

struct Cache {
    char* is_cached;
    int* next_states;
    char* temp_states;
};

struct ReStates {
    int* next;
    int* brackets;
    int* helper_states;
};

bool is_normal(char m);
bool is_digit(char m);
bool is_special_group(const char* re, int idx);
bool is_group_type(const char* re, int idx, char type);
void reset_array(char* arr, int len);
bool process_re(const char *re, ReStates re_states);
void progress(const char *re, ReStates re_states, int p, Cache cache, bool or_group);
void cache_states(const char* re, int* next_states, int* flags = nullptr);

#endif
