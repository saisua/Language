#ifndef SHARED_VAR_LPPL_CPP
#define SHARED_VAR_LPPL_CPP

#include <cstdint>
#include <list>

struct share_var_t {
    char * name;
    uint_fast32_t type;
    std::list<uintptr_t> value {};
    bool named;
    bool callable;
};

#endif