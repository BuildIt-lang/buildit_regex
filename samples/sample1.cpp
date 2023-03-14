#include "compile.h"

int main() {
    // full match
    string regex = "a(bc)*d";
    string str = "Abcbcd";
    
    // default options (ignore_case = 0, interleaving_parts = 1, no special flags)
    RegexOptions default_options;
    int is_match = match(regex, str, default_options, MatchType::FULL);
    cout << "regex: " << regex << " str: " << str << " match: " << is_match << endl;

    // ignore_case included
    RegexOptions ic_options;
    ic_options.ignore_case = 1;
    is_match = match(regex, str, ic_options, MatchType::FULL);
    cout << "regex: " << regex << " str: " << str << " match: " << is_match << endl;

}

