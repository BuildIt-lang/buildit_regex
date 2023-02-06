#include "test_all_partial.h"

/**
 Uses PCRE to find the longest match starting from each
 character in the text. PCRE FindAndConsume would be more
 appropriate for this, but in case of an empty string match it
 results in an infinite loop. In this function we manually
 advance the input to avoid an infinite loop.
*/
vector<string> get_pcre_submatches(string text, string regex) {
    vector<string> matches;
    pcrecpp::RE re(regex);
    pcrecpp::StringPiece input(text);
    string word;
    int text_len = text.length();
    for (int i = 0; i < text_len; i++) {
        // advance the input
        pcrecpp::StringPiece input(text.substr(i, text_len - i));
        if (re.Consume(&input, &word)) {
            matches.push_back(word);
        }
    }
    return matches;
}

/**
 Compare correctness against PCRE output.
*/
void check_correctness(string regex, string text, bool verbose) {
    vector<string> expected = get_pcre_submatches(text, "(" + regex + ")");
    vector<string> result = get_all_partial_matches(text, regex, "");
    cout << "Matching " << regex << " in " << text << ": ";
    
    if (verbose) {
        // print matches from buildit
        cout << endl << "---\nBuildit matches: " << endl;
        for (string m: result) {
            cout << m << endl;
        }
        cout << "---" << endl;
    }
    
    if (result.size() != expected.size()) {
        // wrong number of matches
        cout << "failed!" << endl;
        cout << "Incorrect number of matches: ";
        cout << "expected " << expected.size() << ", got " << result.size() << endl;
        return;
    }
   
    // check if all the matches are the same
    for (unsigned int i = 0; i < expected.size(); i++) {
        string exp = expected[i];
        string res = result[i];
        if (exp.compare(res) != 0) {
            cout << "failed!" << endl;
            cout << "Incorrect match at position " << i << ": ";
            cout << "expected " << exp << ", got: " << res << endl;
            return;
        }
    }
    cout << "ok" << endl;
}

int main() {
    string text = "abbbcc";
    check_correctness("b", text);
    check_correctness("b+", text);
    check_correctness("b*", text);
    check_correctness("b*c*", text);
    check_correctness("ab*", text);
    check_correctness("bc*", text);
    check_correctness("b+c+", text);
    check_correctness("ab*c*", text);
    
}
