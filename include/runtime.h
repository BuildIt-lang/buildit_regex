#ifndef RUNTIME_H
#define RUNTIME_H

#include "builder/dyn_var.h"
#include "types.h"

using builder::dyn_var;

//dyn_var<dyn_var<vector_t<int>>*(void)> new_vector_t("new_vector_ptr");

template <typename T>
std::vector<T>* new_vector_ptr();

#endif
