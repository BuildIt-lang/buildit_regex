
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <map>
#include "pattern_config.h"
#include <re2/re2.h>
#include <re2/stringpiece.h>
#include <src/hs.h>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "match.h"
#include "progress.h"
#include "parse.h"
#include <pcrecpp.h>
#include "compile.h"

using namespace std;
using namespace re2;
using namespace std::chrono;
using namespace builder;


/**
Loads the patterns from a file into a vector.
*/
vector<string> load_patterns(string fname) {
    // load the patterns from a file
    ifstream patterns_file;
    patterns_file.open(fname);
    
    vector<string> patterns;
    string pattern_line;
    while (getline(patterns_file, pattern_line)) {
        string token;
        int tok_id = 0;
        stringstream line(pattern_line);
        while (getline(line, token, '/')) {
            // push both the pattern and its flags
            if (tok_id > 0)
                patterns.push_back(token);
            tok_id++;
        }
    }
    patterns_file.close();
    return patterns;
}

/**
Loads the text that is going to be searched for matches.
*/
string load_corpus(string fname) {
    ifstream input_file;
    input_file.open(fname);
    string text =  string((istreambuf_iterator<char>(input_file)), istreambuf_iterator<char>());   
    input_file.close();
    return text;
}

string generate_flags(string pattern) {
    string flags = "";
    for (int i = 0; i < pattern.length(); i++) {
        if (i > 0 && (pattern[i-1] == '*' || pattern[i-1] == '+' || pattern[i-1] == '}')) {
            flags += "s";
        } else {
            flags += ".";    
        }
    }
    return flags;
    
}


vector<vector<Matcher>> compile_buildit(vector<string> patterns) {
    auto start = high_resolution_clock::now();
    vector<vector<Matcher>> compiled_patterns;
    for (int re_id = 0; re_id < 100; re_id = re_id + 2) {
        cout << "Compiling regex " << to_string(re_id / 2) << ": " << patterns[re_id] << endl;
        string regex = patterns[re_id];
        string flags = patterns[re_id + 1];
        cout << "Flags: " << flags << "; ";
        RegexOptions opt;
        opt.interleaving_parts = 1; // TODO: change this!!
        opt.binary = true; // we don't care about the specific match
        //opt.flags = "";
        opt.flags = generate_flags(regex); // split for faster compilation
        cout << "Split positions: " << opt.flags << endl;
        opt.ignore_case = (flags.find("i") != std::string::npos);
        opt.dotall = (flags.find("s") != std::string::npos);
        vector<Matcher> funcs = get<0>(compile(regex, opt, MatchType::PARTIAL_SINGLE, false));
        compiled_patterns.push_back(funcs);
    }
    auto end = high_resolution_clock::now();
    float elapsed_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / 1e6f;
    cout << "BuildIt compilation time: " << elapsed_time << "ms" << endl;
    return compiled_patterns;
}

void run_buildit(vector<vector<Matcher>> compiled_patterns, string text, int n_iters, bool individual) {
    const char* s = text.c_str();
    int s_len = text.length();
    cout << "text len: " << s_len << endl;
    int chunk_len = 3350522;
    Schedule opt;
    auto start = high_resolution_clock::now();
    auto last_end = start;
    for (int iter = 0; iter < n_iters; iter++) {
        for (int i = 0; i < compiled_patterns.size(); i++) {
            vector<Matcher> funcs = compiled_patterns[i];

            int result = 0;
            if (funcs.size() == 1) {
                result = funcs[0](s, s_len, 0);    
            } else {
                result = 0;
                #pragma omp parallel for
                for (int tid = 0; tid < funcs.size(); tid++) {
                    if (funcs[tid](s, s_len, 0) > -1)
                        result = 1;
                }
            }
            /*int result = 0;
            #pragma omp parallel for
            for (int chunk = 0; chunk < s_len; chunk = chunk + chunk_len) {
                if (funcs[0](s, s_len, chunk - chunk_len, chunk) > -1)
                    result = 1;
            }*/
            if (iter == 0 && individual) {
                auto end = high_resolution_clock::now();
                float elapsed_time = (duration_cast<nanoseconds>(end - last_end)).count() * 1.0 / 1e6f;
                last_end = end;
                cout << i << " running time: " << elapsed_time << "ms; result: " << result << endl;
            }
        }
    }
    auto end = high_resolution_clock::now();
    float elapsed_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    cout << "BuildIt running time: " << elapsed_time << "ms" << endl;
}

void run_re2(vector<string> patterns, string text, int n_iters, bool individual) {
    auto start = high_resolution_clock::now();
    vector<unique_ptr<RE2>> compiled_patterns;
    for (int i = 0; i < 100; i = i + 2) {
        string regex = "(" + patterns[i] + ")";
        string flags = patterns[i+1];
        RE2::Options opt;
        opt.set_dot_nl(flags.find('s') != std::string::npos);
        opt.set_case_sensitive(flags.find('i') == std::string::npos);
        compiled_patterns.push_back(unique_ptr<RE2>(new RE2(regex, opt)));
    }
    auto end = high_resolution_clock::now();
    float elapsed_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / 1e6f;
    cout << "RE2 compilation time: " << elapsed_time << "ms" << endl;

    start = high_resolution_clock::now();
    auto last_end = start;
    for (int iter = 0; iter < n_iters; iter++) {
        for (int i = 0; i < compiled_patterns.size(); i++) {
            int result = RE2::PartialMatch(text, *compiled_patterns[i].get());    
            if (iter == 0 && individual) {
                end = high_resolution_clock::now();
                elapsed_time = (duration_cast<nanoseconds>(end - last_end)).count() * 1.0 / 1e6f;
                last_end = end;
                cout << i << " running time: " << elapsed_time << "ms; result: " << result << endl;
            }
        }
    }
    end = high_resolution_clock::now();
    elapsed_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    cout << "RE2 running time: " << elapsed_time << "ms" << endl;
}

void all_partial_time(string str, string pattern) {

    string re = get<0>(expand_regex(pattern));
    cout << "simple re " << re << endl;
    const int re_len = re.length();
    // precompute state transitions
    const int cache_size = (re_len + 1) * (re_len + 1); 
    std::unique_ptr<int> cache_states_ptr(new int[cache_size]);
    int* cache = cache_states_ptr.get();
    cache_states(re.c_str(), cache);
    builder::builder_context context;
    context.feature_unstructured = true;
    context.run_rce = true;

    // compilation
    auto start = high_resolution_clock::now();
    Matcher func = (Matcher)builder::compile_function_with_context(context, get_partial_match, re.c_str(), cache, 0); 
    auto end = high_resolution_clock::now();
    int compile_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / 1e6f;
    cout << "compile time: " << compile_time << "ms" << endl;

    
    // run anchored partial match starting from each position in the string
    vector<string> matches;
    const char* s = str.c_str();
    int str_len = str.length();
    
    start = high_resolution_clock::now();
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
    end = high_resolution_clock::now();
    float run_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / 1e6f;
    cout << "run time: " << run_time << "ms" << endl;
}
int main() {
    string data_dir = "data/hsbench-samples/";
    string gutenberg = load_corpus(data_dir + "corpora/gutenberg.txt");    
    vector<string> teakettle = load_patterns(data_dir + "pcre/teakettle_2500");
    vector<vector<Matcher>> compiled_teakettle = compile_buildit(teakettle); 
    int n_iters = 100;
    bool individual_times = true;
    run_buildit(compiled_teakettle, gutenberg, n_iters, individual_times);
    run_re2(teakettle, gutenberg, n_iters, individual_times);
}
