#include "compile.h"
#include <sstream>

int main(int argc, char *argv[])
{
    // int match(string regex, string str, RegexOptions options, MatchType match_type, string *submatch). the arguments that main must take

    // command = [
    //     "./src/compile.cpp",
    //     regex_string,
    //     input,
    //     str(regex_options),
    // ]
    if (argc != 6)
    {
        std::cerr << "Usage: " << argv[0] << " <regex> <input_string> <regex_options> <match_type>" << std::endl;
        return 1;
    }

    std::string regex = argv[1];
    std::string input_string = argv[2];
    RegexOptions options;
    MatchType match_type = static_cast<MatchType>(std::stoi(argv[4]));
    // Parse the regex options from the stringified dictionary
    std::string regex_options_str = argv[3];
    std::stringstream ss(regex_options_str);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        std::stringstream ss_item(item);
        std::string key;
        std::string value;
        std::getline(ss_item, key, ':');
        std::getline(ss_item, value, ':');
        if (key == "ignore_case")
        {
            options.ignore_case = (value == "True") ? true : false;
        }
        else if (key == "dotall")
        {
            options.dotall = (value == "True") ? true : false;
        }
        else if (key == "interleaving_parts")
        {
            options.interleaving_parts = std::stoi(value);
        }
        else if (key == "flags")
        {
            options.flags = value;
        }
        else if (key == "greedy")
        {
            options.greedy = (value == "True") ? true : false;
        }
        else if (key == "binary")
        {
            options.binary = (value == "True") ? true : false;
        }
        else if (key == "block_size")
        {
            options.block_size = std::stoi(value);
        }
    }

    std::string submatch = argv[5];
    int result = match(regex, input_string, options, match_type, &submatch);

    // Check the result and print the submatch if there is a match
    if (result)
    {
        std::cout << "Match found." << std::endl;
    }
    else
    {
        std::cout << "No match found." << std::endl;
    }

    return 0;
}