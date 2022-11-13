#ifndef TYPES_H
#define TYPES_H

#include "blocks/c_code_generator.h"
#include "builder/builder.h"
#include "builder/builder_context.h"
#include "builder/static_var.h"
#include "builder/dyn_var.h"

using builder::dyn_var;
using builder::as_member_of;

template <typename T>
void resize(T &t, int size) {block::to<block::array_type>(t.block_var->var_type)->size = size;}

extern const char set_t_name[];
extern const char map_t_name[];

template <typename T>
using set_t = builder::name<set_t_name, T>;


template <typename K, typename V>
using map_t = builder::name<map_t_name, K, V>;


namespace builder {
// Use specialization instead of inheritance
template <typename T>
class dyn_var<set_t<T>>: public dyn_var_impl<set_t<T>> {
    public:
        typedef dyn_var_impl<set_t<T>> super;
        using super::super;
        using super::operator=;
        builder operator= (const dyn_var<set_t<T>> &t) {
            return (*this) = (builder)t;
        }   
        dyn_var(const dyn_var& t): dyn_var_impl<set_t<T>>((builder)t){}
        dyn_var(): dyn_var_impl<set_t<T>>() {}

        dyn_var<void(T)> insert = as_member_of(this, "insert");
        dyn_var<void(void)> clear = as_member_of(this, "clear");
};

// Create specialization for foo_t* so that we can overload the * operator

template <typename T>
class dyn_var<set_t<T>*>: public dyn_var_impl<set_t<T>*> {
    public:
        typedef dyn_var_impl<set_t<T>*> super;
        using super::super;
        using super::operator=;
        builder operator= (const dyn_var<set_t<T>*> &t) {
            return (*this) = (builder)t;
        }   
        dyn_var(const dyn_var& t): dyn_var_impl<set_t<T>*>((builder)t){}
        dyn_var(): dyn_var_impl<set_t<T>*>() {}    

        dyn_var<set_t<T>> operator *() {
            // Rely on copy elision
            return (cast)this->operator[](0);
        }

        // This is POC of how -> operator can
        // be implemented. Requires the hack of creating a dummy 
        // member because -> needs to return a pointer. 
        dyn_var<set_t<T>> _p = as_member_of(this, "_p");
        dyn_var<set_t<T>>* operator->() {
            _p = (cast)this->operator[](0);
            return _p.addr();
        }
};

template <typename K, typename V>
class dyn_var<map_t<K,V>>: public dyn_var_impl<map_t<K,V>> {
    public:
        typedef dyn_var_impl<map_t<K,V>> super;
        using super::super;
        using super::operator=;
        builder operator= (const dyn_var<map_t<K,V>> &t) {
            return (*this) = (builder)t;
        }   
        dyn_var(const dyn_var& t): dyn_var_impl<map_t<K,V>>((builder)t){}
        dyn_var(): dyn_var_impl<map_t<K,V>>() {}
        dyn_var<int(void)> empty = as_member_of(this, "empty");
        
};

}

extern dyn_var<set_t<int>(set_t<int>, set_t<int>)> set_t_union;
extern dyn_var<void(set_t<int>, set_t<int>)> set_t_update;
extern dyn_var<void(set_t<int>, int)> set_t_insert;
extern dyn_var<map_t<int, set_t<int>>(map_t<int, set_t<int>>, int, set_t<int>)> map_t_update;

#endif
