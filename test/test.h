#include <regex>
#include <assert.h>
#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include "match.h"
#include <chrono>

void print_expected_all_matches(const char* pattern, const char* candidate);
void check_correctness(const char* pattern, const char* candidate);                                                                                   
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
void test_expand_regex();
