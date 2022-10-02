#include <regex>
#include <assert.h>
#include "blocks/c_code_generator.h"
#include "builder/builder_context.h"
#include "builder/builder_dynamic.h"
#include "match.h"

void check_correctness(const char* pattern, const char* candidate);                                                                                   
void test_simple();
void test_star();
void test_brackets();
void test_brackets_and_star();
