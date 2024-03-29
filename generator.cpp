// Load the corresponding tree
// Load the file passed in the args
// Match the file (as const char *)
// For every match, pass its match_id
//   to a switch containing all definition
//   instructions
// call def_trans_optimize()
// Pass the match to the translator
// (which probably will be another switch)
// Add all lines "unfolded" and pass them
//   to the language
// call trans_lang_optimize()
// Apply all lines and "sew" them correctly
// call language_optimize()

#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include "src/utils/file_read.cpp"
#include "src/utils/cllist.cpp"

#include "languages/current_language.h"
#include "src/Mreg.cpp"


#define var_container std::vector

/*
var_t generate_var();
void add_line(var_container<var_t>);
var_t new_struct_name();
void def_trans_optimize();
void trans_lang_optimize();
void language_optimize();
*/
template<int size>
constexpr void add_line(std::array<var_ct, size> lines)
    __attribute__((always_inline));

int main(int argc, const char ** argv) {
    if(argc < 2){
        printf("Minimal usage: compile <filename>\n");
        return -1;
    }

    //std::fstream tree;
    //tree.open(LANG_SYNTAX_PATH 
    //            LANG_SYNTAX_TREES_FOLDER
    //            LANG_FROM
    //            ".tree"
    //            , std::ios::in);

    using mreg_t = uintptr_t;
    Mreg<mreg_t> mreg = Mreg<mreg_t>();
    //printf("Loading syntax tree in %s\n", LANG_SYNTAX_PATH 
    //                                        LANG_SYNTAX_TREES_FOLDER
    //                                        LANG_FROM
    //                                        ".tree");
    //mreg.load(tree);
    //tree.close();

    //mreg.test();

    std::string file = read_str_file(argv[1]);

    printf("Matching file %s %s\n", argv[1], file.c_str());
    // This does only accept only one line
    mreg.match_and_subgroups(file.c_str());

    mreg_t path = 0, pos = 0;
    C_linked_list<uintptr_t> & data = mreg.result_subgr;
    std::stack<mreg_t, std::vector<mreg_t>> todo_mreg = std::stack<mreg_t, std::vector<mreg_t>>();
    std::stack<void *, std::vector<void *>> todo_jump = std::stack<void *, std::vector<void *>>();

    definition_generated(data)
    end_definition_label:

    return 0;
}

template<int size>
constexpr void add_line(std::array<var_ct, size> lines){
    for (auto sv : lines) {
        printf("%s\n", sv);
    }
}