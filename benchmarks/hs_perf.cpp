#include "hs_perf.h"

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

/**
 Returns the substrings from `text` that were matched.
*/
vector<string> get_matches(string &text, vector<int> &matches) {
    vector<string> result;
    for (int i = 0; i < (int)matches.size(); i+=2) {
        int from = matches[i];
        int to = matches[i + 1];
        result.push_back(text.substr(from, to - from));
    }
    return result;
}

void hs_time(vector<string> &patterns, string &text, int n_iters, bool all_matches) {
    int text_len = text.length();
    char *text_arr = new char[text_len];
    strcpy(text_arr, text.c_str());
    int dur = 0;
    for (int i = 0; i < n_iters; i++) {
        for (string pattern: patterns) {
            int pattern_len = pattern.length();
            char pattern_arr[pattern_len];
            strcpy(pattern_arr, pattern.c_str());

            // a structure to hold the matches
            vector<int> matches;

            // compile the pattern
            hs_database_t *database;
            hs_compile_error_t *compile_err;
            hs_scratch_t *scratch = NULL;
            auto start = high_resolution_clock::now();
            hs_compile(pattern_arr, HS_FLAG_SOM_LEFTMOST, HS_MODE_BLOCK, NULL, &database, &compile_err);
            hs_alloc_scratch(database, &scratch);
            
            // do the matching
            auto event_handler = all_matches ? all_matches_handler : single_match_handler;
            hs_scan(database, text_arr, text_len, 0, scratch, event_handler, &matches);
            auto end = high_resolution_clock::now();
            dur += (duration_cast<milliseconds>(end - start)).count();
            hs_free_scratch(scratch);
            hs_free_database(database);
            
            // print matches
            bool print_matches = false;
            if (print_matches) {
                vector<string> str_matches = get_matches(text, matches);
                cout << "matches: ";
                for (int i = 0; i < (int)str_matches.size(); i++) {
                    cout << str_matches[i] << ", ";    
                }
                cout << endl;
            }
        }
    }
    delete[] text_arr;
    printf("Hyperscan average time: %f ms\n", dur * 1.0 / (n_iters * patterns.size()));
}
