
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

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

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

string generate_split_flags(string pattern) {
    string flags = "";
    int counter = 0;
    for (int i = 0; i < pattern.length(); i++) {
        if (false && i > 0 && (pattern[i-1] == '*' || pattern[i-1] == '+' || pattern[i-1] == '}')) {
            flags += "s";
            counter = 0;
        } else if (counter >= 8 && is_letter(pattern[i])) {
            flags += 's';
            counter = 0;    
        } else {
            flags += ".";
            counter++;
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

tuple<string, string> generate_snort_flags(string pattern, bool ignore_case, int gr_len) {
    if (ignore_case || pattern.find("\\x") != std::string::npos ||
        pattern.find("\"") != std::string::npos) {
        return tuple<string, string> {pattern, generate_split_flags(pattern)}; // split after *, +, }
    }
    string flags = "";
    string regex = "";
    int group_size = 0;
    for (int i = 0; i < pattern.length(); i++) {
        char c = pattern[i];
        if (group_size == gr_len) {
            group_size = 0;
            regex += ')';
            flags += '.';
        }
        if (group_size < gr_len && is_letter(c) && (i == 0 || pattern[i-1] != '\\') && (i == 0 || is_letter(pattern[i-1]) ||
                i == pattern.length() - 1 || is_letter(pattern[i+1]))) {
                    if (group_size == 0) {
                        regex += '(';
                        flags += '.';
                    }
                    flags += 'j';
                    group_size++;
                    regex += c;
        } else {
            if (group_size > 0) {
                group_size = 0;
                regex += ')';
                flags += '.';
            }
            group_size = 0;
            flags += '.';
            regex += c;
        }
    }
    if (group_size > 0) {
        regex += ')';
        flags += '.';
        
    }
    return tuple<string, string> {regex, flags};
}

// helper function to generate flags for teakettle
// 1. group all consecutive a-z chars when ignore_case = false
// 2. limit the group size to 8 chars
tuple<string, string> generate_teakettle_flags(string pattern, bool ignore_case, int gr_len) {
    if (ignore_case) {
        return tuple<string, string> {pattern, generate_split_flags(pattern)}; // split after *, +, }
    }
    string flags = "";
    string regex = "";
    int group_size = 0;
    for (int i = 0; i < pattern.length(); i++) {
        char c = pattern[i];
        if (group_size == gr_len) {
            group_size = 0;
            regex += ')';
            flags += '.';
        }
        if (group_size < gr_len && is_letter(c) && (i == 0 || is_letter(pattern[i-1]) ||
                i == pattern.length() - 1 || is_letter(pattern[i+1]))) {
                    if (group_size == 0) {
                        regex += '(';
                        flags += '.';
                    }
                    flags += 'j';
                    group_size++;
                    regex += c;
        } else {
            if (group_size > 0) {
                group_size = 0;
                regex += ')';
                flags += '.';
            }
            group_size = 0;
            flags += '.';
            regex += c;
        }
    }
    if (group_size > 0) {
        regex += ')';
        flags += '.';
        
    }
    return tuple<string, string> {regex, flags};

}

vector<vector<Matcher>> compile_buildit(vector<string> patterns, int n_patterns, int block_size, int batch_id, string dataset) {
    auto start = high_resolution_clock::now();
    vector<vector<Matcher>> compiled_patterns;
    for (int re_id = 2 * n_patterns * batch_id; re_id < n_patterns * 2 * (batch_id + 1); re_id = re_id + 2) {
        cout << "Compiling regex " << to_string(re_id / 2) << ": " << patterns[re_id] << endl;
        string re = patterns[re_id];
        string regex = (re[0] == '^') ? "^(" + re.substr(1, re.length() - 1) + ")" : "(" + re + ")";
        string flags = patterns[re_id + 1];
        cout << "Flags: " << flags << "; ";
        RegexOptions opt;
        opt.interleaving_parts = 1; // TODO: change this!!
        opt.binary = true; // we don't care about the specific match
        opt.ignore_case = (flags.find("i") != std::string::npos);
        opt.dotall = (flags.find("s") != std::string::npos);
        if (dataset == "teakettle") {
            tuple<string, string> tup = generate_teakettle_flags(regex, opt.ignore_case, 16);
            regex = get<0>(tup);
            opt.flags = get<1>(tup);
        } else {
            tuple<string, string> tup = generate_snort_flags(regex, opt.ignore_case, 16);
            regex = get<0>(tup);
            opt.flags = get<1>(tup);
            //opt.flags = "";
        }
        cout << "new regex: " << regex << endl;
        cout << "Regex flags: " << opt.flags << endl;
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
    cout << "n_iters: " << n_iters << endl;
    for (int iter = 0; iter < n_iters; iter++) {
        for (int i = 0; i < compiled_patterns.size(); i++) {
            vector<Matcher> funcs = compiled_patterns[i];
            int result = -1;
            if (block_size == -1) {
                if (funcs.size() == 1) {
                    result = funcs[0](s, s_len, 0);    
                } else {
                    result = -1;
                    //#pragma omp parallel for
                    for (int tid = 0; tid < funcs.size(); tid++) {
                        if (result > -1)
                            continue;
                        int curr_result = funcs[tid](s, s_len, 0);
                        if (curr_result > -1) {
                            result = curr_result;
                        }
                    }
                }
            } else {
                #pragma omp parallel for
                for (int chunk = 0; chunk < s_len; chunk = chunk + block_size) {
                    if (result > -1)
                        continue;
                    if (funcs.size() == 1) {
                        int curr_result = funcs[0](s, s_len, chunk);
                        if (curr_result > chunk - 1)
                            result = curr_result;
                    } else {
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


void run_pcre2(vector<string> patterns, string text, int n_iters, bool individual, int n_patterns, int batch_id) {
    // text info
    PCRE2_SPTR subject;
    subject = (PCRE2_SPTR)text.c_str();
    PCRE2_SIZE subject_length = (PCRE2_SIZE)text.length();
    cout << "length: " << subject_length << endl; 
    // compilation
    auto start = high_resolution_clock::now();
    vector<pcre2_code*> codes;
    vector<pcre2_match_data*> matches;
    for (int i = 2 * n_patterns * batch_id; i < n_patterns * 2 * (batch_id + 1); i = i + 2) {
        string reg = patterns[i];
        string regex = (reg[0] == '^') ? "^(" + reg.substr(1, reg.length() - 1) + ")" : "(" + reg + ")";
        regex = make_lazy(regex);
        string flags = patterns[i+1];
        bool ignore_case = flags.find('i') != std::string::npos;
        bool dotall = flags.find('s') != std::string::npos;
        
        // copmile pcre2 
        pcre2_code *re;
        PCRE2_SPTR pattern;   

        int errornumber;
        
        // add flags
        uint32_t option_bits = (ignore_case) ? PCRE2_CASELESS : 0;
        if (dotall) option_bits |= PCRE2_DOTALL;
        PCRE2_SIZE erroroffset;
        PCRE2_SIZE *ovector;

        pcre2_match_data *match_data;

        pattern = (PCRE2_SPTR)regex.c_str();
        re = pcre2_compile(
                pattern,               /* the pattern */
                PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
                option_bits,           /* flags */
                &errornumber,          /* for error number */
                &erroroffset,          /* for error offset */
                NULL);                 /* use default compile context */
        match_data = pcre2_match_data_create_from_pattern(re, NULL);
        codes.push_back(re);
        matches.push_back(match_data);
    }
    auto end = high_resolution_clock::now();
    float elapsed_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / 1e6f;
    cout << "PCRE2 compilation time: " << elapsed_time << "ms" << endl;

    // matching
    start = high_resolution_clock::now();
    auto last_end = start;
    for (int iter = 0; iter < n_iters; iter++) {
        for (int i = 0; i < codes.size(); i++) {
            int result = pcre2_match(
                    codes[i],             /* the compiled pattern */
                    subject,              /* the subject string */
                    subject_length,       /* the length of the subject */
                    0,                    /* start at offset 0 in the subject */
                    0,                    /* default options */
                    matches[i],           /* block for storing the result */
                    NULL);                /* use default match context */
            
            // convert result to 1 or 0
            result = (int)(result >= 0);
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
    cout << "PCRE2 running time: " << elapsed_time << "ms" << endl;
}


void run_pcre(vector<string> patterns, string text, int n_iters, bool individual, int n_patterns, int batch_id) {
    auto start = high_resolution_clock::now();
    vector<unique_ptr<pcrecpp::RE>> compiled_patterns;
    for (int i = 2 * n_patterns * batch_id; i < n_patterns * 2 * (batch_id + 1); i = i + 2) {
        string re = patterns[i];
        string regex = (re[0] == '^') ? "^(" + re.substr(1, re.length() - 1) + ")" : "(" + re + ")";
        regex = make_lazy(regex);
        string flags = patterns[i+1];
        pcrecpp::RE_Options opt;
        opt.set_dotall(flags.find('s') != std::string::npos);
        opt.set_caseless(flags.find('i') != std::string::npos);
        compiled_patterns.push_back(unique_ptr<pcrecpp::RE>(new pcrecpp::RE(regex, opt)));
    }
    auto end = high_resolution_clock::now();
    float elapsed_time = (duration_cast<nanoseconds>(end - start)).count() * 1.0 / 1e6f;
    cout << "PCRE compilation time: " << elapsed_time << "ms" << endl;

    start = high_resolution_clock::now();
    auto last_end = start;
    for (int iter = 0; iter < n_iters; iter++) {
        for (int i = 0; i < compiled_patterns.size(); i++) {
            int result = (*compiled_patterns[i]).PartialMatch(text);    
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
    cout << "PCRE running time: " << elapsed_time << "ms" << endl;
}

void run_hyperscan(vector<string> patterns, string text, int n_iters, bool individual, int n_patterns,  int batch_id) {
        
    char* s = (char*)text.c_str();
    int s_len = text.length();
    cout << "hs text len: " << s_len << endl;

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
        unsigned int hs_flags = HS_FLAG_SINGLEMATCH;
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
    int n_iters = 10;
    bool individual_times = true;
    int n_patterns = 20;
    int n_chunks = 1;
    if (run_teakettle) {
        int block_size = (int)(gutenberg.length() / n_chunks);
        if (n_chunks == 1) block_size = -1;
        cout << "block size: " << block_size << endl;
        vector<string> teakettle = load_patterns(data_dir + "pcre/teakettle_2500_small");
        string dataset = "teakettle"; 
        vector<vector<Matcher>> compiled_teakettle = compile_buildit(teakettle, n_patterns, block_size, batch_id, dataset); 
        run_buildit(compiled_teakettle, gutenberg, n_iters, individual_times, block_size, batch_id);
        run_pcre(teakettle, gutenberg, n_iters, individual_times, n_patterns, batch_id);
        run_re2(teakettle, gutenberg, n_iters, individual_times, n_patterns, batch_id);
        run_pcre2(teakettle, gutenberg, n_iters, individual_times, n_patterns, batch_id);
        run_hyperscan(teakettle, gutenberg, n_iters, individual_times, n_patterns, batch_id);
    }
    if (run_snort) {
        vector<string> snort_literals = load_patterns(data_dir + "pcre/snort_literals_small");
        string alexa = load_corpus(data_dir + "corpora/alexa200.txt");
        int block_size = (int)(alexa.length() / n_chunks);
        if (n_chunks == 1) block_size = -1;
        string dataset = "snort"; 
        
        vector<vector<Matcher>> compiled_snort = compile_buildit(snort_literals, n_patterns, block_size, batch_id, dataset); 
        run_buildit(compiled_snort, alexa, n_iters, individual_times, block_size, batch_id);
        run_pcre2(snort_literals, alexa, n_iters, individual_times, n_patterns, batch_id);
        run_pcre(snort_literals, alexa, n_iters, individual_times, n_patterns, batch_id);
        run_re2(snort_literals, alexa, n_iters, individual_times, n_patterns, batch_id);
        run_hyperscan(snort_literals, alexa, n_iters, individual_times, n_patterns, batch_id);
    }
}
