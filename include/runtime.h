#ifndef RUNTIME_H
#define RUNTIME_H

#include <set>
#include <map>
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
    void update_set(std::set<T> &s1, std::set<T> &s2) {
        s1.clear();
        s1.insert(s2.begin(), s2.end());
        s2.clear();
    }
    
    template <typename T>
    void set_insert(std::set<T> &s1, T elt) {
        s1.insert(elt);    
    }

    /**
    Insert a key value pair in the map.
    */
    template <typename K, typename V>
    std::map<K,V> update_map(std::map<K,V> m, K key, V val) {
        m.insert({key, val});
        return m;
    }
}

#endif