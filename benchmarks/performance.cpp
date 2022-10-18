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

using namespace std;
using namespace re2;
using namespace std::chrono;

typedef int (*GeneratedFunction) (const char* word, int len);

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
        string pattern = pattern_line.substr(3, pattern_line.size() - 4);
        patterns.push_back(pattern);
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

void time_re2(vector<string> &patterns, string &text, int n_iters) {	
    StringPiece submatch("");
    StringPiece corpus(text);
    int dur = 0;
    for (int i = 0; i < n_iters; i++) {
    	for (string pattern: patterns) {
            auto start = high_resolution_clock::now();
        	// compile the pattern into RE2 epxression for faster matching
            RE2 re2_pattern(pattern);
            bool is_match = re2_pattern.Match(corpus, 0, text.length(), RE2::Anchor::UNANCHORED, &submatch, 0);
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);
            dur += duration.count();
        }
    }
    printf("RE2 compile + run time: %f ms\n", dur * 1.0 / n_iters);
}

/**
This function is being called every time a match occurs.
Taken from hyperscan/examples/simplegrep.c
*/
static int eventHandler(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *ctx) {
    //printf("Match for pattern \"%s\" at offset %llu\n", (char *)ctx, to);
    return 0;
}

void time_hyperscan(vector<string> &patterns, string &text, int n_iters) {
    int text_len = text.length();
    char *text_arr = new char[text_len];
    strcpy(text_arr, text.c_str());
    int dur = 0;
    for (int i = 0; i < n_iters; i++) {
        for (string pattern: patterns) {
            int pattern_len = pattern.length();
            char pattern_arr[pattern_len];
            strcpy(pattern_arr, pattern.c_str());
            // compile the pattern
            hs_database_t *database;
            hs_compile_error_t *compile_err;
            hs_scratch_t *scratch = NULL;
            auto start = high_resolution_clock::now();
            hs_compile(pattern_arr, HS_FLAG_DOTALL, HS_MODE_BLOCK, NULL, &database, &compile_err);
            hs_alloc_scratch(database, &scratch);
            // do the matching
            hs_scan(database, text_arr, text_len, 0, scratch, eventHandler, pattern_arr);
            auto end = high_resolution_clock::now();
            dur += (duration_cast<milliseconds>(end - start)).count();
            hs_free_scratch(scratch);
            hs_free_database(database);
        }
    }
    delete[] text_arr;
    printf("Hyperscan compile + run time: %f ms\n", dur * 1.0 / n_iters);
}

void time_full_match(vector<string> &patterns, vector<string> &words, int n_iters) {
    
    // pattern compilation
    vector<GeneratedFunction> buildit_patterns;
    vector<unique_ptr<RE2>> re2_patterns;
    for (int i = 0; i < patterns.size(); i++) {
        // buildit
        builder::builder_context context;
        context.feature_unstructured = true;
        context.run_rce = true;
        auto fptr = (GeneratedFunction)builder::compile_function_with_context(context, match_regex, patterns[i].c_str());
        buildit_patterns.push_back(fptr);
        // re2
        re2_patterns.push_back(make_unique<RE2>(patterns[i]));
    }

    // re2 timing
    auto start = high_resolution_clock::now();
    vector<bool> expected;
    for (int i = 0; i < n_iters; i++) {
        for (int j = 0; j < patterns.size(); j++) {
            expected.push_back(RE2::FullMatch(words[j], *re2_patterns[j].get()));
        }
    }
    auto end = high_resolution_clock::now();
    float re2_dur = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / n_iters;
    
    // buildit timing
    start = high_resolution_clock::now();
    vector<bool> result;
    for (int i = 0; i < n_iters; i++) {
        for (int j = 0; j < patterns.size(); j++) {
            result.push_back(buildit_patterns[j](words[j].c_str(), words[j].length()));
        }
    }
    end = high_resolution_clock::now();
    float buildit_dur = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / n_iters;
    
    // check if correct
    for (int i = 0; i < patterns.size(); i++) {
        cout << patterns[i] << " " << words[i] << ": re2 = " << expected[i] << " buildit = " << result[i] << endl;    
    }
    cout << "TIME re2: " << re2_dur << " ns" << endl;
    cout << "TIME buildit: " << buildit_dur << " ns" << endl;

}


int main() {
    string patterns_file = "./data/twain_patterns.txt";
    string corpus_file = "./data/twain.txt";
    string text = load_corpus(corpus_file);
    vector<string> patterns = load_patterns(patterns_file);
    int n_iters = 50;
    vector<string> twain_patterns = {
        "Twain",
        "(Huck[a-zA-Z]+|Saw[a-zA-Z]+)",
//        "[a-q][ˆu-z]{13}x",
        "(Tom|Sawyer|Huckleberry|Finn)",
    };
    vector<string> words = {"Twain", "HuckleberryFinn", "Huckleberry", "Sawyer", "SaHuckleberry"};
    time_full_match(twain_patterns, words, n_iters);
    
    //time_re2(patterns, text, n_iters);

    //time_hyperscan(patterns, text, n_iters);
}

