
void add_pattern(std::vector<std::map<std::string, std::string>>& patterns, std::string regex, std::string interleaving_parts, std::string ignore_case, std::string flags, std::string text) {
    std::map<std::string, std::string> config;
    config["regex"] = regex;
    config["interleaving_parts"] = interleaving_parts;
    config["ignore_case"] = ignore_case;
    config["flags"] = flags;
    config["text"] = text;
    patterns.push_back(config);
}


std::vector<std::map<std::string, std::string>> get_pattern_config(std::string match_type) {
    std::vector<std::map<std::string, std::string>> patterns;
    
    if (match_type == "full") {
/*        add_pattern(patterns, "(Twain)", "1", "0", "", "Twain");
        add_pattern(patterns, "(Twain)", "1", "1", "", "Twain");
        add_pattern(patterns, "([a-z]shing)", "1", "0", "", "ishing");
        add_pattern(patterns, "(Huck[a-zA-Z]+|Saw[a-zA-Z]+)", "1", "0", "", "Huckleberry");
        add_pattern(patterns, "([a-q][^u-z]{13}x)", "1", "0", "", "abbbbbcccccdddx"); 
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "1", "0", "", "Tom");
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "1", "0", "", "Finn");
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "1", "1", "", "Sawyer");
        add_pattern(patterns, "(.{0,2}(Tom|Sawyer|Huckleberry|Finn))", "1", "0", "", "Finn"); 
        add_pattern(patterns, "(.{2,4}(Tom|Sawyer|Huckleberry|Finn))", "1", "0", "", "aaTom"); 
        add_pattern(patterns, "([a-zA-Z]+ing)", "1", "0", "", "washing");
        add_pattern(patterns,  "(\\s[a-zA-Z]{0,12}ing\\s)", "1", "0", "", " swimming ");
        add_pattern(patterns, "(([A-Za-z]awyer|[A-Za-z]inn)\\s)", "1", "0", "", "Sawyer");
        add_pattern(patterns, "(Tom.{10,15}river|river.{10,15}Tom)", "1", "0", "", "Tom swimming river");
        add_pattern(patterns, "(Tom.{10,25}river|river.{10,15}Tom)", "1", "0", "", "Tom swimming river");
  */      
    } else {
        add_pattern(patterns, "(TomddsaSawyerdsad)", "1", "0", "", "");
        //add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "1", "0", "", "");
    /*    add_pattern(patterns, "(Twain)", "1", "0", "", "");
        add_pattern(patterns, "(Twain)", "1", "1", "", "");
        add_pattern(patterns, "([a-z]shing)", "1", "0", "", "");
        add_pattern(patterns, "(Huck[a-zA-Z]+|Saw[a-zA-Z]+)", "1", "0", "", "");
        add_pattern(patterns, "(Huck[a-zA-Z]+|Saw[a-zA-Z]+)", "2", "0", "", "");
        add_pattern(patterns, "(Huck[a-zA-Z]+|Saw[a-zA-Z]+)", "1", "0", ".s.............s............", "");
        //add_pattern(patterns, "([a-q][^u-z]{13}x)", "4", "0", "", ""); 
        //add_pattern(patterns, "([a-q][^u-z]{13}x)", "16", "0", "", ""); 
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "1", "0", "", "");
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "2", "0", "", "");
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "1", "0", ".s...s......s...........s....", "");
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "1", "1", "", "");
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "2", "1", "", "");
        add_pattern(patterns, "(Tom|Sawyer|Huckleberry|Finn)", "1", "1", ".s...s......s...........s....", "");
        add_pattern(patterns, "(.{0,2}(Tom|Sawyer|Huckleberry|Finn))", "1", "0", "", ""); 
        add_pattern(patterns, "(.{0,2}(Tom|Sawyer|Huckleberry|Finn))", "2", "0", "", ""); 
        add_pattern(patterns, "(.{0,2}(Tom|Sawyer|Huckleberry|Finn))", "1", "0", "........s...s......s...........s.....", ""); 
        add_pattern(patterns, "(.{2,4}(Tom|Sawyer|Huckleberry|Finn))", "1", "0", "", ""); 
        add_pattern(patterns, "(.{2,4}(Tom|Sawyer|Huckleberry|Finn))", "2", "0", "", ""); 
        add_pattern(patterns, "(.{2,4}(Tom|Sawyer|Huckleberry|Finn))", "1", "0", "........s...s......s...........s.....", ""); 
        add_pattern(patterns, "([a-zA-Z]+ing)", "1", "0", "", "");
        //add_pattern(patterns,  "(\\s[a-zA-Z]{0,12}ing\\s)", "1", "0", "", "");
        //add_pattern(patterns,  "(\\s[a-zA-Z]{0,12}ing\\s)", "2", "0", "", "");
        //add_pattern(patterns,  "(\\s[a-zA-Z]{0,12}ing\\s)", "4", "0", "", "");
        add_pattern(patterns, "(([A-Za-z]awyer|[A-Za-z]inn)\\s)", "1", "0", "", "");
        add_pattern(patterns, "(([A-Za-z]awyer|[A-Za-z]inn)\\s)", "2", "0", "", "");
        add_pattern(patterns, "(([A-Za-z]awyer|[A-Za-z]inn)\\s)", "1", "0", "..ssssssss......ssssssss.......", "");
        add_pattern(patterns, "(Tom.{10,15}river|river.{10,15}Tom)", "16", "0", "", "");
        add_pattern(patterns, "(Tom.{10,15}river|river.{10,15}Tom)", "32", "0", "", "");
        
        add_pattern(patterns, "(Tom.{10,15}river|river.{10,15}Tom)", "1", "0", ".s................s................", "");
        //add_pattern(patterns, "(Tom.{10,25}river|river.{10,15}Tom)", "16", "0", "", "");
        //add_pattern(patterns, "(Tom.{10,25}river|river.{10,15}Tom)", "32", "0", "", "");
        //add_pattern(patterns, "(Tom.{10,25}river|river.{10,15}Tom)", "1", "0", ".s................s................", "");
    */
    }
    return patterns;
}

