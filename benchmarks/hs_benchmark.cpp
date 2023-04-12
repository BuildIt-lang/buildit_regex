
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


string make_lazy(string greedy_regex) {
    string lazy = "";
    for (int i = 0; i < greedy_regex.length(); i++) {
        lazy += greedy_regex[i];
        char c = greedy_regex[i];
        if (c == '*' || c == '+' || c == '?' || c == '}')
            lazy += "?";
    }
    return lazy;
}
/** 
 This function will get called when a match is found.
 Returning a non-zero number should halt the scanning.
*/
int single_match_handler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *context) {
    vector<int>* matches = (vector<int>*)context;
    matches->push_back((int)from);
    matches->push_back((int)to);
    return 1;
}

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
        vector<string> tokens;
        stringstream line(pattern_line);
        while (getline(line, token, '/')) {
            // push both the pattern and its flags
            tokens.push_back(token);
            tok_id++;
        }
        string pattern = "";
        for (int tid = 1; tid < tokens.size()-1; tid++) {
            if (tokens[tid].length() > 0) {
                pattern += tokens[tid];
                if (tid < tokens.size() - 2) {
                    pattern += "/";    
                }
            }
        }
        patterns.push_back(pattern);
        // only the string after the last / is the flags
        patterns.push_back(tokens.back());
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
    //return ".jjjjjj..jjjjjjj......jjjjjjjjjj......jjjjjj.";
    return flags;
    
}

string generate_j_flags(string pattern) {
    cout << "j flags" << pattern << endl;
    string flags = ".";
    for (int i = 0; i < pattern.length()-2; i++) {
        flags += (i < 15) ? 'j' : '.';
    }
    flags += '.';
    return flags;
    
}


vector<vector<Matcher>> compile_buildit(vector<string> patterns, int n_patterns, int block_size, int batch_id) {
    auto start = high_resolution_clock::now();
    vector<vector<Matcher>> compiled_patterns;
    for (int re_id = 2 * n_patterns * batch_id; re_id < n_patterns * 2 * (batch_id + 1); re_id = re_id + 2) {
        cout << "Compiling regex " << to_string(re_id / 2) << ": " << patterns[re_id] << endl;
        string re = patterns[re_id];
        string regex = (re[0] == '^') ? "^(" + re.substr(1, re.length() - 1) + ")" : "(" + re + ")";
        string flags = patterns[re_id + 1];
        cout << "Flags: " << flags << "; ";
        RegexOptions opt;
        opt.interleaving_parts = 32; // TODO: change this!!
        opt.binary = true; // we don't care about the specific match
        //opt.flags = "";
        if (re_id == 0 || (re_id >= 6 && re_id <= 10))
            opt.flags = generate_j_flags(regex);
        else
            opt.flags = generate_flags(regex); // split for faster compilation
        cout << "Split positions: " << opt.flags << endl;
        opt.ignore_case = (flags.find("i") != std::string::npos);
        opt.dotall = (flags.find("s") != std::string::npos);
        opt.block_size = block_size;
        vector<Matcher> funcs = get<0>(compile(regex, opt, MatchType::PARTIAL_SINGLE, false));
        compiled_patterns.push_back(funcs);
    }
    auto end = high_resolution_clock::now();
    float elapsed_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / 1e6f;
    cout << "BuildIt compilation time: " << elapsed_time << "ms" << endl;
    return compiled_patterns;
}

void run_buildit(vector<vector<Matcher>> compiled_patterns, string text, int n_iters, bool individual, int block_size, int batch_id) {
    const char* s = text.c_str();
    int s_len = text.length();
    cout << "text len: " << s_len << endl;
    Schedule opt;
    auto start = high_resolution_clock::now();
    auto last_end = start;
    for (int iter = 0; iter < n_iters; iter++) {
        for (int i = 0; i < compiled_patterns.size(); i++) {
            vector<Matcher> funcs = compiled_patterns[i];
            int result = -1;
            if (block_size == -1) {
                if (funcs.size() == 1) {
                    result = funcs[0](s, s_len, 0);    
                } else {
                    result = -1;
                    #pragma omp parallel for
                    for (int tid = 0; tid < funcs.size(); tid++) {
                        int curr_result = funcs[tid](s, s_len, 0);
                        if (curr_result > -1)
                            result = curr_result;
                    }
                }
            } else {
                #pragma omp parallel for
                for (int chunk = 0; chunk < s_len; chunk = chunk + block_size) {
                    if (result > -1)
                        continue;
                    if (funcs.size() == 1) {
                        result = funcs[0](s, s_len, chunk);
                        if (result == chunk - 1)
                            result = -1;
                    } else {
                        result = -1;
                        #pragma omp parallel for
                        for (int tid = 0; tid < funcs.size(); tid++) {
                            int curr_result = funcs[tid](s, s_len, chunk);
                            if (curr_result > chunk-1)
                                result = curr_result;
                        }
                    }
                }
            }
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

void run_re2(vector<string> patterns, string text, int n_iters, bool individual, int n_patterns, int batch_id) {
    auto start = high_resolution_clock::now();
    vector<unique_ptr<RE2>> compiled_patterns;
    for (int i = 2 * n_patterns * batch_id; i < n_patterns * 2 * (batch_id + 1); i = i + 2) {
        string re = patterns[i];
        string regex = (re[0] == '^') ? "^(" + re.substr(1, re.length() - 1) + ")" : "(" + re + ")";
        regex = make_lazy(regex);
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

void run_hyperscan(vector<string> patterns, string text, int n_iters, bool individual, int n_patterns,  int batch_id) {
        
    char* s = (char*)text.c_str();
    int s_len = text.length();

    // compile
    vector<hs_database_t*> compiled_patterns;
    auto start = high_resolution_clock::now();
    for (int i = 2 * n_patterns * batch_id; i < 2 * n_patterns * (batch_id + 1); i = i + 2) {
        string pattern = patterns[i];
        char* re_cstr = (char*)pattern.c_str();
        string flags = patterns[i+1];
        bool caseless = (flags.find("i") != std::string::npos);
        bool dotall = (flags.find("s") != std::string::npos);
        hs_database_t *database;
        hs_compile_error_t *compile_error;
        unsigned int hs_flags = HS_FLAG_SOM_LEFTMOST;
        if (caseless) hs_flags = hs_flags || HS_FLAG_CASELESS;
        if (dotall) hs_flags = hs_flags || HS_FLAG_DOTALL;
        hs_compile(re_cstr, hs_flags, HS_MODE_BLOCK, NULL, &database, &compile_error);
        compiled_patterns.push_back(database);
    }
    auto end = high_resolution_clock::now();
    float compile_time = (duration_cast<nanoseconds>(end - start)).count() / (1e6f);
    cout << "Hyperscan compilation time: " << compile_time << "ms" << endl;

    // run
    start = high_resolution_clock::now();
    auto last_end = start;
    for (int iter = 0; iter < n_iters; iter++) {
        for (int i = 0; i < compiled_patterns.size(); i++) {
            vector<int> matches;
            hs_database_t *database = compiled_patterns[i];
            hs_scratch_t *scratch = NULL;
            hs_alloc_scratch(database, &scratch);
            hs_scan(database, s, s_len, 0, scratch, single_match_handler, &matches);
            int result = (matches.size() == 2);
            if (iter == 0 && individual) {
                end = high_resolution_clock::now();
                float elapsed_time = (duration_cast<nanoseconds>(end - last_end)).count() * 1.0 / 1e6f;
                last_end = end;
                cout << i << " running time: " << elapsed_time << "ms; result: " << result << endl;
            }
        }
    }
    end = high_resolution_clock::now();
    float elapsed_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    cout << "Hyperscan running time: " << elapsed_time << "ms" << endl;
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
int main(int argc, char **argv) {
    int batch_id = 0;
    if (argc == 1) {
        cout << "Missing batch id! The default is 0." << endl;    
    } else {
        batch_id = stoi(argv[1]);
    }
    cout << "Running batch " << batch_id << endl;
    bool run_teakettle = false;
    bool run_snort = true;
    string data_dir = "data/hsbench-samples/";
    string gutenberg = load_corpus(data_dir + "corpora/gutenberg.txt");    
    int n_iters = 100;
    bool individual_times = true;
    int n_patterns = 15;
    int n_chunks = 1;
    if (run_teakettle) {
        int block_size = (int)(gutenberg.length() / n_chunks);
        if (n_chunks == 1) block_size = -1;
        vector<string> teakettle = load_patterns(data_dir + "pcre/teakettle_2500");
        //n_patterns = teakettle.size();
        cout << "Num patterns: " << n_patterns << endl;
        vector<vector<Matcher>> compiled_teakettle = compile_buildit(teakettle, n_patterns, block_size, batch_id); 
        run_buildit(compiled_teakettle, gutenberg, n_iters, individual_times, block_size, batch_id);
        run_re2(teakettle, gutenberg, n_iters, individual_times, n_patterns, batch_id);
        run_hyperscan(teakettle, gutenberg, n_iters, individual_times, n_patterns, batch_id);
    }
    if (run_snort) {
        vector<string> snort_literals = load_patterns(data_dir + "pcre/snort_literals");
        string alexa = load_corpus(data_dir + "corpora/alexa200.txt");
        int block_size = (int)(alexa.length() / n_chunks);
        if (n_chunks == 1) block_size = -1;
        vector<vector<Matcher>> compiled_snort = compile_buildit(snort_literals, n_patterns, block_size, batch_id); 
        run_buildit(compiled_snort, alexa, n_iters, individual_times, block_size, batch_id);
        run_re2(snort_literals, alexa, n_iters, individual_times, n_patterns, batch_id);
        run_hyperscan(snort_literals, alexa, n_iters, individual_times, n_patterns, batch_id);
    }
}
