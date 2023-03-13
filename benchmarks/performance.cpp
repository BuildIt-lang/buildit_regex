#include <iostream>
#include <fstream>
#include <string>
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
#include "frontend.h"
#include <pcrecpp.h>
#include "compile.h"

using namespace std;
using namespace re2;
using namespace std::chrono;
using namespace builder;

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


tuple<int, float, float> buildit_time(string text, string regex, RegexOptions options, MatchType match_type, int n_iters) {
    
    // compile
    auto start = high_resolution_clock::now();
    vector<Matcher> funcs = compile(regex, options, match_type);
    for (int iter = 0; iter < n_iters; iter++)
        funcs = compile(regex, options, match_type);
    auto end = high_resolution_clock::now();
    float compile_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);

    // run
    int result = 0;
    const char* s = text.c_str();
    int s_len = text.length();
    if (options.interleaving_parts == 1) {
        // only one function to call
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++)
            result = funcs[0](s, s_len, 0);
        end = high_resolution_clock::now();
    } else {
        // interleave / parallelize
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++) {
            result = 0;
            #pragma omp parallel for
            for (int part_id = 0; part_id < options.interleaving_parts; part_id++) {
                if (funcs[part_id](s, s_len, 0))
                    result = 1;
            }
        }
        end = high_resolution_clock::now();
    }
    float run_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    return tuple<int, float, float>{result, compile_time, run_time};
}

tuple<int, float, float> re2_time(string text, string regex, MatchType match_type, int n_iters) {
   
    // compile
    RE2 re2_pattern(regex);
    auto start = high_resolution_clock::now();
    for (int iter = 0; iter < n_iters; iter++)
        RE2 re2_pattern_copy(regex);
    auto end = high_resolution_clock::now();
    float compile_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);

    // run
    int result = 0;
    if (match_type == MatchType::FULL) {
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++)
            result = RE2::FullMatch(text, re2_pattern);
        end = high_resolution_clock::now();
    } else if (match_type == MatchType::PARTIAL_SINGLE) {
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++)
            result = RE2::PartialMatch(text, re2_pattern);
        end = high_resolution_clock::now();
    } else {
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++) {
            // find all partial matches
            string word;
            result = 0;
            StringPiece input(text);
            while (RE2::FindAndConsume(&input, re2_pattern, &word)) {
                if (word.length() == 0) {
                    // advance the input pointer by 1
                    // otherwise it will be stuck in an infinite loop
                    input.remove_prefix(1);
                } else {
                    result++;
                }
            }
        }
        end = high_resolution_clock::now();
    }

    float run_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f *  n_iters);
    return tuple<int, float, float>{result, compile_time, run_time}; 
}

tuple<int, float, float> hyperscan_time(string text, string regex, MatchType match_type, int n_iters) {
    
    char* re_cstr = (char*)regex.c_str();
    char* s = (char*)text.c_str();
    int s_len = text.length();

    // compile
    hs_database_t *database;
    hs_compile_error_t *compile_error;
    hs_compile(re_cstr, HS_FLAG_SOM_LEFTMOST, HS_MODE_BLOCK, NULL, &database, &compile_error);
    auto start = high_resolution_clock::now();
    for (int iter = 0; iter < n_iters; iter++) {
        hs_database_t *database_copy;
        hs_compile_error_t *compile_error_copy;
        hs_compile(re_cstr, HS_FLAG_SOM_LEFTMOST, HS_MODE_BLOCK, NULL, &database_copy, &compile_error_copy);
    }
    auto end = high_resolution_clock::now();
    float compile_time = (duration_cast<nanoseconds>(end - start)).count() / (1e6f * n_iters);

    // run
    int result = 0;
    if (match_type == MatchType::PARTIAL_SINGLE || match_type == MatchType::FULL) {
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++) {
            vector<int> matches;
            hs_scratch_t *scratch = NULL;
            hs_alloc_scratch(database, &scratch);
            hs_scan(database, s, s_len, 0, scratch, single_match_handler, &matches);
            result = (matches.size() == 2);
        }
        end = high_resolution_clock::now();
    } else {
        // PARTIAL_ALL
        // hs doesn't support greedy matching like PCRE, RE2 and our current implementation
        // it reports all matches as explained here
        // https://intel.github.io/hyperscan/dev-reference/compilation.html#semantics
        // it generally results in more matches, so we won't be using it to compare runtimes

        /*hs_scan(hs_databases[j], (char*)s, s_len, 0, scratch, all_matches_handler, &matches);
        hs_result = matches.size() / 2; // the number of partial matches*/
        
    }
    float run_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    return tuple<int, float, float>{result, compile_time, run_time};
}

tuple<int, float, float> pcre_time(string text, string regex, MatchType match_type, int n_iters) {
    
    //compile 
    pcrecpp::RE pcre_re(regex);
    auto start = high_resolution_clock::now();
    for (int iter = 0; iter < n_iters; iter++)
        pcrecpp::RE pcre_re_copy(regex);
    auto end = high_resolution_clock::now();
    float compile_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);

    // run
    int result = 0;
    const char* s = text.c_str();
    if (match_type == MatchType::PARTIAL_SINGLE) {
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++)
            result = pcre_re.PartialMatch(s);
        end = high_resolution_clock::now();
    } else if (match_type == MatchType::FULL) { 
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++)
            result = pcre_re.FullMatch(s);
        end = high_resolution_clock::now();
    } else {
        // find all partial matches
        start = high_resolution_clock::now();
        for (int iter = 0; iter < n_iters; iter++) {
            string word;
            result = 0;
            pcrecpp::StringPiece input(s);
            while (pcre_re.FindAndConsume(&input, &word)) {
                if (word.length() == 0) {
                    input.remove_prefix(1);    
                } else {
                    result++;    
                }
            }
        }
        end = high_resolution_clock::now();
    }
    float run_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / (1e6f * n_iters);
    
    return tuple<int, float, float>{result, compile_time, run_time};
}

void compare_matches(string regex, string text, int buildit_res, int re2_res, int hs_res, int pcre_res) {
    bool all_match = (buildit_res == re2_res) && (re2_res == hs_res) && (hs_res == pcre_res);
    if (all_match)
        return;
    cout << endl << "Correctness failed for regex " << regex << " and text " << text << ":" <<endl;
    cout << "  buildit match = " << buildit_res << endl;
    cout << "  re2 match = " << re2_res << endl;
    cout << "  hs match = " << hs_res << endl;
    cout << "  pcre match = " << pcre_res << endl;

}

void print_times(string regex, string text, string lib_name, string match_type, tuple<int, float, float> result, RegexOptions options) {
    printf("matcher: %s, regex %s, text: %s, match type: %s\n", lib_name.c_str(), regex.c_str(), text.c_str(), match_type.c_str());
    if (lib_name == "buildit")
        printf("config: (interleaved_parts, %d), (ignore_case, %d), (flags, %s)\n", options.interleaving_parts, options.ignore_case, options.flags.c_str());
    printf("compile time: %f ms, run time: %f ms\n", get<1>(result), get<2>(result));
}

void compare_all(int n_iters, string text, string match) {
    vector<map<string, string>> patterns = get_pattern_config(match);
    MatchType match_type = (match == "partial") ? MatchType::PARTIAL_SINGLE : MatchType::FULL;
    for (auto config: patterns) {
        string regex = config["regex"];
        // if full take the string to match from the config
        if (match == "full") {
            text = config["text"];    
        }
        string text_to_print = (match == "full") ? text : "<Twain text>";
        // set buildit flags
        RegexOptions options;
        options.interleaving_parts = stoi(config["interleaving_parts"]);
        options.ignore_case = stoi(config["ignore_case"]);
        options.flags = config["flags"];
        cout << regex << options.flags << endl;
        tuple<int, float, float> buildit_result = buildit_time(text, regex, options, match_type, n_iters);
        tuple<int, float, float> re2_result = re2_time(text, regex, match_type, n_iters);
        tuple<int, float, float> hs_result = hyperscan_time(text, regex, match_type, n_iters);
        tuple<int, float, float> pcre_result = pcre_time(text, regex, match_type, n_iters);
        compare_matches(regex, text_to_print, get<0>(buildit_result), get<0>(re2_result), get<0>(hs_result), get<0>(pcre_result));
        print_times(regex, text_to_print, "buildit", match, buildit_result, options);
        cout << endl;
        print_times(regex, text_to_print, "re2", match, re2_result, options);
        cout << endl;
        print_times(regex, text_to_print, "hyperscan", match, hs_result, options);
        cout << endl;
        print_times(regex, text_to_print, "pcre", match, pcre_result, options);
        cout << endl << endl;

        // TODO: pass the ignore case flag to the other libs
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

void run_twain_benchmark() {
    int n_iters = 10;
    string corpus_file = "./data/twain.txt";
    string text = load_corpus(corpus_file);
    std::cout << "Twain Text Length: " << text.length() << std::endl;
    compare_all(n_iters, text, "full");
    compare_all(n_iters, text, "partial");
}

/*void run_re2_benchmark() {
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

}*/

int main() {
    string patterns_file = "./data/twain_patterns.txt";
    vector<string> patterns = load_patterns(patterns_file);
    run_twain_benchmark();
    //run_re2_benchmark();
}

