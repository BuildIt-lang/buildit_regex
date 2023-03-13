#include "compile.h"

Schedule get_schedule_options(string regex, RegexOptions regex_options) {
    Schedule options;
    options.or_split = regex_options.flags.find("s") != string::npos;
    options.state_group = regex_options.flags.find("g") != string::npos;
    options.interleaving_parts = regex_options.interleaving_parts;
    options.ignore_case = regex_options.ignore_case;
    return options;
}

Matcher compile_helper(const char* regex, const char* flags, bool partial, int* cache, int part_id, Schedule schedule, string headers) {
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
        auto ast = context.extract_function_ast(match_with_schedule, fname, regex, first_state, working_set, done_set, partial, cache, part_id, schedule, flags);
        functions.push_back(ast);
    }
    
    // generate all functions
    builder::builder_context ctx;
    ctx.run_rce = true;
    ctx.dynamic_header_includes = headers;
    Matcher func = (Matcher)builder::compile_asts(ctx, functions, "match_0");
    return func;
}

vector<Matcher> compile(string regex, RegexOptions options, MatchType match_type) {
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

    string headers = "// regex: " + regex + "\n";
    string mt = (match_type == MatchType::FULL) ? "full" : "partial";
    headers += "// match type: " + mt + "\n"; 
    headers += "// config: (interleaving_parts: " + to_string(options.interleaving_parts) + "), (ignore_case: " + to_string(options.ignore_case) + "), (flags: " + options.flags + ")\n";

    vector<Matcher> funcs;
    for (int part_id = 0; part_id < schedule.interleaving_parts; part_id++) {
        Matcher func = compile_helper(parsed_regex.c_str(), parsed_flags.c_str(), partial, cache, part_id, schedule, headers);
        funcs.push_back(func);
    }
    
    delete[] cache;

    return funcs;    
}


int match(string regex, string str, RegexOptions options, MatchType match_type) {
    vector<Matcher> funcs = compile(regex, options, match_type);
    const char* str_c = str.c_str();
    int str_len = str.length();

    if (options.interleaving_parts == 1)
        return funcs[0](str_c, str_len, 0);

    // parallelize
    int result = 0;
    #pragma omp parallel for
    for (int part_id = 0; part_id < options.interleaving_parts; part_id++) {
        if (funcs[part_id](str_c, str_len, 0)) {
            result = 1;
        }
    }
    
    return result;
}
