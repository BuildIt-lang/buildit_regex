#include <regex>
#include <assert.h>
#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include "match.h"
#include <chrono>
#include <vector>
#include "../include/compile.h"
using namespace std::chrono;

void test_group_states(string re, vector<int> expected);
string remove_special_chars(string regex, char special);
void compare_result(const char* pattern, const char* candidate, string groups, MatchType match_type, const char* flags="");
void test_group_states(string re, vector<int> expected);
void group_states_tests();
void test_simple(MatchType type);
void test_star(MatchType type);
void test_brackets(MatchType type);
void test_or_groups(MatchType type);
void test_combined(MatchType type);
void test_repetition(MatchType type);
void test_or_split(MatchType type);
