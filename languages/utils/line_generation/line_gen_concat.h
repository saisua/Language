
#include <string>
#include "codes//line_gen_codes.h"

#define var_t char *
#define var_t_null '\0'

#ifndef var_container
#define var_container std::vector
#endif

var_t generate_var();

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
}\
*/