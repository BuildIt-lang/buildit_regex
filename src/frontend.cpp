#include "frontend.h"

MatchFunction compile_regex(const char* regex, int* cache, MatchType match_type) {
    cache_states(regex, cache);
    // code generation
    builder::builder_context context;
    context.feature_unstructured = true;
    context.run_rce = true;
    bool partial = (match_type == MatchType::PARTIAL_SINGLE);
    return (MatchFunction)builder::compile_function_with_context(context, match_regex, regex, partial, cache);    
}

bool run_matcher(MatchFunction func, const char* str, int n_threads) {
    if (n_threads == 1)
        return func(str, strlen(str), 0);

    bool result = false;
    #pragma omp parallel for
    for (int tid = 0; tid < n_threads; tid++) {
        if (func(str, strlen(str), tid))
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
    // get a pointer to the generated function
    MatchFunction matcher = compile_regex(parsed_re.c_str(), cache.get(), match_type);
    if (match_type == MatchType::FULL)
        n_threads = 1;
    return run_matcher(matcher, str.c_str(), n_threads);
}


