#ifndef COMPILE_H
#define COMPILE_H

#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include <string>
#include "match.h"
#include "progress.h"
#include "parse.h"
#include "all_partial.h"
#include "match_with_schedule.h"
#include <tuple>

using namespace std;

enum MatchType { FULL, PARTIAL_SINGLE, PARTIAL_ALL };

struct RegexOptions {
    bool ignore_case = false; // same as the PCRE i flag
    bool dotall = false; // same as the PCRE s flag
    int interleaving_parts = 1;
    string flags = "";
    bool greedy = false;
    bool binary = true;
    int block_size = -1; // split the string into blocks
};
typedef int (*Matcher) (const char*, int, int);

Matcher compile_helper(const char* regex, const char* flags, bool partial, int* cache, int part_id, Schedule schedule, string headers, int start_state);
Schedule get_schedule_options(string regex, RegexOptions regex_options, MatchType match_type, int pass);
tuple<vector<Matcher>, vector<Matcher>> compile(string regex, RegexOptions flags, MatchType match_type, bool two_pass);
int match(string regex, string str, RegexOptions flags, MatchType match_type, string* submatch = nullptr);
vector<string> get_all_partial_matches(string str, string regex, RegexOptions options);
int eom_to_binary(int eom, int str_start, int str_len, MatchType match_type, Schedule options);
vector<int> run_matchers(vector<Matcher>& funcs, string str, int str_start, Schedule options, MatchType match_type, bool binary);
string generate_headers(string regex, MatchType match_type, RegexOptions options);
vector<Matcher> compile_single_pass(string orig_regex, string regex, RegexOptions options, MatchType match_type, int pass, string flags, int* cache);
void print_cache(int* cache, int re_len);
#endif
