#include "match.h"

/*
Parts of the code below are taken from https://intimeand.space/docs/CGO2022-BuilDSL.pdf
*/


/**
It returns whether `re` is a valid regular expression.

It updates `states` such that if a character is inside []
the next character we should look at in `progress` is just after the brackets.

If a char is inside () the next char we look at should be the char right after
the current one, except for the char that's just before ) - in that case we look
at the character right after )

If it's outside [] and () then look at the char right after the current one.

`brackets` is an array of the same length as `re` that holds the index
of a closing ( or [ for each opening one, and vice versa. 
*/
bool process_re(const char* re, int *next_states, int *prev_states, int *brackets, int *ors) {
    const int re_len = strlen(re);
    static_var<int> idx = re_len - 1;
    static_var<int> closed_paran = -1;
    static_var<int> or_count = 0;
    while (idx >= 0) {
        // by default assume that the char before * or {} is a normal char
        if (is_repetition_char(re[idx])) {
            prev_states[idx] = idx - 1;    
        }
        if (re[idx] == ')') closed_paran = idx;
        if (re[idx] == ']') {
            static_var<int> s = (re[idx+1] == '|' || re[idx+1] == ')') ? closed_paran + 1 : idx + 1;
            while (re[idx] != '[') {
                // the current char is inside brackets
                // the next state is after the closing bracket
                next_states[idx] = s;
                idx = idx - 1;
                // couldn't find a closing bracket => invalid regex
                if (idx < 0) {
                    printf("Couldn't find a matching bracket!\n");
                    return false;
                }
            }
            // mark the opening and closing brackets' indices
            brackets[idx] = s - 1;
            brackets[s - 1] = idx;
            if (is_repetition_char(re[s])) {
                prev_states[s] = idx;
            }
        } else if (idx < re_len - 1 && re[idx + 1] == ')') {
            if (re[idx] == '(') {
                printf("Cannot have empty parantheses!\n");
                return false;
            }
            // the char before ) should map to the char after the )
            next_states[idx] = idx + 2;
            idx = idx - 1;
            continue;
        } else if (re[idx] == '(') {
            brackets[closed_paran] = idx;
            brackets[idx] = closed_paran;
            if (is_repetition_char(re[closed_paran+1])) {
                prev_states[closed_paran+1] = idx;
            }
            ors[closed_paran] = (int)or_count;
            closed_paran = -1;
            or_count = 0;
        } else {
            // check for invalid characters
            // TODO: currently does not handle '.' inside brackets
            if (!(is_normal(re[idx]) || re[idx] == '|' || re[idx] == '*' || re[idx] == ')')) {
                printf("Invalid character: %c\n", (char)re[idx]);
                return false;
            }
        }
        
        if (closed_paran != -1) {
            // look for | inside ()
            if (re[idx+1] == '|') {
                next_states[idx] = closed_paran + 1;
                ors[closed_paran - 1 - or_count] = idx + 1;
                or_count = or_count + 1;
                idx = idx - 1;
                continue;
            }
        }
        // the next state is after the current char
        next_states[idx] = idx + 1;
        idx = idx - 1;
    }
    if (closed_paran != -1) {
        printf("Couldn't find a matching paranthesis!\n");
        return false;
    }
    /*printf("------\n");
    for (static_var<int> i = 0; i < re_len; i = i + 1) {
        printf("%d, ", next_states[i]);    
    }
    printf("\n-----\n");*/
    return true;
}

/**
Returns if `m` is an alphanumeric character.
*/
bool is_normal(char m) {
    return (m >= 'a' && m <= 'z') || (m >= 'A' && m <= 'Z') || (m >= '0' && m <= '9');
}

bool is_digit(char m) {
    return m >= '0' && m <= '9';
}

bool is_repetition_char(char c) {
    return c == '*' || c == '{';    
}

/**
Returna if `c` is between the chars `left` and `right`.
*/
dyn_var<int> is_in_range(char left, char right, dyn_var<char> c) {
    if (!(is_normal(left) && is_normal(right))) {
        printf("Invalid Characters %c %c\n", left, right);
        return 0; 
    }
    return left <= c && c <= right;
}

/**
Given that the character `re[p]` has just been matched, finds all the characters
in `re` that can be matched next and sets their corresponding locations in `next` to `true`.
*/
void progress(const char *re, static_var<char> *next, int *ns_arr, int *prev_arr, int *brackets, int *ors, int p) {
    // unsigned int ns = p + 1;
    unsigned int ns = (unsigned int)ns_arr[p];
    if (strlen(re) == ns) {
        next[ns] = true;
    } else if (is_normal(re[ns]) || '.' == re[ns]) {
        next[ns] = true;
        if ('*' == re[ns+1] || '?' == re[ns+1]) {
            // we can also skip this char
            progress(re, next, ns_arr, prev_arr, brackets, ors, ns+1);
        }
    } else if ('*' == re[ns] || '+' == re[ns]) { // can match char p again
        int prev_state = prev_arr[ns];
        progress(re, next, ns_arr, prev_arr, brackets, ors, prev_state-1);
    } else if ('[' == re[ns]) {
        static_var<int> curr_idx = ns + 1;
        if (re[ns + 1] == '^') {
            // negative class - mark only '^' as true
            // the character matching is handled in `match_regex`
            next[ns + 1] = true;
        } else {
            while (re[curr_idx] != ']') {
                // allowed to match any of the chars inside []
                next[curr_idx] = true;
                curr_idx = curr_idx + 1;
            }
        }
        if (brackets[ns] < (int)strlen(re) - 1 && '*' == re[brackets[ns]+1])
            // allowed to skip []
            progress(re, next, ns_arr, prev_arr, brackets, ors, brackets[ns]+1);
    } else if ('(' == re[ns]) {
        progress(re, next, ns_arr, prev_arr, brackets, ors, ns); // char right after (
        // start by trying to match the first char after each |
        for (static_var<int> k = 0; k < ors[brackets[ns]]; k = k + 1) {
            progress(re, next, ns_arr, prev_arr, brackets, ors, ors[brackets[ns]-1-k]);
        }
        // if () are followed by *, it's possible to skip the () group
        if (brackets[ns] < (int)strlen(re) - 1 && '*' == re[brackets[ns]+1])
            progress(re, next, ns_arr, prev_arr, brackets, ors, brackets[ns]+1);
    } 
}

/**
Tries to match each character in `str` one by one.
It relies on `progress` to get the possible states we can transition to
from the current state.
*/
dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len) {
    // allocate two state vectors
    const int re_len = strlen(re);
    static_var<char> *current = new static_var<char>[re_len + 1];
    static_var<char> *next = new static_var<char>[re_len + 1];
    std::unique_ptr<int> next_state_ptr(new int[re_len]);
    int *next_state = next_state_ptr.get();
    std::unique_ptr<int> prev_state_ptr(new int[re_len]);
    int *prev_state = prev_state_ptr.get();
    std::unique_ptr<int> brackets_ptr(new int[re_len]);
    int *brackets = brackets_ptr.get(); // hold the opening and closing indices for each bracket pair
    std::unique_ptr<int> ors_ptr(new int[re_len]);
    int *ors = ors_ptr.get();
    bool re_valid = process_re(re, next_state, prev_state, brackets, ors);
    if (!re_valid) {
        printf("Invalid regex");
        return false;
    }

    for (static_var<int> i = 0; i < re_len + 1; i++)
        current[i] = next[i] = 0;
    progress(re, current, next_state, prev_state, brackets, ors, -1);
    dyn_var<int> to_match = 0;
    while (to_match < str_len) {
        // Don’t do anything for $.
        static_var<int> early_break = -1;
        static_var<int> open_bracket = 0;
        static_var<int> bracket_match = 0;
        for (static_var<int> state = 0; state < re_len; ++state) {
            // flags for early skipping in case of a bracket match
            if (re[state] == '[') open_bracket = 1;
            else if (re[state] == ']') open_bracket = bracket_match = 0;
            // we are still inside [], but we already found a match
            // => skip iters up to the closing bracket
            if (bracket_match == 1) continue;
            // check if there is a match for this state
            static_var<int> state_match = 0;
            if (current[state]) {
                static_var<char> m = re[state];
                if (is_normal(m)) {
                    if (-1 == early_break) {
                        // Normal character
                        if (str[to_match] == m) {
                            progress(re, next, next_state, prev_state, brackets, ors, state);
                            // If a match happens, it
                            // cannot match anything else
                            // Setting early break
                            // avoids unnecessary checks
                            early_break = m;
                            state_match = 1;
                        }
                    } else if (early_break == m) {
                        // The comparison has been done
                        // already, let us not repeat
                        progress(re, next, next_state, prev_state, brackets, ors, state);
                        state_match = 1;
                    }
                } else if ('.' == m) {
                    progress(re, next, next_state, prev_state, brackets, ors, state);
                    state_match = 1;
                } else if ('^' == m) {
                    // we are inside a [^] class
                    static_var<int> idx = state + 1;
                    dyn_var<int> matches = 1;
                    // check if str[to_match] matches any of the chars in []
                    while (re[idx] != ']') {
                        if (re[idx] == str[to_match]) {
                            matches = 0;
                            break;
                        } else if (re[idx] == '-') {
                            // this is used for ranges, e.g. [a-d]
                            matches = !is_in_range(re[idx-1], re[idx+1], str[to_match]);
                            if (!matches)
                                break;
                        }
                        idx = idx + 1;
                    }
                    if (matches == 1) {
                        state_match = 1;
                        progress(re, next, next_state, prev_state, brackets, ors, state);
                    }
                } else if ('-' == m) {
                    static_var<char> left = re[state - 1];
                    static_var<char> right = re[state + 1];
                    if (is_in_range(left, right, str[to_match])) {
                        progress(re, next, next_state, prev_state, brackets, ors, state);
                        state_match = 1;
                    }
                } else {
                    printf("Invalid Character(%c)\n", (char)m);
                    return false;
                }

                if (state_match == 1 && open_bracket == 1) bracket_match = 1;
            }
        }
        // All the states have been checked
        // Now swap the states and clear next
        static_var<int> count = 0;
        for (static_var<int> i = 0; i < re_len + 1; i++) {
            current[i] = next[i];
            next[i] = false;
            if (current[i])
                count++;
        }
        if (count == 0)
            return false;
        to_match = to_match + 1;
    }
    // Now that the string is done,
    // we should have $ in the state
    static_var<int> is_match = (char)current[re_len];
    for (static_var<int> i = 0; i < re_len + 1; i++) {
        next[i] = 0;
        current[i] = 0;
    }
    return is_match;
}

