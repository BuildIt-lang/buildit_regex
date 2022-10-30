
#include <iostream>
#include <fstream>
#include <cstring>
#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "blocks/c_code_generator.h"
#include "builder/builder_dynamic.h"
#include <set>
#include "types.h"
#include "runtime.h"

using builder::dyn_var;
using builder::static_var;


static void test_set(int size) {
    dyn_var<set_t<int>[]> set_arr;
    resize(set_arr, size);
    dyn_var<set_t<int>> s1 = {3};
    //s1.insert(2);
    dyn_var<set_t<int>> s2 = {2, 4};
    //s2.insert(3);
    //s2.insert(5);
    //dyn_var<set_t<int>> s3 = set_t_union(s1, s2);
}

int main(int argc, char* argv[]) {
    builder::builder_context context;
    context.feature_unstructured = true;
    //context.run_rce = true;
    std::ofstream code_file;
    code_file.open("generated_code/sample2.h");
    auto ast = context.extract_function_ast(test_set, "test_set", 5);
    code_file << "#include <set>\n#include <algorithm>" << std::endl;
    block::c_code_generator::generate_code(ast, code_file);
    code_file.close();
    return 0;
}
