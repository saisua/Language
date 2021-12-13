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

    std::vector<std::string> from_path = {LANG_GEN_RENAME_FROM};
    for (std::string path : from_path)
    {
        printf("Reading file: %s\n", path.c_str());
        files.push_back(json11::Json::parse(
                    read_str_file(
                        std::regex_replace(path, std::regex("%s"), LANG_FROM)
                    ), err));
    }

    std::vector<std::string> to_path = {LANG_GEN_RENAME_TO};
    for(std::string path : to_path){
        printf("Reading file: %s\n", path.c_str());
        files.push_back(json11::Json::parse(
                    read_str_file(
                        std::regex_replace(path, std::regex("%s"), LANG_TO)
                    ), err));
    }
    }

    std::string compile_info = read_str_file("lang_compile.h");
    compile_info += "\n\n";
    compile_info += read_str_file("src/generated_tag_list.h");
    compile_info += "\n\n#include \"" LANG_SYNTAX_FOLDER LANG_SYNTAX_TREES_FOLDER LANG_FROM ".tree.h\"";

    // Clean current_lang
    std::ofstream current_lang;
    current_lang.open(LANG_LANGUAGES_FOLDER "current_language.h", std::ios::out | std::ofstream::trunc);
    current_lang << compile_info + "\n\n";
    current_lang.close();

    printf("(1/5) Generate syntax\n");
    generate_syntax(generated_matcher, files[0]);
    printf("(2/5) Generate codes\n");
    generate_codes(generated_matcher);
    //generate_states(generated_matcher, files);
    printf("(3/5) Generate checks\n");
    generate_definition_checks(generated_matcher, files);
    printf("(4/5) Resolve dependencies\n");
    generate_utils_dependencies(files);
    printf("(5/5) Generate compile time syntax tree\n");
    generated_matcher.clean_gen();
    std::ofstream constexpr_syntax;
    constexpr_syntax.open(LANG_SYNTAX_PATH LANG_SYNTAX_TREES_FOLDER LANG_FROM ".tree.h", std::ios::out);
    generated_matcher.generate_constexpr<uint16_t>(constexpr_syntax);
    printf("[ DONE ]\n");

    return 0;
}