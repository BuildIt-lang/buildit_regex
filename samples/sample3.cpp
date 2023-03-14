#include "compile.h"

int main() {
    // get all partial matches    
    string regex = "(ab|cd|de)1*23";
    string text = "4561ab23cd123";

    RegexOptions options;
    vector<string> matches = get_all_partial_matches(text, regex, options);
    cout << "regex: " << regex << " text: " << text << endl;
    cout << "found " << matches.size() << " matches: ";
    for (string match: matches)
        cout << match << " ";
    cout << endl;
}

