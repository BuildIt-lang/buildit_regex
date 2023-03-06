#include "frontend.h"

/**
 Generates a matching function for the given regex.
*/
MatchFunction compile_regex(const char* regex, int* cache, MatchType match_type, int tid, int n_threads, string flags) {
    cache_states(regex, cache);
    // code generation
    builder::builder_context context;
    context.feature_unstructured = true;
    context.run_rce = true;
    bool partial = (match_type == MatchType::PARTIAL_SINGLE);
    int ignore_case = flags.compare("i") == 0;
    // all partial matches
    if (match_type == MatchType::PARTIAL_ALL) {
        return (MatchFunction)builder::compile_function_with_context(context, get_partial_match, regex, cache, ignore_case);    
    }
    // full or partial_single matching
    return (MatchFunction)builder::compile_function_with_context(context, match_regex, regex, partial, cache, tid, n_threads, ignore_case);    
}

/**
 Runs the generated matching functions on the input string.
*/
bool run_matcher(MatchFunction* funcs, const char* str, int n_threads) {
    if (n_threads == 1) {
        return funcs[0](str, strlen(str), 0);
    }

    bool result = false;
    #pragma omp parallel for
    for (int tid = 0; tid < n_threads; tid++) {
        if (funcs[tid](str, strlen(str), 0))
            result = true;
    }
    return result;

}

/**
 Default matching function for FULL and PARTIAL_SINGLE match types.
*/
int compile_and_run(string str, string regex, MatchType match_type, int n_threads, string flags) {
    // simplify the regex
    tuple<string, string> parsed = expand_regex(regex);
    string parsed_re = get<0>(parsed);
    int re_len = parsed_re.length();
    const int cache_size = (re_len + 1) * (re_len + 1);
    std::unique_ptr<int> cache(new int[cache_size]);

    MatchFunction* funcs = new MatchFunction[n_threads];
    for (int tid = 0; tid < n_threads; tid++) {    
        MatchFunction func = compile_regex(parsed_re.c_str(), cache.get(), match_type, tid, n_threads, flags);
        funcs[tid] = func;
    }
    
    return run_matcher(funcs, str.c_str(), n_threads);
}

/**
 Partial matching of PARTIAL_SINGLE type where the regex is split
 into multiple sub-regexes according to | chars. Usually results in
 a better regex compilation time compared to the default matching method.
*/
int compile_and_run_decomposed(string str, string regex, MatchType match_type, int n_threads, string flags) {
    // simplify the regex and decompose it into parts based on | chars
    tuple<string, string> parsed = expand_regex(regex);
    string parsed_re = get<0>(parsed);
    int* or_positions = new int[parsed_re.length()];
    get_or_positions(parsed_re, or_positions);
    set<string> regex_parts = split_regex(parsed_re, or_positions, 0, parsed_re.length() - 1);
    delete[] or_positions;
    if (regex_parts.size() == 0) // regex is an empty string
        return true;
    // if any of the parts matches, return true
    for (string part: regex_parts) {
        if (compile_and_run(str, part, match_type, 1, flags))
            return true;
    }
    return false;
}

/**
 A simple version of partial matching of PARTIAL_SINGLE type.
 Starting from each position in `str` it tries a full match
 of "regex.*" and returns true as soon as a match is found.
*/
int compile_and_run_partial(string str, string regex, string flags) {
   if (str.length() == 0 && regex.length() == 0)
       return 1;
    // simplify the regex
    regex = regex + ".*";
    tuple<string, string> parsed = expand_regex(regex);
    string parsed_re = get<0>(parsed);
    const char* re = parsed_re.c_str();
    int re_len = parsed_re.length();
    const int cache_size = (re_len + 1) * (re_len + 1);
    std::unique_ptr<int> cache(new int[cache_size]);
    int* cache_ptr = cache.get();
    // compile
    cache_states(re, cache_ptr);
    builder::builder_context context;
    context.feature_unstructured = true;
    context.run_rce = true;
    int ignore_case = flags.compare("i") == 0;
    MatchFunction func = (MatchFunction)builder::compile_function_with_context(context, match_regex, re, 0, cache_ptr, 0, 1, ignore_case); 
    return partial_match_loop(str.c_str(), str.length(), 1, func);
}

int partial_match_loop(const char* str, int str_len, int stride, MatchFunction func) {
    int result = 0;
    #pragma omp parallel for
    for (int i = 0; i < stride; i++) {
        for (int j = i; j < str_len; j += stride) {
            if (func(str, str_len, j)) {
                result = 1;
                break;
            }
        }    
    }
    return result;
}


/**
 Returns a list of the longest partial matches starting from each char in str.
*/
vector<string> get_all_partial_matches(string str, string regex, string flags) {
    // parse regex and cache state transitions
    tuple<string, string> parsed = expand_regex(regex);
    string parsed_re = get<0>(parsed);
    int re_len = parsed_re.length();
    const int cache_size = (re_len + 1) * (re_len + 1);
    std::unique_ptr<int> cache(new int[cache_size]);
    // compile a function for anchored matching
    MatchFunction func = compile_regex(parsed_re.c_str(), cache.get(), MatchType::PARTIAL_ALL, 0, 1, flags);
    
    // run anchored partial match starting from each position in the string
    vector<string> matches;
    const char* s = str.c_str();
    int str_len = str.length();
    //for (int i = 0; i < str_len; i++) {
    int i = 0;
    while (i < str_len) {
        int end_idx = func(s, str_len, i);
        // don't include empty matches
        if (end_idx != -1 && i != end_idx) {
            matches.push_back(str.substr(i, end_idx - i));
            i = end_idx;
        } else {
            i++;    
        }
       
    }
    return matches;
    
}

