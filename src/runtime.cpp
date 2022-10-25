#include <vector>

template <typename T>
std::vector<T>* new_vector_ptr() {
    return new std::vector<T>;
}

