#include <iostream>
#include <fstream>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include "match.h"

int main(int argc, char* argv[]) {
    builder::builder_context context;
    context.feature_unstructured = true;
    context.run_rce = true;
    std::ofstream code_file;
    code_file.open("generated_code/sample1.h");
    const int re_len = strlen("[abc]+");
    std::unique_ptr<char> cache_ptr(new char[re_len + 1]);
    const int cache_size = (re_len + 1) * (re_len + 1);
    std::unique_ptr<int> cache_states_ptr(new int[cache_size]);
    std::unique_ptr<int> next_state_ptr(new int[re_len]);
    int *next_state = next_state_ptr.get();

    std::unique_ptr<int> brackets_ptr(new int[re_len]);
    int *brackets = brackets_ptr.get(); // hold the opening and closing indices for each bracket pair

    std::unique_ptr<int> helper_states_ptr(new int[re_len]);
    int *helper_states = helper_states_ptr.get();
    char* cache = cache_ptr.get();
    for (int i = 0; i < re_len + 1; i++) cache[i] = 0;
    int* cache_states = cache_states_ptr.get();
    auto ast = context.extract_function_ast(match_regex_full, "match_re", "[abc]+", false, cache, cache_states, next_state, brackets, helper_states);
    code_file << "#include <string.h>" << std::endl;
    block::c_code_generator::generate_code(ast, code_file);
    code_file.close();
    return 0;
}


