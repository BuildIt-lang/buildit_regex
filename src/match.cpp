#include "match.h"
#include <memory>

namespace std {
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}


/*
Parts of the code below are taken from https://intimeand.space/docs/CGO2022-BuilDSL.pdf
*/

/**
Returns if `c` is between the chars `left` and `right`.
*/
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c, int ignore_case) {
    if (!(is_normal(left) && is_normal(right))) {
        printf("Invalid Characters %c %c\n", left, right);
        return 0; 
    }
    if (!ignore_case || !is_alpha(left) || !is_alpha(right)) {
        return left <= c && c <= right;    
    }
    // ignore case: lowercase and uppercase letters are the same in binary 
    // except for the 6th least significant bit
    return (left <= c && c <= right) || ((left ^ 32) <= c && c <= (right ^ 32));
}

dyn_var<int> match_char(dyn_var<char> dyn_c, char static_c, bool ignore_case) {
    // upper and lower case letters differ only by the 5th bit
    bool is_upper = ('A' <= static_c && static_c <= 'Z');
    bool is_lower = ('a' <= static_c && static_c <= 'z');
    if (ignore_case && is_lower)
        return (dyn_c | 32) == static_c;
    else if (ignore_case && is_upper)
        return (dyn_c & ~32) == static_c;
    else
        return dyn_c == static_c;
}

/**
Update `next` with the reachable states from state `p`.
*/
void update_from_cache(static_var<char[]>& next, int* cache, int p, int re_len, bool update) {
    if (!update)
        return;
    for (int i = 0; i < re_len + 1; i++) {
        static_var<char> cache_val = cache[(p+1) * (re_len + 1) + i];
        next[i] = (cache_val != 0) ? (char)cache_val : next[i];
    }    
}

int is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');  
}

/**
 Returns true if the character class starting at index `state` in `re`
 matches the character `c` in the input string. Otherwise returns false.
*/
dyn_var<int> match_class(dyn_var<char> c, const char* re, int state, bool ignore_case) {
    static_var<int> idx = state + 1;
    // ^ means we are looking for an inverse match
    static_var<int> inverse = 0;
    if ('^' == re[idx]) {
        inverse = 1;
        idx = idx + 1;
    }
    // keep track of open brackets;
    // due to recursion, only match chars from the
    // top level [] i.e. if open == 1
    static_var<int> open = 1;

    dyn_var<int> matches = 0;
    // check if c matches any of the chars in []
    while (open > 0) {
        if (re[idx] == '[') {
            open++;
            // in case of nested [] keep matching recursively
            if (match_class(c, re, idx, ignore_case)) {
                matches = 1;
                break;
            }
        } else if (re[idx] == ']') {
            open--;    
        } else if (open == 1 && re[idx] == '-') {
            // this is used for ranges, e.g. [a-d]
            bool in_range = is_in_range(re[idx-1], re[idx+1], c, ignore_case);
            if (in_range) {
                matches = 1;
                break;
            }
        } else if (open == 1 && match_char(c, re[idx], ignore_case)) {
            // normal match
            matches = 1;
            break;
        }
        idx = idx + 1;
    }
    return (inverse == 1 && matches == 0) || (inverse == 0 && matches == 1);
}



