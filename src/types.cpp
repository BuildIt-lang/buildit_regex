#include "types.h"

const char set_t_name[] = "std::set";
const char map_t_name[] = "std::map";

dyn_var<set_t<int>(set_t<int>, set_t<int>)> set_t_union(builder::as_global("regex_runtime::get_union"));
dyn_var<void(set_t<int>, set_t<int>)> set_t_update(builder::as_global("regex_runtime::update_set"));
dyn_var<void(map_t<int, set_t<int>>, int, set_t<int>)> map_t_update(builder::as_global("regex_runtime::update_map"));
