#include <regex>
#include <assert.h>
#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include "match.h"
#include <chrono>
#include "../include/frontend.h"
#include "../include/compile.h"

using namespace std::chrono;

void test_flag_expand(string regex, string inp_flags, string expected);
void compare_result(const char* pattern, const char* candidate, string groups, MatchType match_type, const char* flags="");

void expand_flags_tests();
void test_simple(MatchType type);
void test_star(MatchType type);
void test_brackets(MatchType type);
void test_or_groups(MatchType type);
void test_combined(MatchType type);
void test_repetition(MatchType type);
void test_or_split(MatchType type);
void test_brackets_and_star(MatchType type);
void test_negative_brackets(MatchType type);
void test_grouping(MatchType type);
void test_nested_grouping(MatchType type);
void test_or_groups(MatchType type);
void test_plus(MatchType type);
void test_question(MatchType type);
void test_partial(MatchType type);
void test_escaping(MatchType type);
void test_expand_regex();
void test_ignore_case(MatchType type);


void compare_result(const char* pattern, const char* candidate, string groups, MatchType match_type, const char* flags) {
    bool expected = 0;
    if (match_type == MatchType::FULL) {
    	expected = (strcmp(flags, "i") == 0) ? 
            regex_match(candidate, regex(pattern, regex_constants::icase)) :
            regex_match(candidate, regex(pattern));
    } else if (match_type == MatchType::PARTIAL_SINGLE) {
        expected = (strcmp(flags, "i") == 0) ? 
            regex_search(candidate, regex(pattern, regex_constants::icase)) :
            regex_search(candidate, regex(pattern));
    }
    RegexOptions options;
    if (strcmp(flags, "i") == 0)
        options.ignore_case = true;
    options.flags = groups;
    int result = match(pattern, candidate, options, match_type);
    
    cout << "Matching " << pattern << " with " << candidate << " -> ";
    bool match = (result == expected);
    if (match) {
        cout << "ok. Result is: " << result << endl;
    } else {
        cout << "failed\nExpected: " << expected << ", got: " << result << endl;
    }
}

