#include "partial.h"

vector<string> get_leftmost_longest_matches(string text, map<int, set<int>> end_to_start) {
    // get start to end mapping
    vector<int> start_positions;
    map<int, set<int>> start_to_end;
    map<int, set<int>>::iterator it;
    for (auto const& k: end_to_start) {
        int end = k.first;
        set<int> start_indices = k.second;
        for (int start: start_indices) {
            it = start_to_end.find(start);
            if (it  == start_to_end.end()) {
                set<int> s = {end};
                start_to_end.insert({start, s});
                start_positions.push_back(start);
            } else {
                it->second.insert(end);
            }
        }
    }
    /*cout << "start to end" << endl;
    for (auto const& k: start_to_end) {
        std::cout << k.first << ": ";
        for (int v: k.second) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    */
    // sort the start positions for leftmost matching
    sort(start_positions.begin(), start_positions.end());
     
    vector<string> matches;
    int last_end = -1;
    for (int i = 0; i < (int)start_positions.size(); i++) {
        int start = start_positions[i];
        if (start < last_end)
            continue;
        int end = *(start_to_end[start].rbegin());
        matches.push_back(text.substr(start, end - start));
        last_end = end;
    }
    
    return matches;
}
