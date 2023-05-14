#include "compile.h"

Schedule get_schedule_options(string regex, RegexOptions regex_options, MatchType match_type, int pass) {
    Schedule options;
    options.or_split = regex_options.flags.find("s") != string::npos;
    options.state_group = regex_options.flags.find("g") != string::npos;
    options.interleaving_parts = regex_options.interleaving_parts;
    options.ignore_case = regex_options.ignore_case;
    options.dotall = regex_options.dotall;
    options.block_size = regex_options.block_size;
    if (match_type == MatchType::FULL) {
        options.start_anchor = true;
        options.last_eom = true;
        options.reverse = false;
    } else if (match_type == MatchType::PARTIAL_SINGLE) {
        if (regex.length() > 0 && regex[0] == '^') {
            // anchor at the start of the string - we need only one pass
            options.start_anchor = true;
            options.last_eom = regex_options.greedy;
            options.reverse = false;
        } else if (regex_options.binary) {
            // any match is fine
            options.start_anchor = false;
            options.last_eom = false;
            options.reverse = false;
        } else if (regex_options.greedy) {
            // look for the first longest match
            options.start_anchor = (pass == 1); // true if second pass
            options.last_eom = true; // last match in both passes
            options.reverse = (pass == 0); // reverse if first pass
        } else {
            // lazy matching, return the first found match
            options.start_anchor = (pass == 1); // anchor in second pass
            options.last_eom = (pass == 0);
            options.reverse = (pass == 0); // reverse in the first pass
        }
    }
    return options;
}

Matcher compile_helper(const char* regex, const char* flags, bool partial, int* cache, int part_id, Schedule schedule, string headers, int start_state) {
    // data for or_split
    set<int> working_set, done_set;
    vector<block::block::Ptr> functions;
    working_set.insert(start_state);
    
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
    Matcher func = (Matcher)builder::compile_asts(ctx, functions, "match_" + to_string(start_state));
    return func;
}

// generate a comment to add at the top of the generated code file
string generate_headers(string regex, MatchType match_type, RegexOptions options) {
    
    //string headers = (regex.find('\n') == std::string::npos) ? "// regex: " + regex + "\n" : "";
    string headers = "";
    string mt = (match_type == MatchType::FULL) ? "full" : "partial";
    headers += "// match type: " + mt + "\n"; 
    headers += "// config: (interleaving_parts: " + to_string(options.interleaving_parts) + "), (ignore_case: " + to_string(options.ignore_case) + "), (flags: " + options.flags +  ")\n";
    headers += "#include <stdio.h>\n#include <string.h>\n";
    //headers += "int memcmp ( const void * ptr1, const void * ptr2, int num );\n";
    return headers;
}

vector<Matcher> compile_single_pass(string orig_regex, string regex, RegexOptions options, MatchType match_type, int pass, string flags, int* cache) {
    
    Schedule schedule = get_schedule_options(orig_regex, options, match_type, pass);
    
    int re_len = regex.length();
    int start_state = (schedule.reverse) ? re_len + 1 : 0;
    
    string headers = generate_headers(regex, match_type, options);

    bool partial = (match_type == MatchType::PARTIAL_SINGLE);

    vector<Matcher> funcs;
    for (int part_id = 0; part_id < schedule.interleaving_parts; part_id++) {
        Matcher func = compile_helper(regex.c_str(), flags.c_str(), partial, cache, part_id, schedule, headers, start_state);
        funcs.push_back(func);
    }
    return funcs;
    
}

void print_cache(int* cache, int re_len) {
    cout << "cache: " << endl;
    for (int i = 0; i < re_len + 1; i++) {
        for (int j = 0; j < re_len + 1; j++) {
            cout << cache[i * (re_len + 1) + j] << " ";    
        }
        cout << endl;
    }
    cout << endl;
}

/**
 Generates code for the given regex accroding to the provided scheduling options.
*/
tuple<vector<Matcher>, vector<Matcher>> compile(string regex, RegexOptions options, MatchType match_type, bool two_pass) {
    // regex preprocessing
    tuple<string, string> parsed = expand_regex(regex, options.flags);
    string parsed_regex = get<0>(parsed);
    string parsed_flags = get<1>(parsed);
    const char* regex_cstr = parsed_regex.c_str();
    int re_len = parsed_regex.length();
        
    // precompute state transitions
    // mark grouped states and or groups
    int cache_size = (re_len + 1) * (re_len + 1);
    int* cache = new int[cache_size];
    cache_states(regex_cstr, cache);
    
    //print_cache(cache, re_len);
    
    string headers = generate_headers(regex, match_type, options);
    
    // first pass
    vector<Matcher> first_funcs = compile_single_pass(regex, parsed_regex, options, match_type, 0, parsed_flags, cache);
    // second pass
    vector<Matcher> second_funcs;
    if (two_pass)
        second_funcs = compile_single_pass(regex, parsed_regex, options, match_type, 1, parsed_flags, cache);
    
    delete[] cache;

    return tuple<vector<Matcher>, vector<Matcher>>{first_funcs, second_funcs};  
}

// convert the end of match index into a binary result (match or no match)
int eom_to_binary(int eom, int str_start, int str_len, MatchType match_type, Schedule options) {
    int inc = (options.reverse) ? -1 : 1;
    if (eom == (str_start - inc)) // no match
        return 0;
    else if (match_type == MatchType::PARTIAL_SINGLE)
        return 1;
    else
        return eom == str_len; // in case of full match eom has to be str_len
}

vector<int> run_matchers(vector<Matcher>& funcs, string str, int str_start, Schedule options, MatchType match_type, bool binary) {
    
    const char* str_c = str.c_str();
    int str_len = str.length();
    
    // intialize the result
    vector<int> result;
    for (int i = 0; i < options.interleaving_parts; i++)
        result.push_back(0);

    // only one function to run
    if (options.interleaving_parts == 1) {
        result[0] = funcs[0](str_c, str_len, str_start);
        if (binary)
            result[0] = eom_to_binary(result[0], str_start, str_len, match_type, options);
        return result;
    }

    // parallelize
    #pragma omp parallel for
    for (int part_id = 0; part_id < options.interleaving_parts; part_id++) {
        int current = funcs[part_id](str_c, str_len, str_start);
        int binary_match = eom_to_binary(current, str_start, str_len, match_type, options);
        result[part_id] = (binary) ? binary_match : current;
    }
    return result;
}

/**
 Compiles and calls the generated function. Returns if there is a match or not.
*/
int match(string regex, string str, RegexOptions options, MatchType match_type, string* submatch) {
    Schedule schedule1 = get_schedule_options(regex, options, match_type, 0);
    if (options.binary || submatch == nullptr) {
        // binary match - one pass is enough
        vector<Matcher> funcs = get<0>(compile(regex, options, match_type, false));
        vector<int> result = run_matchers(funcs, str, 0, schedule1, match_type, true);

        // check if any of the threads resulted in a match
        for (int tid = 0; tid < (int)result.size(); tid++) {
            if (result[tid])
                return 1;
        }
        return 0;
    }
    
    // for matches anchored at the start of string we need only a forward pass
    bool two_pass = !(regex.length() > 0 && regex[0] == '^');
    
    // need to extract the first match - need 2 passes
    tuple<vector<Matcher>, vector<Matcher>> funcs = compile(regex, options, match_type, two_pass);
    
    int str_start = (schedule1.reverse) ? str.length() - 1 : 0; // both greedy and lazy start with reversed matching, for ^ start at 0
    vector<int> first_pass = run_matchers(get<0>(funcs), str, str_start, schedule1, match_type, false);

    int som;
    if (schedule1.last_eom && !schedule1.reverse)
        som = *max_element(first_pass.begin(), first_pass.end());
    else
        som = *min_element(first_pass.begin(), first_pass.end());
    bool is_match = eom_to_binary(som, str_start, str.length(), match_type, schedule1);
    //cout << "som: " << som << endl;
    // the first pass is reversed, the second one is forward
    if (!is_match) {
        *submatch = ""; // no match
        return 0;
    } else if (!two_pass) {
        *submatch = str.substr(str_start, som);
        return 1;
    } else {
        Schedule schedule2 = get_schedule_options(regex, options, match_type, 1);

        vector<int> second_pass = run_matchers(get<1>(funcs), str, som + 1, schedule2, match_type, false);
        int eom = *max_element(second_pass.begin(), second_pass.end());
        //cout << "eom: " << eom << endl;
        *submatch = str.substr(som + 1, eom - som - 1);
        return 1;
    }
    
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



