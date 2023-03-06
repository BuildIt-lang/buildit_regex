#include "compile.h"

Schedule get_schedule_options(string regex, RegexOptions regex_options) {
    Schedule options;
    options.or_split = regex_options.flags.find("s") != string::npos;
    options.state_group = regex_options.flags.find("g") != string::npos;
    options.interleaving_parts = regex_options.interleaving_parts;
    options.ignore_case = regex_options.ignore_case;
    return options;
}

pair<Matcher, int> compile(string regex, RegexOptions options, MatchType match_type) {
    // regex preprocessing
    tuple<string, string> parsed = expand_regex(regex, options.flags);
    string parsed_regex = get<0>(parsed);
    string parsed_flags = get<1>(parsed);
    const char* regex_cstr = parsed_regex.c_str();
    int re_len = parsed_regex.length();
    
    // get schedule
    Schedule schedule = get_schedule_options(regex, options);
    bool partial = (match_type == MatchType::PARTIAL_SINGLE);
    
    // precompute state transitions
    // mark grouped states and or groups
    int cache_size = (re_len + 1) * (re_len + 1);
    int* cache = new int[cache_size];
    cache_states(regex_cstr, cache);

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
        auto ast = context.extract_function_ast(match_with_schedule, fname, regex_cstr, first_state, working_set, done_set, partial, cache, 0, schedule, parsed_flags.c_str());
        functions.push_back(ast);
    }
    
    // generate all functions
    builder::builder_context ctx;
    Matcher func = (Matcher)builder::compile_asts(ctx, functions, "match_0");
    delete[] cache;

    return make_pair(func, re_len);    
}

int match(string regex, string str, RegexOptions options, MatchType match_type) {
    pair<Matcher, int> matcher = compile(regex, options, match_type);
    int re_len = matcher.second;
    Matcher func = matcher.first;
    
    char* dyn_current = new char[re_len+1];
    char* dyn_next = new char[re_len+1];
    int result = func(str.c_str(), str.length(), 0, dyn_current, dyn_next);
    
    delete[] dyn_current;
    delete[] dyn_next;
    return result;
}
