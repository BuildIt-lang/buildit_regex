#ifndef RUNTIME_H
#define RUNTIME_H

#include <set>
#include <algorithm>


namespace regex_runtime {
    
    /**
    Returns a new set which is a union of `s1` and `s2`.
    */
    template <typename T>
    std::set<T> get_union(std::set<T> s1, std::set<T> s2) {
        std::set<T> result;
        std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(result, result.begin()));
        return result;
    }

    /**
    Inserts the elements from `s2` into `s1`.
    */
    template <typename T>
    void update_set(std::set<T> s1, std::set<T> s2) {
        s1.insert(s2.begin(), s2.end());    
    }
}

#endif
