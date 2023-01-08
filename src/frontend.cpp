#include "frontend.h"

MatchFunction compile_regex(const char* regex, int* cache, MatchType match_type, int tid, int n_threads) {
    cache_states(regex, cache);
    // code generation
    builder::builder_context context;
    context.feature_unstructured = true;
    context.run_rce = true;
    bool partial = (match_type == MatchType::PARTIAL_SINGLE);
    return (MatchFunction)builder::compile_function_with_context(context, match_regex, regex, partial, cache, tid, n_threads);    
}

bool run_matcher(MatchFunction* funcs, const char* str, int n_threads) {
    if (n_threads == 1) {
        return funcs[0](str, strlen(str));
    }

    bool result = false;
    #pragma omp parallel for
    for (int tid = 0; tid < n_threads; tid++) {
        if (funcs[tid](str, strlen(str)))
            result = true;
    }
    return result;

}

int compile_and_run(string str, string regex, MatchType match_type, int n_threads) {
    // simplify the regex
    string parsed_re = expand_regex(regex);
    int re_len = parsed_re.length();
    const int cache_size = (re_len + 1) * (re_len + 1);
    std::unique_ptr<int> cache(new int[cache_size]);
    
    MatchFunction* funcs = new MatchFunction[n_threads];
    for (int tid = 0; tid < n_threads; tid++) {    
        MatchFunction func = compile_regex(parsed_re.c_str(), cache.get(), match_type, tid, n_threads);
        funcs[tid] = func;
    }
    return run_matcher(funcs, str.c_str(), n_threads);
}


int compile_and_run_decomposed(string str, string regex, MatchType match_type, int n_threads) {
    // simplify the regex and decompose it into parts based on | chars
    string parsed_re = expand_regex(regex);
    int* or_positions = new int[parsed_re.length()];
    get_or_positions(parsed_re, or_positions);
    set<string> regex_parts = split_regex(parsed_re, or_positions, 0, parsed_re.length() - 1);
    delete[] or_positions;
    if (regex_parts.size() == 0) // regex is an empty string
        return true;
    // if any of the parts matches, return true
    for (string part: regex_parts) {
        if (compile_and_run(str, part, match_type, 1))
            return true;
    }
    return false;
}
