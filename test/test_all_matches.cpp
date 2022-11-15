#include <iostream>
#include "test.h"
#include "../include/parse.h"
#include "../include/progress.h"
#include "../include/partial.h"

#include <re2/re2.h>
#include <re2/stringpiece.h>

using namespace re2;

/**
General function to compare results.
*/
void check_correctness(const char* pattern, const char* candidate) {
    // expected
    string regex(pattern);
    string text(candidate);
    vector<string> re2_matches = get_re2_matches(regex, text);
    
    // result
    vector<string> buildit_matches = get_buildit_matches(regex, text);
    
    bool correct = compare_matches(re2_matches, buildit_matches);
    cout << "Searching for " << regex << " in " << text << " -> ";
    if (correct) {
        cout << "ok. Number of matches: " << buildit_matches.size() << endl;    
    } else {
        cout << "failed." << endl;
        cout << "Expected: ";
        print_matches(re2_matches);
        cout << "Got: ";
        print_matches(buildit_matches);
    }
    cout << endl;

}

void print_matches(vector<string> &matches) {
    for (string m: matches) {
        cout << "\"" << m << "\" ";    
    }
    cout << endl;
    
}

bool compare_matches(vector<string> &expected, vector<string> &result) {
    if (expected.size() != result.size())
        return false;
    for (int i = 0; i < (int)expected.size(); i++) {
        if (expected[i].compare(result[i]) != 0)
            return false;
    }
    return true;
}

vector<string> get_re2_matches(string pattern, string text) {
    RE2 re_pattern("(" + pattern + ")");
    StringPiece str(text);
    string match;
    vector<string> matches;
    while (RE2::FindAndConsume(&str, re_pattern, &match)) {
        string word = match;
        if (word.length() == 0 && matches.size() > 0 && matches.back().length() == 0) break;
        matches.push_back(word);
    }
    return matches;
}

vector<string> get_buildit_matches(string pattern, string text) {
    string p = expand_regex(pattern);
    const int re_len = p.length();
    // precompute a state cache
    const int cache_size = (re_len + 1) * (re_len + 1);
    int* next = new int[cache_size];
    cache_states(p.c_str(), next);
    
    builder::builder_context context;
    context.dynamic_use_cxx = true;
    context.dynamic_header_includes = "#include <set>\n#include <map>\n#include \"../../include/runtime.h\"";
    context.feature_unstructured = true;
	context.run_rce = true;
    
    auto fptr = (std::map<int, std::set<int>> (*)(const char*, int))builder::compile_function_with_context(context, find_all_matches, p.c_str(), next);
    std::map<int, std::set<int>> result = fptr(text.c_str(), text.length());
    delete[] next;

    vector<string> all_matches = get_leftmost_longest_matches(text, result);
    return all_matches;
    
}

void test_partial() {
	check_correctness("ab", "aaba");
    check_correctness("ab", "aba");
	check_correctness("a?", "aaaa");
	check_correctness("a+", "aaaa");
	check_correctness("[ab]", "ab");
    check_correctness("a?", "bb");
    check_correctness("aa", "aaa");
	check_correctness("c[ab]", "ca");
	check_correctness("c[ab]+", "abc");
	check_correctness("c[ab]+", "aaba");
	check_correctness("c[ab]+", "caaaabcc");
	check_correctness("123", "a123a");
	check_correctness("(123)*1", "112312311");
    check_correctness("Twain", "MarkTwainTwainTomSawyer");
}

int main() { 
    test_partial();
}
