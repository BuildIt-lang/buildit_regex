#include <iostream>
#include <fstream>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include "match.h"
#include "progress.h"

int main(int argc, char* argv[]) {
    builder::builder_context context;
    context.feature_unstructured = true;
    context.run_rce = true;
    std::ofstream code_file;
    code_file.open("generated_code/sample1.h");
    const int re_len = strlen("[abc]+");
    const int cache_size = (re_len + 1) * (re_len + 1);
    std::unique_ptr<int> cache_states_ptr(new int[cache_size]);
    int* next_states = cache_states_ptr.get();
    cache_states("[abc]+", next_states);
    auto ast = context.extract_function_ast(match_regex_full, "match_re", "[abc]+", next_states, 0);
    code_file << "#include <string.h>" << std::endl;
    block::c_code_generator::generate_code(ast, code_file);
    code_file.close();
    return 0;
}


