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
    // except for the 6th least bsignificant bit
    return (left <= c && c <= right) || ((left ^ 32) <= c && c <= (right ^ 32));
}

dyn_var<int> match_char(dyn_var<char> dyn_c, char static_c, bool ignore_case, bool escaped) {
    if (escaped) {
        // special classes
        if (static_c == 'd') // a digit
            return dyn_c >= '0' && dyn_c <= '9';
        else if (static_c == 'D') // not a digit
            return !(dyn_c >= '0' && dyn_c <= '9');
        else if (static_c == 'w') // a word character
            return (dyn_c >= 'a' && dyn_c <= 'z') || (dyn_c >= 'A' && dyn_c <= 'Z') || (dyn_c >= '0' && dyn_c <= '9') || dyn_c == '_';
        else if (static_c == 'W') // not a word character
            return !((dyn_c >= 'a' && dyn_c <= 'z') || (dyn_c >= 'A' && dyn_c <= 'Z') || (dyn_c >= '0' && dyn_c <= '9') || dyn_c == '_');
        else if (static_c == 's') // a space
            return (dyn_c == ' ');
        else if (static_c == 'S') //  not a space
            return dyn_c != ' ';
    }

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
bool update_from_cache(static_var<char[]>& next, int* cache, int p, int re_len, bool reverse, bool update, bool read_only) {
    if (!update)
        return false;
    int cache_size = (re_len + 1) * (re_len + 1);
    for (int i = 0; i < re_len + 1; i++) {
        int cache_idx = (reverse) ? ((i + 1) * (re_len + 1) + p) % cache_size : (p + 1) * (re_len + 1) + i;
        static_var<char> cache_val = cache[cache_idx];
        if (read_only) {
            if (cache_val && !next[i])
                return true;
        } else {
            next[i] = (cache_val != 0) ? (char)cache_val : next[i];
        }
    }
    return false;
}

bool check_state_updates(static_var<char[]>& next, int* cache, int p, int re_len) {
    for (int i = 0; i < re_len + 1; i++) {
        static_var<char> cache_val = cache[(p + 1) * (re_len + 1) + i];
        if (cache_val && !next[i]) {
            return true;
        }
    }
    return false;
}

int is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');  
}

/**
 Returns true if the character class starting at index `state` in `re`
 matches the character `c` in the input string. Otherwise returns false.
*/
bool match_class(dyn_var<char> c, const char* re, int state, bool ignore_case) {
    static_var<int> idx = state + 1;
    // ^ means we are looking for an inverse match
    static_var<int> inverse = 0;
    if ('^' == re[idx]) {
        inverse = 1;
        idx = idx + 1;
    }

    dyn_var<int> matches = 0;
    // check if c matches any of the chars in []
    while (re[idx] != ']') {
        if (re[idx] == '\\') {
            if (match_char(c, re[idx+1], ignore_case, true)) {
                return true ^ inverse;
            }
            idx = idx + 1;
        } else if (re[idx+1] == '-') {
            // this is used for ranges, e.g. [a-d]
            bool in_range = is_in_range(re[idx], re[idx+2], c, ignore_case);
            idx = idx + 2;
            if (in_range) {
                return true ^ inverse;
            }
        } else if (match_char(c, re[idx], ignore_case)) {
            // normal match
            return true ^ inverse;
        }
        idx = idx + 1;
    }
    return false ^ inverse;
}



