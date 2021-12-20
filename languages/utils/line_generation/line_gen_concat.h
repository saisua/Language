
#include <string>
#include <string_view>
#include "codes//line_gen_codes.h"
#include <iostream>
#include <array>

#define var_t char*
#define var_ct std::string_view
#define var_t_null '\0'
#define new_line '\n'

#ifndef var_container
#define var_container std::array
#endif

constexpr inline var_t generate_var()
    __attribute__((always_inline))
    __attribute__((hot))
    __attribute__((flatten));
template <int size>
constexpr void add_line(var_container<var_t, size> line)
    __attribute__((always_inline))
    __attribute__((hot))
    __attribute__((flatten));


constexpr inline var_t generate_var(){
    return "$t0";
}

template <int size>
constexpr void add_line(var_container<var_t, size> line){
    std::string result_line = "";
   
    for (var_t part : line)
    {
        switch (*part)
        {
        case VAR_CODE:
            result_line += generate_var();
            break;
        
        default:
            result_line += new_line;
            result_line += part;
            break;
        }
    }

    #if LANG_VERBOSE
    printf("%s\n", result_line.c_str());
    #endif
}
