#include "compile.h"

int main() {
    // binary partial match
    string regex = "(ab|cd|de)1*23";
    string text = "4561b23cd123";

    RegexOptions default_options;
    int is_match = match(regex, text, default_options, MatchType::PARTIAL_SINGLE);
    cout << "DEFAULT regex: " << regex << " text: " << text << " match: " << is_match << endl;

    // splitting on | (mark each state immediately after | with s)
    RegexOptions split_options;
    split_options.flags = ".s..s..s......"; // should be the same length as the regex
    is_match = match(regex, text, split_options, MatchType::PARTIAL_SINGLE);
    cout << "OR SPLIT regex: " << regex << " text: " << text << " match: " << is_match << endl;

    // group multiple states into one
    RegexOptions group_options;
    group_options.flags = "..........gggg"; // same length as the regex, (1*23) are grouped together
    is_match = match(regex, text, group_options, MatchType::PARTIAL_SINGLE);
    cout << "GROUPING regex: " << regex << " text: " << text << " match: " << is_match << endl;

    // interleaving
    RegexOptions options;
    // each generated function starts matching from
    // every second position of the text
    // number of generated functions = number of interleaving parts
    options.interleaving_parts = 2;
    is_match = match(regex, text, options, MatchType::PARTIAL_SINGLE);
    cout << "INTERLEAVING regex: " << regex << " text: " << text << " match: " << is_match << endl;
    
    
}

