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
#include "compile.h"

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
    if (from != to) {
        vector<int>* matches = (vector<int>*)context;
        matches->push_back((int)from);
        matches->push_back((int)to);
    }
    // returning 0 continues matching
    return 0;    
}

// taken from https://github.com/google/re2/blob/main/re2/testing/regexp_benchmark.cc
// Generate random text that won't contain the search string,
// to test worst-case search behavior.
std::string generate_random_text(int64_t nbytes) {
    static const std::string* const text = []() {
        std::string* text = new std::string;
        srand(1);
        text->resize(16<<20);
        for (int64_t i = 0; i < 16<<20; i++) {
            // Generate a one-byte rune that isn't a control character (e.g. '\n').
            // Clipping to 0x20 introduces some bias, but we don't need uniformity.
            int byte = rand() & 0x7F;
            if (byte < 0x20)
                byte = 0x20;
            (*text)[i] = byte;
        }
        return text;
    }();
    return text->substr(0, nbytes);
}

void time_compare(const vector<string> &patterns, const vector<string> &strings, int n_iters, MatchType match_type, const vector<RegexOptions> &options) {
    // pattern compilation
    vector<vector<Matcher>> buildit_patterns;
    vector<int> sub_parts; // sizes of the regex parts
    vector<unique_ptr<RE2>> re2_patterns;
	vector<pcrecpp::RE> pcre_patterns;
    vector<hs_database_t*> hs_databases;
	vector<char *> hs_pattern_arrs;
    int ignore_case = false;
    cout << endl << "COMPILATION TIMES" << endl << endl;
    for (int i = 0; i < patterns.size(); i++) {
        cout << "--- " << patterns[i] << " ---" << endl;
        auto start = high_resolution_clock::now();
        cout << "interleaving parts: " << options[i].interleaving_parts << endl;
        vector<Matcher> func = compile(patterns[i], options[i], match_type);
        auto end = high_resolution_clock::now();
        cout << "buildit compile time: " << (duration_cast<nanoseconds>(end - start)).count() / 1e6f << "ms" << endl;
        buildit_patterns.push_back(func);
       
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
    cout << endl << "MATCHING TIMES" << endl << endl;
    for (int j = 0; j < patterns.size(); j++) {
        const string& cur_string = (match_type != MatchType::FULL) ? strings[0] : strings[j];
       
        const char* s = cur_string.c_str();
        int s_len = cur_string.length();
        
        // re2 timing
        int re_result = 0;
        auto re_start = high_resolution_clock::now();
        for (int i = 0; i < n_iters; i++) {
            if (match_type == MatchType::PARTIAL_SINGLE) {
                re_result = RE2::PartialMatch(cur_string, *re2_patterns[j].get()); 
            } else if (match_type == MatchType::FULL) {
                re_result = RE2::FullMatch(cur_string, *re2_patterns[j].get());    
            } else {
                // find all partial matches
                string word;
                re_result = 0;
                StringPiece input(s);
                while (RE2::FindAndConsume(&input, *re2_patterns[j].get(), &word)) {
                    if (word.length() == 0) {
                        // advance the input pointer by 1
                        // otherwise it will be stuck in an infinite loop
                        input.remove_prefix(1);
                    } else {
                        re_result++;
                    }    
                }
            }
        }
        auto re_end = high_resolution_clock::now();
        float re2_dur = (duration_cast<nanoseconds>(re_end - re_start)).count() * 1.0 / (1e6f *  n_iters);
        
        // hs timing
        int hs_result = 0;
        auto hs_start = high_resolution_clock::now();
        for (int i = 0; i < n_iters; i++) {
            vector<int> matches;
            hs_scratch_t *scratch = NULL;
            hs_alloc_scratch(hs_databases[j], &scratch);
			if (match_type == MatchType::PARTIAL_SINGLE || match_type == MatchType::FULL) {
				hs_scan(hs_databases[j], (char*)s, s_len, 0, scratch, single_match_handler, &matches);
                hs_result = (matches.size() == 2);
  			} else {
                // hs doesn't support greedy matching like PCRE, RE2 and our current implementation
                // it reports all matches as explained here
                // https://intel.github.io/hyperscan/dev-reference/compilation.html#semantics
                // it generally results in more matches, so we won't be using it to compare runtimes

                /*hs_scan(hs_databases[j], (char*)s, s_len, 0, scratch, all_matches_handler, &matches);
                hs_result = matches.size() / 2; // the number of partial matches*/
                
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
            } else if (match_type == MatchType::FULL) { 
                pcre_result = pcre_patterns[j].FullMatch(s);    
            } else {
                // find all partial matches
                string word;
                pcre_result = 0;
                pcrecpp::StringPiece input(s);
                while (pcre_patterns[j].FindAndConsume(&input, &word)) {
                    if (word.length() == 0) {
                        input.remove_prefix(1);    
                    } else {
                        pcre_result++;    
                    }
                }
            }
        }
        auto pcre_end = high_resolution_clock::now();
        float pcre_dur = (duration_cast<nanoseconds>(pcre_end - pcre_start)).count() * 1.0 / (1e6f * n_iters);
        
        // buildit timing
        float buildit_dur;
        int buildit_result = 0; 
        vector<Matcher> funcs = buildit_patterns[j];
        int n_funcs = funcs.size();
        if (n_funcs == 1) {
            auto buildit_start = high_resolution_clock::now();
            for (int i = 0; i < n_iters; i++) {
                buildit_result = funcs[0](s, s_len, 0);
            }
            auto buildit_end = high_resolution_clock::now();
            buildit_dur = (duration_cast<nanoseconds>(buildit_end - buildit_start)).count() * 1.0 / (1e6f * n_iters);
        } else {
            auto buildit_start = high_resolution_clock::now();
            for (int i = 0; i < n_iters; i++) {
                buildit_result = 0;
                #pragma omp parallel for
                for (int part_id = 0; part_id < n_funcs; part_id++) {
                    if (funcs[part_id](s, s_len, 0))
                        buildit_result = 1;
                }
            }
            auto buildit_end = high_resolution_clock::now();
            buildit_dur = (duration_cast<nanoseconds>(buildit_end - buildit_start)).count() * 1.0 / (1e6f * n_iters);
        }
        
        // check correctness
		const string& str = (match_type != MatchType::FULL) ? "<text>" : strings[j].substr(0, 10) + "...";
        if (match_type == MatchType::PARTIAL_ALL) {
            // we are not comparing times with Hyperscan in this mode
            // make its output correct by default
            hs_result = buildit_result;
        }
        
        if (!((hs_result == buildit_result || match_type == MatchType::FULL) && re_result == buildit_result)) {
            cout << endl << "Correctness failed for regex " << patterns[j] << " and text " << str << ":" <<endl;
            cout << "  re2 match = " << re_result << endl;
            cout << "  hs match = " << hs_result << endl;
            cout << "  pcre match = " << pcre_result << endl;
            cout << "  buildit match = " << buildit_result << endl;
        }

        cout << "--- pattern: " << patterns[j] << " text: " << str << " ---" << endl;
        cout << "buildit run time: " << buildit_dur << " ms" << endl;
        cout << "re2 run time: " << re2_dur << " ms" << endl;
        if (match_type != MatchType::PARTIAL_ALL)
            cout << "hs run time: " << hs_dur << " ms" << endl;
        cout << "pcre run time: " << pcre_dur << " ms" << endl;
    }

}

void optimize_partial_match_loop(string str, string pattern) {
    
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

void time_partial_single_split_ors(string &text, int n_iters) {
    
    vector<string> twain_patterns = {
        "(?SHuck[a-zA-Z]+|Saw[a-zA-Z]+)",
        "(?STom|Sawyer|Huckleberry|Finn)",
        "(.{0,2}(?STom|Sawyer|Huckleberry|Finn))",
        "(.{2,4}(?STom|Sawyer|Huckleberry|Finn))",
        "(?STom.{10,15}river|river.{10,15}Tom)",
    };
    /*for (string re: twain_patterns) {
        auto start = high_resolution_clock::now();
        MatchFunction func;
        for (int n = 0; n < n_iters; n++) {
            func = compile_split(text, re, 0, MatchType::PARTIAL_SINGLE, "");
        }
        auto end = high_resolution_clock::now();
        float compile_dur = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
        const char* c_text = text.c_str();
        start = high_resolution_clock::now();
        int result = 0;
        for (int n = 0; n < n_iters; n++) {
            result = func(c_text, text.length(), 0);
        }
        end = high_resolution_clock::now();
        float run_dur = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
        cout << "regex: " << re << endl;
        cout << "compile time: " << compile_dur << endl;
        cout << "run time: " << run_dur << endl;
        cout << "result: " << result << endl;
        cout << endl;
    }*/
    
}


void run_twain_benchmark() {
    int n_iters = 10;
    string corpus_file = "./data/twain.txt";
    string text = load_corpus(corpus_file);
    std::cout << "Twain Text Length: " << text.length() << std::endl;
    vector<string> twain_patterns = {
        "(Twain)",
        "(Huck[a-zA-Z]+|Saw[a-zA-Z]+)",
        "([a-q][^u-z]{5}x)", 
        "(Tom|Sawyer|Huckleberry|Finn)",
        "(.{0,2}(Tom|Sawyer|Huckleberry|Finn))",
        "(.{2,4}(Tom|Sawyer|Huckleberry|Finn))",
        "(Tom.{10,15})",
        "(Tom.{10,15}river|river.{10,15}Tom)",
        "([a-zA-Z]+ing)",
    };
    vector<int> n_funcs = {1, 1, 2, 1, 1, 1, 1, 4, 1};
    vector<int> decompose = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    RegexOptions parallel_flags_16;
    parallel_flags_16.interleaving_parts = 16;
    RegexOptions or_split_flags;
    or_split_flags.flags = ".s................s................";
    RegexOptions parallel_flags_2;
    RegexOptions groups;
    groups.flags = "....gggggggg.";
    parallel_flags_2.interleaving_parts = 2;
    RegexOptions flags;
    vector<RegexOptions> options = {
        flags,
        flags,
        parallel_flags_2,
        flags,
        flags,
        flags,
        flags,
        or_split_flags,
        flags
    };
    vector<string> words = {"Twain", "HuckleberryFinn", "qabcabx", "Sawyer", "Sawyer Tom", "SaHuckleberry", "Tom swam in the river", "Tom swam in the river", "swimming"};
    
    cout << "\n------- FULL MATCH -------\n" << endl;
    
    time_compare(twain_patterns, words, n_iters, MatchType::FULL, options);
    
    cout << "\n------- PARTIAL SINGLE -------\n" << endl;
    
    time_compare(twain_patterns, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, options);
    
    //cout << "\n------- PARTIAL SINGLE SPLIT ORS ----------\n" << endl;
    
    //time_partial_single_split_ors(text, n_iters);

/*
// trying to optimize partial matches
    for (string re: twain_patterns) {
        cout << endl;
        cout << re << endl;
        optimize_partial_match_loop(text, re + ".*");
    }*/
    
    //cout << "\n------- PARTIAL ALL -------\n" << endl;
	
    //time_compare(twain_patterns, vector<string>{text}, 1, MatchType::PARTIAL_ALL, options);
}

void run_re2_benchmark() {
    // benchmark taken from https://swtch.com/~rsc/regexp/regexp3.html
    int n_iters = 10;
    string text = generate_random_text(1000000); 
    RegexOptions flags;
    vector<RegexOptions> options = {flags};
    // full match
    time_compare(vector<string>{".*"}, vector<string>{text}, n_iters, MatchType::FULL, options);
    time_compare(vector<string>{"[0-9]+.(.*)"}, vector<string>{"650-253-0001"}, n_iters, MatchType::FULL, options);
    // partial match, no submatch
    time_compare(vector<string>{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"}, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, options);
    time_compare(vector<string>{"[XYZ]ABCDEFGHIJKLMNOPQRSTUVWXYZ"}, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, options);
    time_compare(vector<string>{"A[AB]B[BC]C[CD]D[DE]E[EF]F[FG]G[GH]H[HI]I[IJ]J"}, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, options);
    time_compare(vector<string>{"A(A|B)B(B|C)C(C|D)D(D|E)E(E|F)F(F|G)G(G|H)H(H|I)I(I|J)J"}, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, options);
    time_compare(vector<string>{"[ -~]*ABCDEFGHIJKLMNOPQRSTUVWXYZ"}, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, options);
    time_compare(vector<string>{"([ -~])*(A)(B)(C)(D)(E)(F)(G)(H)(I)(J)(K)(L)(M)(N)(O)(P)(Q)(R)(S)(T)(U)(V)(W)(X)(Y)(Z)"}, vector<string>{text}, n_iters, MatchType::PARTIAL_SINGLE, options);
    // partial match, with submatch
    //optimize_partial_match_loop(text + "650 253-0001", "(\\d{3}\\s+)(\\d{3}-\\d{4}).*");
    //time_compare(vector<string>{"(\\d{3}\\s+)(\\d{3}-\\d{4})"}, vector<string>{text + "650 253-0001"}, n_iters, MatchType::PARTIAL_SINGLE, vector<int>{16}, vector<int>{0});

}

int main() {
    string patterns_file = "./data/twain_patterns.txt";
    vector<string> patterns = load_patterns(patterns_file);
    run_twain_benchmark();
    //run_re2_benchmark();
}

