
#include <fstream>
#include <string>
#include <json11.hpp>
#include <regex>
#include <cmath>

#include "regex_perm.cpp"
#include "generated_tag_list.h"

#define call 0
#define upper_call 1
#define var 2
#define arg 3
#define result 4
#define struct 5

#define base "\", \\\n\t\""
#define base_type "\", \\\n\t\t\""

#define tag_regex std::regex(Language_match_symbol "(.*?)" Language_match_symbol)

std::string get_string_from_path(json11::Json &doc, const std::string &path);
std::vector<std::string> get_recursive_strings(json11::Json &doc, const std::string path);
bool solve_tags(std::string &code, json11::Json &doc, std::regex ref_regex);
void solve_calls(std::string &code, std::vector<std::regex> &patterns, json11::Json &doc);
void solve_vars_args(std::string &code, std::vector<std::regex> &patterns, json11::Json &doc);
int offset_vars(std::string new_line, uint max_var, std::vector<std::regex> &patterns);

std::vector<std::regex> generate_patterns(json11::Json &file)
{
    printf("Generating patterns...\n");
    return {
        std::regex(get_string_from_path(file, "Reserved-regex_Call")),
        std::regex(get_string_from_path(file, "Reserved-regex_Upper-all")),
        std::regex(get_string_from_path(file, "Reserved-regex_Variable")),
        std::regex(get_string_from_path(file, "Reserved-regex_Arguments")),
        std::regex(get_string_from_path(file, "Reserved-regex_Result")),
        std::regex(get_string_from_path(file, "Reserved-regex_Struct"))
    };
}

void solve_code(std::string & code, std::vector<std::regex> & patterns, json11::Json & doc){
    printf("Solving language code...\n");
    // Get patterns as an attr not to execute generate_patterns every single line of code
    std::smatch ref_match, ref_call, ref_upper, ref_var, ref_arg;

    // First, solve code references from the files. 
    solve_tags(code, doc, tag_regex);

    // Then, match the patterns, replacing them with the correct id.
    solve_calls(code, patterns, doc);

    solve_vars_args(code, patterns, doc);
    printf("\nCode solved!\n\n");
}

bool solve_tags(std::string &code, json11::Json &doc, std::regex ref_regex){
    printf("-> tags ");
    std::smatch ref_match;
    bool solved = false;
    // Replace all references with the content of the file.
    while(std::regex_match(code, ref_match, ref_regex)){
        code.assign(ref_match.prefix().str());
        for(std::string & code_replacement : get_recursive_strings(doc, ref_match[1]))
            code += base + code_replacement + base;

        code += ref_match.suffix().str();

        solved = true;
    }

    return solved;
}

void solve_calls(std::string & code, std::vector<std::regex> & patterns, json11::Json & doc){
    printf("-> calls ");
    // Get the max argument value
    int max_var = 0;
    int tmp_max_var;

    std::smatch ref_call;

    if(std::regex_match(code, ref_call, patterns[var])){
        std::string remaining_code = ref_call.suffix().str();
        while(std::regex_match(remaining_code, ref_call, patterns[var])){
            max_var = std::max(max_var, std::stoi(ref_call[1]));
            remaining_code = ref_call.suffix().str();
        }
    }

    std::string tmp_code = std::string(code);
    std::vector<std::string> new_code;
    while (std::regex_match(tmp_code, ref_call, patterns[call]))
    {
        tmp_code.assign(ref_call.prefix().str());
        new_code = get_recursive_strings(doc, ref_call[1]);

        // Offset vars to keep them unique
        for(std::string & new_line : new_code)
            tmp_max_var = offset_vars(new_line, max_var, patterns);

        // If we need to solve the result in the call, do it now.
        if(ref_call[3].str().length() && ref_call[3].str() != "{result}")
            for(std::string & new_line : new_code)
                new_line = std::regex_replace(new_line, std::regex("\\{result\\}"), ref_call[3].str());

        std::smatch ref_arg;

        std::vector<std::string> arg_values = std::vector<std::string>();
        std::string tmp_arg_values = std::string(ref_call[5].str());
        while (std::regex_search(tmp_arg_values, ref_arg, std::regex("(@|\\{).*?\\[\\d+\\](@|\\})")))
        {
            if(ref_arg[0].str().length() >= 5){
                arg_values.emplace_back(ref_arg[0].str());

            }
            tmp_arg_values.assign(ref_arg.suffix().str());
        }
        if(tmp_arg_values.length() >= 5) // @[0]@
            arg_values.emplace_back(tmp_arg_values);

        // If the call takes any argument, solve them now.
        if(arg_values.size())
            for (uint val_num = 0; val_num != arg_values.size(); ++val_num)
                if(!std::regex_match(arg_values[val_num], std::regex("\\{arg\\["+std::to_string(val_num+1)+"\\]\\}"))){
                    for(std::string & new_line : new_code)
                        new_line = std::regex_replace(new_line, std::regex("\\{arg\\["+std::to_string(val_num+1)+"\\]\\}"), arg_values[val_num]);
                }

        max_var = tmp_max_var;
    }

    // Now that we have all our code solved, we can solve again the code, searching
    // any other tag->call references.
    for(std::string & new_line : new_code){
        printf("\nRecursive: nl: %s\n", new_line.c_str());
        if(solve_tags(new_line, doc, tag_regex))
            solve_calls(new_line, patterns, doc);
    }

    // Finally, replace the code with the new code.
    // since all reference has been recursively solved.
    if(new_code.size()){
        code = "";
        for(std::string & new_line : new_code)
            code += new_line;
    }
}

void solve_vars_args(std::string & code, std::vector<std::regex> & patterns, json11::Json & doc){
    printf("-> vars/args\n");
    std::string tmp_code = std::string(code);
    code = "";

    std::smatch ref_var;

    // Solve vars
    while(std::regex_search(tmp_code, ref_var, patterns[var])){
        if (ref_var.prefix().str().length()){
            if(code.length())
                code += base;
            code += ref_var.prefix().str() + base_type;
        } else  if(code.length()) 
            code += base_type;

        code += "\\";
        code += std::to_string(var + 1).append("\"\"").append(ref_var[1].str());

        tmp_code.assign(ref_var.suffix().str());
    }
    if(tmp_code.length() && code.length())
        tmp_code.assign(code + base + tmp_code);
    else if(code.length())
        tmp_code.assign(code);
    code = "";

    // Solve args
    while(std::regex_search(tmp_code, ref_var, patterns[arg])){
        if (ref_var.prefix().str().length()){
            if(code.length())
                code += base;
            code += ref_var.prefix().str() + base_type;
        }   
        code += "\\";
        code += std::to_string(arg + 1).append("\"\"").append(ref_var[1].str());

        
        tmp_code.assign(ref_var.suffix().str());
    }
    
    if(tmp_code.length())
        if(code.length() && code.back() != '\"' && tmp_code[0] != '\"'){
            code += base + tmp_code;
        }
        else
            code += tmp_code;
}

int offset_vars(std::string new_line, uint max_var, std::vector<std::regex> & patterns){
    printf("-> offset vars ");
    uint new_var;
    uint new_max_vars = max_var;

    std::smatch ref_var;

    if(std::regex_match(new_line, ref_var, patterns[var]))
    {
        new_line.assign(ref_var.prefix().str() + "@var[");

        new_var = std::stoi(ref_var[1]) + max_var;

        new_max_vars = std::max(new_max_vars, new_var);

        new_line += std::to_string(new_var) + "]@";

        std::string tmp_line = ref_var.suffix().str();
        while(std::regex_match(tmp_line, ref_var, patterns[var])){
            new_var = std::stoi(ref_var[1]) + max_var;

            new_max_vars = std::max(new_max_vars, new_var);

            new_line += ref_var.prefix().str();
            new_line += "@var[" + std::to_string(new_var) + "]@";

            tmp_line = ref_var.suffix().str();
        }
        new_line += tmp_line;
    }

    return new_max_vars;
}