#include <regex>
#include <assert.h>
#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include "match.h"
#include <chrono>
#include "../include/frontend.h"

typedef int (*CompileFunction) (string, string, MatchType, int, string);
void check_correctness(const char* pattern, const char* candidate, const char* flags = "");                        
void check_split(const char* pattern, const char* candidate, int start_state = 0, const char* flags = "");                        
string remove_special_chars(string regex, char special);
void test_simple();
void test_star();
void test_brackets();
void test_brackets_and_star();
void test_negative_brackets();
void test_grouping();
void test_nested_grouping();
void test_or_groups();
void test_plus();
void test_question();
void test_repetition();
void test_combined();
void test_partial();
void test_escaping();
void test_expand_regex();
void test_ignore_case();
void test_split_or_groups();


