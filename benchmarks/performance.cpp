#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <re2/re2.h>
#include <re2/stringpiece.h>
#include <src/hs.h>
#include <cstring>
#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "match.h"
#include "progress.h"
#include "parse.h"

using namespace std;
using namespace re2;
using namespace std::chrono;

typedef int (*GeneratedFunction) (const char*, int);

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

enum MatchType { FULL, PARTIAL_SINGLE, PARTIAL_ALL };
 
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

void time_compare(const vector<string> &patterns, const vector<string> &strings, int n_iters, MatchType match_type) {
    // pattern compilation
    vector<GeneratedFunction> buildit_patterns;
    vector<unique_ptr<RE2>> re2_patterns;
	vector<hs_database_t*> hs_databases;
	vector<char *> hs_pattern_arrs;
    for (int i = 0; i < patterns.size(); i++) {
        cout << "--- " << patterns[i] << " ---" << endl;
        
        // buildit
        // parse regex and init cache
        auto start = high_resolution_clock::now();
        string processed_re = expand_regex(patterns[i]);
        const int re_len = processed_re.length();
        const int cache_size = (re_len + 1) * (re_len + 1); 
        std::unique_ptr<int> cache_states_ptr(new int[cache_size]);
        int* cache = cache_states_ptr.get();
        cache_states(processed_re.c_str(), cache);
        // code generation
        builder::builder_context context;
        context.feature_unstructured = true;
        context.run_rce = true;
        auto fptr = (match_type == MatchType::PARTIAL_SINGLE) ? 
			(GeneratedFunction)builder::compile_function_with_context(context, match_regex_partial, processed_re.c_str(), cache) :
            (GeneratedFunction)builder::compile_function_with_context(context, match_regex_full, processed_re.c_str(), cache);
        auto end = high_resolution_clock::now();
        cout << "buildit compile time: " << (duration_cast<nanoseconds>(end - start)).count() / 1e6f << "ms" << endl;
        buildit_patterns.push_back(fptr);
        
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

    // re2 timing
    auto start = high_resolution_clock::now();
    vector<bool> re2_expected;
    for (int i = 0; i < n_iters; i++) {
        for (int j = 0; j < patterns.size(); j++) {
            if (match_type == MatchType::PARTIAL_SINGLE) {
                re2_expected.push_back(RE2::PartialMatch(strings[0], *re2_patterns[j].get()));    
            } else {
                re2_expected.push_back(RE2::FullMatch(strings[j], *re2_patterns[j].get()));    
            }
        }
    }
    auto end = high_resolution_clock::now();
    float re2_dur = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f *  n_iters);
	
    // hyperscan timing
	start = high_resolution_clock::now();
	vector<bool> hs_expected;
	for (int i = 0; i < n_iters; i++) {
		for (int j = 0; j < patterns.size(); j++) {
			if (match_type == MatchType::PARTIAL_SINGLE || match_type == MatchType::FULL) {
                vector<int> matches;
				hs_scratch_t *scratch = NULL;
				hs_alloc_scratch(hs_databases[j], &scratch);
			    const string& cur_string = (match_type == MatchType::PARTIAL_SINGLE) ? strings[0] : strings[j];
				hs_scan(hs_databases[j], (char*)cur_string.c_str(), cur_string.length(), 0, scratch, single_match_handler, &matches);
                hs_expected.push_back(matches.size() == 2);
			}
		}
	}
	end = high_resolution_clock::now();
    float hs_dur = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    
    // buildit timing
    start = high_resolution_clock::now();
    vector<bool> result;
    for (int i = 0; i < n_iters; i++) {
        for (int j = 0; j < patterns.size(); j++) {
			const string& cur_string = (match_type == MatchType::PARTIAL_SINGLE) ? strings[0] : strings[j];
            bool is_match = buildit_patterns[j](cur_string.c_str(), cur_string.length());
            result.push_back(is_match);
        }
    }
    end = high_resolution_clock::now();
    float buildit_dur = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    
    // check if correct
    for (int i = 0; i < patterns.size(); i++) {
		const string& cur_string = (match_type == MatchType::PARTIAL_SINGLE) ? "<Twain Text>" : strings[i];
        
        if (re2_expected[i] == hs_expected[i] && re2_expected[i] == result[i]) {
            continue;    
        } else {
            cout << "Correctness failed for regex " << patterns[i] << " and text " << cur_string << ":" <<endl;
            cout << "  re2 match = " << re2_expected[i] << endl;
            cout << "  hs match = " << hs_expected[i] << endl;
            cout << "  buildit match = " << result[i] << endl; 
        }
    }

    cout << "~ TOTAL RUNNING TIMES ~" << endl;
    cout << "re2 run time: " << re2_dur << " ms" << endl;
    cout << "hs run time: " << hs_dur << " ms" << endl;
    cout << "buildit run time: " << buildit_dur << " ms" << endl;
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
        //"[a-q][^u-z]{5}x",
        "(Tom|Sawyer|Huckleberry|Finn)",
        ".{2,4}(Tom|Sawyer|Huckleberry|Finn)",
        ".{2,4}(Tom|Sawyer|Huckleberry|Finn)",
        //"Tom.{10,15}",
        //"(Tom.{10,15}river|river.{10,15}Tom)",
        //"[a-zA-Z]+ing",
    };
    vector<string> words = {"Twain", "HuckleberryFinn", "qabcabx", "Sawyer", "Sawyer Tom", "SaHuckleberry", "Tom swam in the river", "swimming"};
	
	time_compare(twain_patterns, words, n_iters, MatchType::FULL);
	cout << endl;
    time_compare(twain_patterns, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE);
}

