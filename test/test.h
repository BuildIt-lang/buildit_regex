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
void test_extra(MatchType type);

string make_lazy(string greedy_regex);

string make_lazy(string greedy_regex) {
    string lazy = "";
    for (int i = 0; i < (int)greedy_regex.length(); i++) {
        lazy += greedy_regex[i];
        char c = greedy_regex[i];
        if (c == '*' || c == '+' || c == '?' || c == '}')
            lazy += "?";
    }
    return lazy;
}

void compare_result(const char* pattern, const char* candidate, string groups, MatchType match_type, const char* flags) {
    string sflags = flags;
    bool ignore_case = (sflags.find("i") != std::string::npos);
    bool include_newline = (sflags.find("s") != std::string::npos);
    bool greedy = true;

    bool expected = 0;
    string expected_word;
    string regex = pattern;
    if (!greedy)
        regex = make_lazy(regex);
    pcrecpp::RE_Options pcre_opt;
    pcre_opt.set_caseless(ignore_case);
    pcre_opt.set_dotall(include_newline);
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
    options.dotall = include_newline;
    options.flags = groups;
    options.binary = true;
    // binary match
    int result = match(pattern, candidate, options, match_type);
    
    bool is_match = (result == expected);
    if (is_match) {
        cout << "ok. Result is: " << result << endl;
    } else {
        cout << "failed\nExpected: " << expected << ", got: " << result << endl;
    }
    if (match_type == MatchType::PARTIAL_SINGLE) {
        string actual_match;
        options.greedy = greedy;
        options.binary = false;
        match(pattern, candidate, options, match_type, &actual_match);
        if (actual_match != expected_word) {
            cout << "expected: " << expected_word << " got: " << actual_match << endl;    
        }
    }

}

