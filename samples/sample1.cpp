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
    auto ast = context.extract_function_ast(match_regex, "match_re", "[abc]+");
    code_file << "#include <string.h>" << std::endl;
    block::c_code_generator::generate_code(ast, code_file);
    code_file.close();
    return 0;
}


