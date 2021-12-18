
#include <string>
#include <string_view>
#include "codes//line_gen_codes.h"
#include <iostream>

#define var_t char*
#define var_ct std::string_view
#define var_t_null '\0'

#ifndef var_container
#define var_container std::vector
#endif

var_t generate_var(){
    return "$t0";
}

#define add_line(lines)\
    for (auto sv : lines) \
        std::cout << sv << '\n';

/*
void add_line(var_container<var_t> line){
    std::string result_line = "";
   
    for (var_t part : line)
    {
        switch (*part)
        {
        case VAR_CODE:
            result_line += generate_var();
            break;
        
        default:
            result_line += part;
            break;
        }
    }

    printf("%s\n", result_line.c_str());
}
*/