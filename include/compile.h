#ifndef COMPILE_H
#define COMPILE_H

#include "frontend.h"
#include "match_with_schedule.h"
#include <tuple>

struct RegexOptions {
    bool ignore_case = false;
    int interleaving_parts = 1;
    string flags = "";
};
typedef int (*Matcher) (const char*, int, int, char*, char*);

Schedule get_schedule_options(string regex, RegexOptions regex_options);
pair<Matcher, int> compile(string regex, RegexOptions flags, MatchType match_type);
int match(string regex, string str, RegexOptions flags, MatchType match_type);

#endif
