#ifndef RUNTIME_H
#define RUNTIME_H

#include "types.h"
#include <set>
#include <algorithm>


namespace regex_runtime {
    template <typename T>
    std::set<T> get_union(std::set<T> s1, std::set<T> s2) {
        std::set<T> result;
        std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(result, result.begin()));
        return result;
    }
}

#endif
