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
    /**int expected = std::regex_search(candidate, std::regex(pattern));
    int len = strlen(candidate); 
	
    // parse the regex
    string processed_re = expand_regex(pattern);
    const int re_len = processed_re.length();
    
    // fill the cache
    const int cache_size = (re_len + 1) * (re_len + 1); 
    int* next = new int[cache_size];
    cache_states(processed_re.c_str(), next);

    builder::builder_context context;
	context.dynamic_use_cxx = true;
    context.dynamic_header_includes = "#include <set>\n#include <map>\n#include \"../../include/runtime.h\"";
    context.feature_unstructured = true;
	context.run_rce = true;
    
    auto fptr = (std::map<int, std::set<int>> (*)(const char*, int))builder::compile_function_with_context(context, find_all_matches, processed_re.c_str(), next);
    std::map<int, std::set<int>> result = fptr((char*)candidate, len);
    delete[] next;
    
    std::cout << "Matching " << pattern << " with " << candidate << " -> ";
    bool match = (!result.empty() == expected);
    if (match) {
        std::cout << "ok. Result is: " << !result.empty() << std::endl;
    } else {
        std::cout << "failed\nExpected: " << expected << ", got: " << !result.empty() << std::endl;
    }
    std::cout << "All matches: " << std::endl;
    for (auto const& k: result) {
        std::cout << k.first << ": ";
        for (int v: k.second) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;*/
    string regex(pattern);
    string text(candidate);
    vector<string> re2_matches = get_re2_matches(regex, text);
    cout << pattern << " " << text << endl;
    for (string s: re2_matches) {
        cout << s << ", ";    
    }
    cout << endl;

    cout << "buildit" << endl;
    vector<string> buildit_matches = get_buildit_matches(regex, text);
    for (string s: buildit_matches) {
        cout << s << ", ";    
    }
    cout << endl;
}

vector<string> get_re2_matches(string pattern, string text) {
    RE2 re_pattern("(" + pattern + ")");
    StringPiece str(text);
    vector<string> matches;
    string match;
    int n_matches = 0;
    while (RE2::FindAndConsume(&str, re_pattern, &match)) {
        string word = match;
        matches.push_back(word);
        //cout << "m: " << match << endl;
        n_matches += 1;
    }
    cout << "Number of matches: " << n_matches << endl;
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
    std::cout << "All matches: " << std::endl;
    for (auto const& k: result) {
        std::cout << k.first << ": ";
        for (int v: k.second) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
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
    check_correctness("Twain", "MarkTwainTwainTomSawyer");
	check_correctness("(123)*1", "112312311");
	check_correctness("(123|23*)", "123333");
    //test_partial();
}
