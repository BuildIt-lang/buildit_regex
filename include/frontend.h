#ifndef FRONTEND_H
#define FRONTEND_H

#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include <string>
#include "match.h"
#include "progress.h"
#include "parse.h"
#include "all_partial.h"

using namespace std;

typedef int (*MatchFunction) (const char*, int, int);
typedef int (*PartialMatchFunction) (const char*, int, int, int);

enum MatchType { FULL, PARTIAL_SINGLE, PARTIAL_ALL };

MatchFunction compile_regex(const char* regex, int* cache, MatchType match_type, int tid, int n_threads, string flags);

bool run_matcher(MatchFunction* funcs, const char* str, int n_threads);

int compile_and_run(string str, string regex, MatchType match_type, int n_threads, string flags);

int compile_and_run_decomposed(string str, string regex, MatchType match_type, int n_threads, string flags);

int compile_and_run_partial(string str, string regex, string flags);

int partial_match_loop(const char* str, int str_len, int stride, MatchFunction func);

vector<string> get_all_partial_matches(string str, string regex, string flags);

#endif
