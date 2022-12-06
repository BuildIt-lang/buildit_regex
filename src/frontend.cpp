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


