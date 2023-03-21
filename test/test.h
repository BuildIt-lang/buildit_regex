#include <regex>
#include <assert.h>
#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include "match.h"
#include <chrono>
#include "../include/parse.h"
#include "../include/progress.h"
#include "../include/compile.h"
#include <pcrecpp.h>

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
    bool ignore_case = (strcmp(flags, "i") == 0);

    bool expected = 0;
    string expected_word;
    string regex = pattern;
    pcrecpp::RE_Options pcre_opt;
    pcre_opt.set_caseless(ignore_case);
    pcrecpp::RE pcre_re("(" + regex + ")", pcre_opt);
    pcrecpp::StringPiece text(candidate);
    if (match_type == MatchType::FULL) {
        expected = pcre_re.FullMatch(text, &expected_word);    
    } else if (match_type == MatchType::PARTIAL_SINGLE) {
        expected = pcre_re.PartialMatch(text, &expected_word);
    }
    
    cout << "Matching " << pattern << " with " << candidate << " -> ";
    
    RegexOptions options;
    options.ignore_case = ignore_case;
    options.flags = groups;
    int result = match(pattern, candidate, options, match_type);
    
    bool match = (result == expected);
    if (match) {
        cout << "ok. Result is: " << result << endl;
    } else {
        cout << "failed\nExpected: " << expected << ", got: " << result << endl;
    }
    if (match_type == MatchType::PARTIAL_SINGLE) {
        string actual_match = first_longest(pattern, candidate, options);
        if (actual_match != expected_word) {
            cout << "expected: " << expected_word << " got: " << actual_match << endl;    
        }
    }

}

