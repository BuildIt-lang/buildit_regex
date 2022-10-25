#ifndef TYPES_H
#define TYPES_H

#include "builder/dyn_var.h"
#include "builder/builder.h"

using builder::dyn_var;

const char vector_t_name[] = "std::vector";

template <typename T>
using vector_t = builder::name<vector_t_name, T>;

namespace builder {

template <typename T>
class dyn_var<vector_t<T>>: public dyn_var_impl<vector_t<T>> {
public:
    typedef dyn_var_impl<vector_t<T>> super;
    using super::super;
    using super::operator=;
    builder operator= (const dyn_var<vector_t<T>> &t) {
        return (*this) = (builder)t;
    }
    dyn_var(const dyn_var& t): dyn_var_impl<vector_t<T>>((builder)t){}
    dyn_var(): dyn_var_impl<vector_t<T>>() {}

    dyn_var<void(T)> push = as_member_of(this, "push_back");
    //dyn_var<void(void)> pop = as_member_of(this, "pop_back");
};
}

#endif
