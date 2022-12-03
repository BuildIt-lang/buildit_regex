#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>
#include <src/hs.h>

using namespace std;
using namespace std::chrono;

int single_match_handler(unsigned int id, unsigned long long from, 
                    unsigned long long to, unsigned int flags, void *context);
int all_matches_handler(unsigned int id, unsigned long long from, 
                    unsigned long long to, unsigned int flags, void *context);
vector<string> get_matches(string &text, vector<int> &matches);
void hs_time(vector<string> &patterns, string &text, int n_iters, bool all_matches);


