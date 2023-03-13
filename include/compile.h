#ifndef COMPILE_H
#define COMPILE_H

#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include "frontend.h"
#include "match_with_schedule.h"
#include <tuple>

struct RegexOptions {
    bool ignore_case = false;
    int interleaving_parts = 1;
    string flags = "";
};
typedef int (*Matcher) (const char*, int, int);

Matcher compile_helper(const char* regex, const char* flags, bool partial, int* cache, int part_id, Schedule schedule, string headers);
Schedule get_schedule_options(string regex, RegexOptions regex_options);
vector<Matcher> compile(string regex, RegexOptions flags, MatchType match_type);
int match(string regex, string str, RegexOptions flags, MatchType match_type);

#endif
