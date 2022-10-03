#include "match.h"

/*
The code below is taken from https://intimeand.space/docs/CGO2022-BuilDSL.pdf
*/


/**
It returns whether `re` is a valid regular expression.
It updates `states` such that if a character is inside brackets
the next character we should look at in `progress` is just after the brackets.
If it's outside brackets then look at the char right after the current one.
*/
bool process_re(const char* re, int *states, int *brackets) {
    const int re_len = strlen(re);
    static_var<int> idx = re_len - 1;
    static_var<int> closed_paran = -1;
    while (idx >= 0) {
        if (re[idx] == ']') {
            static_var<int> s = idx + 1;
            while (re[idx] != '[') {
                // the current char is inside brackets
                // the next state is after the closing bracket
                states[idx] = s;
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
        } else if (idx < re_len - 1 && re[idx + 1] == ')') {
            if (re[idx] == '(') {
                printf("Cannot have empty parantheses!\n");
                return false;
            }
            closed_paran = idx + 1;
            // the char before ) should map to the char after the )
            states[idx] = idx + 2;
            idx = idx - 1;
            continue;
        } else if (re[idx] == '(') {
            brackets[closed_paran] = idx;
            brackets[idx] = closed_paran;
            closed_paran = -1;
        } else {
            // check for invalid characters
            // TODO: currently does not handle '.' inside brackets
            if (!(is_normal(re[idx]) || re[idx] == '*' || re[idx] == ')')) {
                printf("Invalid character: %c\n", (char)re[idx]);
                return false;
            }
        }
        // the next state is after the current char
        states[idx] = idx + 1;
        idx = idx - 1;
    }
    if (closed_paran != -1) {
        printf("Couldn't find a matching paranthesis!\n");
        return false;
    }
    return true;
}

bool is_normal(char m) {
    return (m >= 'a' && m <= 'z') || (m >= 'A' && m <= 'Z') || (m >= '0' && m <= '9');
}

dyn_var<int> is_in_range(char left, char right, dyn_var<char> c) {
    if (!(is_normal(left) && is_normal(right))) {
        printf("Invalid Characters %c %c\n", left, right);
        return 0; 
    }
    return left <= c && c <= right;
}

void progress(const char *re, static_var<char> *next, int *ns_arr, int *brackets, int p) {
    // unsigned int ns = p + 1;
    unsigned int ns = (unsigned int)ns_arr[p];
    if (strlen(re) == ns) {
        next[ns] = true;
    } else if (is_normal(re[ns]) || '.' == re[ns]) {
        next[ns] = true;
        if ('*' == re[ns+1]) {
        // We are allowed to skip this
        // so just progress again
        progress(re, next, ns_arr, brackets, ns+1);
        }
    } else if ('*' == re[ns]) {
        if (ns != 0 && re[ns-1] == ']') {
            // this * comes right after []
            // allow to match any of the chars from inside []
            static_var<int> curr_idx = brackets[ns-1];
            if (re[curr_idx+1] == '^') {
                // only mark ^ for negative classes
                next[curr_idx+1] = true;
            } else {
                // positive class - mark all chars from inside []
                curr_idx = curr_idx + 1;
                while (re[curr_idx] != ']') {
                    next[curr_idx] = true;
                    curr_idx = curr_idx + 1;
                }
            }
        } else if (ns != 0 && re[ns-1] == ')') {
            // there is a () group just before *
            // can match the char right after (
            next[brackets[ns-1]+1] = true;
        } else {
            // can match the char before *
            next[ns-1] = true;
        }
        progress(re, next, ns_arr, brackets, ns);
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
            progress(re, next, ns_arr, brackets, brackets[ns]+1);
    } else if ('(' == re[ns]) {
        // can  match the char after (
        next[ns + 1] = true;
        // if () are followed by *, it's possible to skip the () group
        if (brackets[ns] < (int)strlen(re) - 1 && '*' == re[brackets[ns]+1])
            progress(re, next, ns_arr, brackets, brackets[ns]+1);
    }

}

dyn_var<int> match_regex(const char* re, dyn_var<char*> str, dyn_var<int> str_len) {
    // allocate two state vectors
    const int re_len = strlen(re);
    //std::unique_ptr<static_var<char>> current_ptr(new static_var<char>[re_len + 1]);
    static_var<char> *current = new static_var<char>[re_len + 1];
    static_var<char> *next = new static_var<char>[re_len + 1];
    std::unique_ptr<int> progress_ns_ptr(new int[re_len]);
    int *progress_ns = progress_ns_ptr.get();
    std::unique_ptr<int> brackets_ptr(new int[re_len]);
    int *brackets = brackets_ptr.get(); // hold the opening and closing indices for each bracket pair
    bool re_valid = process_re(re, progress_ns, brackets);
    if (!re_valid) {
        printf("Invalid regex");
        return false;
    }

    for (static_var<int> i = 0; i < re_len + 1; i++)
        current[i] = next[i] = 0;
    progress(re, current, progress_ns, brackets, -1);
    // dyn_var<int> str_len = d_strlen(str);
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
                            progress(re, next, progress_ns, brackets, state);
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
                        progress(re, next, progress_ns, brackets, state);
                        state_match = 1;
                    }
                } else if ('.' == m) {
                    progress(re, next, progress_ns, brackets, state);
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
                            matches = !is_in_range(re[idx-1], re[idx+1], str[to_match]);
                            if (!matches)
                                break;
                        }
                        idx = idx + 1;
                    }
                    if (matches == 1) {
                        state_match = 1;
                        progress(re, next, progress_ns, brackets, state);
                    }
                } else if ('-' == m) {
                    static_var<char> left = re[state - 1];
                    static_var<char> right = re[state + 1];
                    if (is_in_range(left, right, str[to_match])) {
                        progress(re, next, progress_ns, brackets, state);
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

