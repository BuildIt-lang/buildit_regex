#ifndef ALL_PARTIAL
#define ALL_PARTIAL

#include "match.h"

dyn_var<int> get_partial_match(const char* re, dyn_var<char*> str, dyn_var<int> str_len, dyn_var<int> to_match, 
                            int* cache, int ignore_case);
#endif
