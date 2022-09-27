#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <re2/re2.h>
#include <re2/stringpiece.h>

using namespace std;
using namespace re2;
using namespace std::chrono;

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
    for (int i = 0; i < n_iters; i++) {
    	for (string pattern: patterns) {
        	// compile the pattern into RE2 epxression for faster matching
            RE2 re2_pattern(pattern);
            bool is_match = re2_pattern.Match(corpus, 0, text.length(), RE2::Anchor::UNANCHORED, &submatch, 0);
        }
    }
}

int main() {
    string patterns_file = "./data/twain_patterns.txt";
    string corpus_file = "./data/twain.txt";
    string text = load_corpus(corpus_file);
    vector<string> patterns = load_patterns(patterns_file);
    int n_iters = 10;
    auto start = high_resolution_clock::now();
    time_re2(patterns, text, n_iters);
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    printf("RE2 compile + run time: %ld ms\n", duration.count() / n_iters);
}

