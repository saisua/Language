// Internal includes
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <queue>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <chrono>
#include <array>
#include <regex>
#include <utility> 
#include <json11.hpp>

// External includes
#include <json11.hpp>
#include <omp.h>

// global compiler time settings
#ifndef LANG_DEBUG
#define LANG_DEBUG false
#endif
#ifndef LANG_TIMEIT
#define LANG_TIMEIT true
#endif

// Local includes
#include "src/regex_perm.cpp"
#include "src/Mreg-gen.cpp"
//#include "src/testing.cpp"
#include "src/step_gen.cpp"
#include "src/utils/file_read.cpp"


#include "src/syntax_gen.cpp"
#include "src/definition_gen.cpp"

#include "lang_compile.h"
#include "src/generated_tag_list.h"


// Forward declarations
std::string read_str_file(const std::string& filename);
inline void generate_syntax(Mreg_gen<uintptr_t> & data);
inline void generate_codes(Mreg_gen<uintptr_t> & data);
inline void generate_definition_checks(Mreg_gen<uintptr_t> & data, 
                    std::vector<json11::Json> & files);

int main(int argc, const char * argv[]){
    Mreg_gen<uintptr_t> generated_matcher = Mreg_gen<uintptr_t>();

    std::vector<json11::Json> files = std::vector<json11::Json>();
    {
    std::string err = "";

    for(std::string path : {LANG_GEN_RENAME_FROM}){
        printf("Reading file: %s\n", path.c_str());
        files.push_back(json11::Json::parse(
                    read_str_file(
                        std::regex_replace(path, std::regex("%s"), current_compilation)
                    ), err));
    }

    for(std::string path : {LANG_GEN_RENAME_TO}){
        printf("Reading file: %s\n", path.c_str());
        files.push_back(json11::Json::parse(
                    read_str_file(
                        std::regex_replace(path, std::regex("%s"), current_compilation)
                    ), err));
    }
    }

    printf("(1/3) Generate syntax\n");
    generate_syntax(generated_matcher);
    printf("(2/3) Generate codes\n");
    generate_codes(generated_matcher);
    printf("(3/3) Generate checks\n");
    generate_definition_checks(generated_matcher, files);

    return 0;
}