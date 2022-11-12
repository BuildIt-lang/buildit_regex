#include <cstring>
#include <vector>
#include <iostream>

using namespace std;

struct Cache {
    char* is_cached;
    int* next_states;
};

bool is_normal(char m);
bool is_digit(char m);
bool process_re(const char *re, int *next_states, int *brackets, int *helper_states);
void progress(const char *re, int *ns_arr, int *brackets, int *helper_states, int p, char *cache, int *cache_states, char *temp);
void cache_states(const char* re, Cache cache, int* ns, int* brackets, int* helper_states);
