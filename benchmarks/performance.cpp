#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <re2/re2.h>
#include <re2/stringpiece.h>
#include <src/hs.h>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "match.h"
#include "progress.h"
#include "parse.h"
#include "frontend.h"
#include <pcrecpp.h>

using namespace std;
using namespace re2;
using namespace std::chrono;

typedef int (*GeneratedFunction)(const char*, int, int, int);

/**
Loads the patterns from a file into a vector.
*/
vector<string> load_patterns(string fname) {
    // load the patterns from a file
    ifstream patterns_file;
    patterns_file.open(fname);
    // compile the patterns for faster matching
    vector<string> patterns;
    string pattern_line;
    while (getline(patterns_file, pattern_line)) {
        patterns.push_back(pattern_line);
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
 Gets called every time a match is found during scanning.
 It adds the found match to the `context` array.
*/
int all_matches_handler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *context) {
    
    // update context with the new match
    vector<int>* matches = (vector<int>*)context;
    matches->push_back((int)from);
    matches->push_back((int)to);
    // returning 0 continues matching
    return 0;    
}

void time_compare(const vector<string> &patterns, const vector<string> &strings, int n_iters, MatchType match_type, vector<int> &n_funcs, vector<int> decompose) {
    // pattern compilation
    vector<MatchFunction**> buildit_patterns;
    vector<int> sub_parts; // sizes of the regex parts
    vector<unique_ptr<RE2>> re2_patterns;
	vector<pcrecpp::RE> pcre_patterns;
    vector<hs_database_t*> hs_databases;
	vector<char *> hs_pattern_arrs;
    int ignore_case = false;
    cout << endl << "COMPILATION TIMES" << endl << endl;
    for (int i = 0; i < patterns.size(); i++) {
        cout << "--- " << patterns[i] << " ---" << endl;
        int n_threads = (match_type == MatchType::FULL) ? 1 : n_funcs[i];
        
        // buildit
        // parse regex
        string processed_re = expand_regex(patterns[i]);
        set<string> regex_parts = {processed_re};

        if (decompose[i] && match_type != MatchType::FULL) {
            // decompose the regex into or groups
            int* or_positions = new int[processed_re.length()];
            get_or_positions(processed_re, or_positions);
            regex_parts = split_regex(processed_re, or_positions, 0, processed_re.length()-1);
            delete[] or_positions;
        }

        sub_parts.push_back(regex_parts.size());
        MatchFunction** all_funcs = new MatchFunction*[regex_parts.size()];
        auto start = high_resolution_clock::now();
        int rid = 0;
        // generate n_threads functions for each regex part
        for (string re: regex_parts) {

            // cache the state transitions
            const int re_len = re.length();
            const int cache_size = (re_len + 1) * (re_len + 1); 
            std::unique_ptr<int> cache_states_ptr(new int[cache_size]);
            int* cache = cache_states_ptr.get();
            cache_states(re.c_str(), cache);
            MatchFunction* funcs = new MatchFunction[n_threads];
            all_funcs[rid] = funcs;

            // generate a function for each thread
            for (int tid = 0; tid < n_threads; tid++) {    
                builder::builder_context context;
                context.feature_unstructured = true;
                context.run_rce = true;
                bool partial = (match_type == MatchType::PARTIAL_SINGLE);
                MatchFunction func = (MatchFunction)builder::compile_function_with_context(context, match_regex, re.c_str(), partial, cache, tid, n_threads, ignore_case); 
                funcs[tid] = func;
            }
            rid++;
        }
        auto end = high_resolution_clock::now();
        cout << "buildit compile time: " << (duration_cast<nanoseconds>(end - start)).count() / 1e6f << "ms" << endl;
        buildit_patterns.push_back(all_funcs);
        
        // re2
        auto re_start = high_resolution_clock::now();
        re2_patterns.push_back(unique_ptr<RE2>(new RE2(patterns[i])));
        auto re_end = high_resolution_clock::now();
        cout << "re compile time: " << (duration_cast<nanoseconds>(re_end - re_start)).count() / 1e6f << "ms" << endl;

		// hyperscan
		hs_database_t *database;
		hs_compile_error_t *compile_error;
		auto hs_start = high_resolution_clock::now();
		hs_compile((char*)patterns[i].c_str(), HS_FLAG_SOM_LEFTMOST, HS_MODE_BLOCK, NULL, &database, &compile_error);
		auto hs_end = high_resolution_clock::now();
		cout << "hs compile time: " << (duration_cast<nanoseconds>(hs_end - hs_start)).count() / 1e6f << "ms" << endl;
		hs_databases.push_back(database);
    
        // pcre
        auto pcre_start = high_resolution_clock::now();
        pcrecpp::RE pcre_re(patterns[i]);
        pcre_patterns.push_back(pcre_re);
        auto pcre_end = high_resolution_clock::now();
        cout << "pcre compile time: " << (duration_cast<nanoseconds>(pcre_end - pcre_start)).count() / 1e6f << "ms" << endl;
    }

    // matching
    bool re_result;
    bool hs_result;
    bool buildit_result;
    cout << endl << "MATCHING TIMES" << endl << endl;
    for (int j = 0; j < patterns.size(); j++) {
        const string& cur_string = (match_type == MatchType::PARTIAL_SINGLE) ? strings[0] : strings[j];
        int n_threads = (match_type == MatchType::FULL) ? 1 : n_funcs[j];
        const char* s = cur_string.c_str();
        int s_len = cur_string.length();
        // re2 timing
        auto re_start = high_resolution_clock::now();
        for (int i = 0; i < n_iters; i++) {
            if (match_type == MatchType::PARTIAL_SINGLE) {
                re_result = RE2::PartialMatch(cur_string, *re2_patterns[j].get()); 
            } else {
                re_result = RE2::FullMatch(cur_string, *re2_patterns[j].get());    
            }
        }
        auto re_end = high_resolution_clock::now();
        float re2_dur = (duration_cast<nanoseconds>(re_end - re_start)).count() * 1.0 / (1e6f *  n_iters);
        
        // hs timing
        auto hs_start = high_resolution_clock::now();
        for (int i = 0; i < n_iters; i++) {
			if (match_type == MatchType::PARTIAL_SINGLE || match_type == MatchType::FULL) {
                vector<int> matches;
				hs_scratch_t *scratch = NULL;
				hs_alloc_scratch(hs_databases[j], &scratch);
				hs_scan(hs_databases[j], (char*)s, s_len, 0, scratch, single_match_handler, &matches);
                hs_result = (matches.size() == 2);
			}
        }
        auto hs_end = high_resolution_clock::now();
        float hs_dur = (duration_cast<nanoseconds>(hs_end - hs_start)).count() * 1.0 / (1e6f * n_iters);

        // pcre timing
        auto pcre_start = high_resolution_clock::now();
        int pcre_result = 0;
        for (int i = 0; i < n_iters; i++) { 
            if (match_type == MatchType::PARTIAL_SINGLE) {
                pcre_result = pcre_patterns[j].PartialMatch(s); 
            } else {
                pcre_result = pcre_patterns[j].FullMatch(s);    
            }
        }
        auto pcre_end = high_resolution_clock::now();
        float pcre_dur = (duration_cast<nanoseconds>(pcre_end - pcre_start)).count() * 1.0 / (1e6f * n_iters);
        
        // buildit timing
        float buildit_dur;
        if (match_type == MatchType::FULL || !decompose[j]) {
            auto buildit_start = high_resolution_clock::now();
            for (int i = 0; i < n_iters; i++) {
                if (n_threads == 1) {
                    buildit_result = buildit_patterns[j][0][0](s, s_len, 0);
                } else {
                    buildit_result = false;
                    #pragma omp parallel for
                    for (int tid = 0; tid < n_threads; tid++) {
                        if (buildit_patterns[j][0][tid](s, s_len, 0))
                            buildit_result = true;
                    }
                }
            }
            auto buildit_end = high_resolution_clock::now();
            buildit_dur = (duration_cast<nanoseconds>(buildit_end - buildit_start)).count() * 1.0 / (1e6f * n_iters);
        } else {
            auto buildit_start = high_resolution_clock::now();
            for (int i = 0; i < n_iters; i++) {
                if (n_threads == 1) {
                    buildit_result = false;
                    #pragma omp parallel for
                    for (int rid = 0; rid < sub_parts[j]; rid++) {
                        if (buildit_patterns[j][rid][0](s, s_len, 0))
                            buildit_result = true;
                    }    
                } else {
                    buildit_result = false;
                    #pragma omp parallel for
                    for (int tid = 0; tid < n_threads; tid++) {
                        //#pragma omp parallel for
                        for (int rid = 0; rid < sub_parts[j]; rid++) {
                            if (buildit_patterns[j][rid][tid](s, s_len, 0))
                                buildit_result = true;
                        }
                    }
                }
            }
            auto buildit_end = high_resolution_clock::now();
            buildit_dur = (duration_cast<nanoseconds>(buildit_end - buildit_start)).count() * 1.0 / (1e6f * n_iters);
        }
        // check correctness
		const string& str = (match_type == MatchType::PARTIAL_SINGLE) ? "<Twain Text>" : strings[j];
        
        if (!((hs_result == buildit_result || match_type == MatchType::FULL) && re_result == buildit_result)) {
            cout << "Correctness failed for regex " << patterns[j] << " and text " << str << ":" <<endl;
            cout << "  re2 match = " << re_result << endl;
            cout << "  hs match = " << hs_result << endl;
            cout << "  pcre match = " << pcre_result << endl;
            cout << "  buildit match = " << buildit_result << endl;
        }

        cout << "--- pattern: " << patterns[j] << " text: " << str << " ---" << endl;
        cout << "re2 run time: " << re2_dur << " ms" << endl;
        cout << "hs run time: " << hs_dur << " ms" << endl;
        cout << "pcre run time: " << pcre_dur << " ms" << endl;
        cout << "buildit run time: " << buildit_dur << " ms" << endl;
    }

}

void optimize_partial_match_loop(string str, string pattern) {
    
    string re = expand_regex(pattern);
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
    MatchFunction func = (MatchFunction)builder::compile_function_with_context(context, match_regex, re.c_str(), 0, cache, 0, 1, 0); 
    auto end = high_resolution_clock::now();
    int compile_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / 1e6f;
    cout << "compile time: " << compile_time << "ms" << endl;
    
    // running
    const char* s = str.c_str();
    int len = str.length();
    int result = 0;
    int stride = 16;
    int n_iters = 10;
    start = high_resolution_clock::now();
    for (int k = 0; k < n_iters; k++) {
        result = partial_match_loop(s, len, stride, func);
    }
    end = high_resolution_clock::now();
    int expected = RE2::PartialMatch(s, pattern);
    cout << "expected: " << expected << ", got: " << result << endl;

    float run_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    cout << "run time: " << run_time << "ms" << endl;

}

int main() {
    string patterns_file = "./data/twain_patterns.txt";
    string corpus_file = "./data/twain.txt";
    string text = load_corpus(corpus_file);
    std::cout << "Twain Text Length: " << text.length() << std::endl;

    vector<string> patterns = load_patterns(patterns_file);
    int n_iters = 10;
    vector<string> twain_patterns = {
        "Twain",
        "(Huck[a-zA-Z]+|Saw[a-zA-Z]+)",
        "[a-q][^u-z]{5}x", 
        "(Tom|Sawyer|Huckleberry|Finn)",
        ".{0,2}(Tom|Sawyer|Huckleberry|Finn)",
        ".{2,4}(Tom|Sawyer|Huckleberry|Finn)",
        "Tom.{10,15}",
        "(Tom.{10,15}river|river.{10,15}Tom)",
        "[a-zA-Z]+ing",
    };
    vector<int> n_funcs = {1, 1, 2, 1, 1, 1, 1, 4, 1};
    vector<int> decompose = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    vector<string> words = {"Twain", "HuckleberryFinn", "qabcabx", "Sawyer", "Sawyer Tom", "SaHuckleberry", "Tom swam in the river", "Tom swam in the river", "swimming"};
	time_compare(twain_patterns, words, n_iters, MatchType::FULL, n_funcs, decompose);
	cout << endl;
    time_compare(twain_patterns, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, n_funcs, decompose);
    //cout << "pcre" << pcrecpp::RE("h.*o").FullMatch("hello") << endl;
    // trying to optimize partial matches
    for (string re: twain_patterns) {
        cout << endl;
        cout << re << endl;
        optimize_partial_match_loop(text, re + ".*");
    }
}

