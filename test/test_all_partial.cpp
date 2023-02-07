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
    while (re.FindAndConsume(&input, &word)) {
        if (word.length() == 0) {
            // advance the input pointer manually
            // to avoid an infinite loop
            input.remove_prefix(1);    
        } else {
            matches.push_back(word);
        }
    }
    return matches;
}

string load_corpus(string fname) {
    ifstream input_file;
    input_file.open(fname);
    string text =  string((istreambuf_iterator<char>(input_file)), istreambuf_iterator<char>());   
    input_file.close();
    return text;
}
/**
 Compare correctness against PCRE output.
*/
void check_correctness(string regex, string text, bool verbose) {
    vector<string> expected = get_pcre_submatches(text, "(" + regex + ")");
    vector<string> result = get_all_partial_matches(text, regex, "");
    string display_text = (text.length() > 20) ? "<text>" : text;
    cout << "Matching " << regex << " in " << display_text << ": ";
    
    if (verbose) {
        // print matches from buildit
        cout << endl << "---\nBuildit matches: " << endl;
        for (string m: result) {
            cout << m << endl;
        }
        cout << "---" << endl;
        // print expected matches
        cout << endl << "---\nexpected matches: " << endl;
        for (string m: expected) {
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

void check_twain() {
    string twain = load_corpus("./benchmarks/data/twain.txt");
    vector<string> twain_patterns = {
        "(Twain)",
        "(Huck[a-zA-Z]+|Saw[a-zA-Z]+)",
        "([a-q][^u-z]{5}x)", 
        "(Tom|Sawyer|Huckleberry|Finn)",
        "(.{0,2}(Tom|Sawyer|Huckleberry|Finn))",
        "(.{2,4}(Tom|Sawyer|Huckleberry|Finn))",
        "(Tom.{10,15})",
        "(Tom.{10,15}river|river.{10,15}Tom)",
        "([a-zA-Z]+ing)",
    };
    for (string pattern: twain_patterns) {
        check_correctness(pattern, twain);    
    }
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

    check_twain();
}
