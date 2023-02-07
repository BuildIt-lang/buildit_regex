#include <vector>
#include <pcrecpp.h>
#include "frontend.h"

vector<string> get_pcre_submatches(string text, string regex);
void check_correctness(string regex, string text, bool verbose = false);
string load_corpus(string fname);
void check_twain();
