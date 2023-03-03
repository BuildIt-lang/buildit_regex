#include "compile.h"

Schedule get_schedule_options(string regex, RegexOptions regex_options) {
    Schedule options;
    options.or_split = regex.find("(?S") != string::npos;
    options.state_group = regex.find("(?G") != string::npos;
    options.interleaving_parts = regex_options.interleaving_parts;
    options.ignore_case = regex_options.ignore_case;
    return options;
}

pair<Matcher, int> compile(string regex, RegexOptions flags, MatchType match_type) {
    // regex preprocessing
    string parsed_regex = expand_regex(regex);
    const char* regex_cstr = parsed_regex.c_str();
    int re_len = parsed_regex.length();
    
    // get schedule
    Schedule schedule = get_schedule_options(regex, flags);
    bool partial = (match_type == MatchType::PARTIAL_SINGLE);
    
    // precompute state transitions
    // mark grouped states and or groups
    int cache_size = (re_len + 1) * (re_len + 1);
    int* cache = new int[cache_size];
    int* groups = new int[re_len];
    cache_states(regex_cstr, cache, groups);

    // data for or_split
    set<int> working_set, done_set;
    vector<block::block::Ptr> functions;
    working_set.insert(0);
    
    while (!working_set.empty()) {
        int first_state = *working_set.begin();
        working_set.erase(first_state);
        done_set.insert(first_state);
        string fname = "match_" + to_string(first_state);
        // define context
        builder::builder_context context;
        context.feature_unstructured = true;
        context.run_rce = true;
        auto ast = context.extract_function_ast(match_with_schedule, fname, regex_cstr, first_state, working_set, done_set, partial, cache, 0, schedule, groups);
        functions.push_back(ast);
    }
    
    // generate all functions
    builder::builder_context ctx;
    Matcher func = (Matcher)builder::compile_asts(ctx, functions, "match_0");
    delete[] cache;
    delete[] groups;

    return make_pair(func, re_len);    
}

int match(string regex, string str, RegexOptions flags, MatchType match_type) {
    pair<Matcher, int> matcher = compile(regex, flags, match_type);
    int re_len = matcher.second;
    Matcher func = matcher.first;
    
    char* dyn_current = new char[re_len+1];
    char* dyn_next = new char[re_len+1];
    int result = func(str.c_str(), str.length(), 0, dyn_current, dyn_next);
    
    delete[] dyn_current;
    delete[] dyn_next;
    return result;
}
