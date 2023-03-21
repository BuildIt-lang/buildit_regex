#include "compile.h"

Schedule get_schedule_options(string regex, RegexOptions regex_options) {
    Schedule options;
    options.or_split = regex_options.flags.find("s") != string::npos;
    options.state_group = regex_options.flags.find("g") != string::npos;
    options.interleaving_parts = regex_options.interleaving_parts;
    options.ignore_case = regex_options.ignore_case;
    options.start_anchor = regex_options.start_anchor;
    options.last_eom = regex_options.last_eom;
    options.reverse = regex_options.reverse;
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

/**
 Generates code for the given regex accroding to the provided scheduling options.
*/
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

// convert the end of match index into a binary result (match or no match)
int eom_to_binary(int eom, int str_start, int str_len, MatchType match_type) {
    //cout << "eom: " << eom << endl;
    if (eom == (str_start - 1)) // no match
        return 0;
    else if (match_type == MatchType::PARTIAL_SINGLE)
        return 1;
    else
        return eom == str_len; // in case of full match eom has to be str_len
}

/**
 Compiles and calls the generated function. Returns if there is a match or not.
*/
int match(string regex, string str, RegexOptions options, MatchType match_type) {
    if (match_type == MatchType::FULL) {
        options.start_anchor = true;
        options.last_eom = true;
    } else if (match_type == MatchType::PARTIAL_SINGLE) {
        // just use the default values which are false    
    }
    
    
    vector<Matcher> funcs = compile(regex, options, match_type);
    const char* str_c = str.c_str();
    int str_len = str.length();

    if (options.interleaving_parts == 1)
        return eom_to_binary(funcs[0](str_c, str_len, 0), 0, str_len, match_type);

    // parallelize
    int result = 0;
    #pragma omp parallel for
    for (int part_id = 0; part_id < options.interleaving_parts; part_id++) {
        if (eom_to_binary(funcs[part_id](str_c, str_len, 0), 0, str_len, match_type)) {
            result = 1;
        }
    }
    
    return result;
}

/**
 Returns a list of the longest partial matches starting from each char in str.
*/
vector<string> get_all_partial_matches(string str, string regex, RegexOptions options) {
    // parse regex and cache state transitions
    tuple<string, string> parsed = expand_regex(regex);
    string parsed_re = get<0>(parsed);
    string flags = get<1>(parsed);
    const char* regex_cstr = parsed_re.c_str();
    int re_len = parsed_re.length();

    int cache_size = (re_len + 1) * (re_len + 1);
    int* cache = new int[cache_size];
    cache_states(regex_cstr, cache);

    builder::builder_context context;
    context.feature_unstructured = true;
    context.run_rce = true;

    Matcher func = (Matcher)builder::compile_function_with_context(context, get_partial_match, regex_cstr, cache, options.ignore_case);
    
    delete[] cache;

    // run anchored partial match starting from each position in the string
    vector<string> matches;
    const char* s = str.c_str();
    int str_len = str.length();
    
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

