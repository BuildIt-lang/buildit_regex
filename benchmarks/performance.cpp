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

using namespace std;
using namespace re2;
using namespace std::chrono;

typedef int (*GeneratedFunction)(const char*, int);

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

void time_compare(const vector<string> &patterns, const vector<string> &strings, int n_iters, MatchType match_type, vector<int> &n_funcs) {
    // pattern compilation
    vector<MatchFunction*> buildit_patterns;
    vector<unique_ptr<RE2>> re2_patterns;
	vector<hs_database_t*> hs_databases;
	vector<char *> hs_pattern_arrs;
    cout << endl << "COMPILATION TIMES" << endl << endl;
    for (int i = 0; i < patterns.size(); i++) {
        cout << "--- " << patterns[i] << " ---" << endl;
        int n_parts = (match_type == MatchType::FULL) ? 1 : n_funcs[i];
        
        // buildit
        // parse regex and init cache
        auto start = high_resolution_clock::now();
        string processed_re = expand_regex(patterns[i]);
        const int re_len = processed_re.length();
        const int cache_size = (re_len + 1) * (re_len + 1); 
        std::unique_ptr<int> cache_states_ptr(new int[cache_size]);
        //auto fptr = compile_regex(processed_re.c_str(), cache_states_ptr.get(), match_type);
        int* cache = cache_states_ptr.get();
        cache_states(processed_re.c_str(), cache);
        MatchFunction* funcs = new MatchFunction[n_parts];
        // code generation
        for (int tid = 0; tid < n_parts; tid++) {    
            builder::builder_context context;
            context.feature_unstructured = true;
            context.run_rce = true;
            bool partial = (match_type == MatchType::PARTIAL_SINGLE);
            MatchFunction func = (MatchFunction)builder::compile_function_with_context(context, match_regex, processed_re.c_str(), partial, cache, tid, n_parts);    
            funcs[tid] = func;
        }
        auto end = high_resolution_clock::now();
        cout << "buildit compile time: " << (duration_cast<nanoseconds>(end - start)).count() / 1e6f << "ms" << endl;
        buildit_patterns.push_back(funcs);
        
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
    }

    // matching
    bool re_result;
    bool hs_result;
    bool buildit_result;
    cout << endl << "MATCHING TIMES" << endl << endl;
    for (int j = 0; j < patterns.size(); j++) {
        const string& cur_string = (match_type == MatchType::PARTIAL_SINGLE) ? strings[0] : strings[j];
        int n_parts = (match_type == MatchType::FULL) ? 1 : n_funcs[j];
        
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
				hs_scan(hs_databases[j], (char*)cur_string.c_str(), cur_string.length(), 0, scratch, single_match_handler, &matches);
                hs_result = (matches.size() == 2);
			}
        }
        auto hs_end = high_resolution_clock::now();
        float hs_dur = (duration_cast<nanoseconds>(hs_end - hs_start)).count() * 1.0 / (1e6f * n_iters);

        // buildit timing
        auto buildit_start = high_resolution_clock::now();
        for (int i = 0; i < n_iters; i++) {
            if (n_parts == 1) {
                buildit_result = buildit_patterns[j][0](cur_string.c_str(), cur_string.length());
            } else {
                buildit_result = false;
                #pragma omp parallel for
                for (int tid = 0; tid < n_parts; tid++) {
                    if (buildit_patterns[j][tid](cur_string.c_str(), cur_string.length()))
                        buildit_result = true;
                }
            }
        }
        auto buildit_end = high_resolution_clock::now();
        float buildit_dur = (duration_cast<nanoseconds>(buildit_end - buildit_start)).count() * 1.0 / (1e6f * n_iters);

        // check correctness
		const string& str = (match_type == MatchType::PARTIAL_SINGLE) ? "<Twain Text>" : strings[j];
        
        if (!((hs_result == buildit_result || match_type == MatchType::FULL) && re_result == buildit_result)) {
            cout << "Correctness failed for regex " << patterns[j] << " and text " << str << ":" <<endl;
            cout << "  re2 match = " << re_result << endl;
            cout << "  hs match = " << hs_result << endl;
            cout << "  buildit match = " << buildit_result << endl; 
        }

        cout << "--- pattern: " << patterns[j] << " text: " << str << " ---" << endl;
        cout << "re2 run time: " << re2_dur << " ms" << endl;
        cout << "hs run time: " << hs_dur << " ms" << endl;
        cout << "buildit run time: " << buildit_dur << " ms" << endl;

    }

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
    vector<int> n_funcs = {1, 1, 2, 1, 1, 1, 1, 2, 2};

    vector<string> words = {"Twain", "HuckleberryFinn", "qabcabx", "Sawyer", "Sawyer Tom", "SaHuckleberry", "Tom swam in the river", "Tom swam in the river", "swimming"};
	
	time_compare(twain_patterns, words, n_iters, MatchType::FULL, n_funcs);
	cout << endl;
    time_compare(twain_patterns, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, n_funcs);
}

