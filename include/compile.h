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
    bool ignore_case = false;
    int interleaving_parts = 1;
    string flags = "";
    bool start_anchor = false;
    bool last_eom = false;
    bool reverse = false;
};
typedef int (*Matcher) (const char*, int, int);

Matcher compile_helper(const char* regex, const char* flags, bool partial, int* cache, int part_id, Schedule schedule, string headers, int start_state);
Schedule get_schedule_options(string regex, RegexOptions regex_options);
vector<Matcher> compile(string regex, RegexOptions flags, MatchType match_type);
int match(string regex, string str, RegexOptions flags, MatchType match_type);
vector<string> get_all_partial_matches(string str, string regex, RegexOptions options);
int eom_to_binary(int eom, int str_start, int str_len, MatchType match_type, RegexOptions options);
int run_matchers(vector<Matcher>& funcs, string str, int str_start, RegexOptions options, MatchType match_type);
string first_longest(string regex, string str, RegexOptions opt);
#endif
