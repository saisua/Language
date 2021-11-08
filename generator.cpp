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

#include <vector>
#include <string>
#include <fstream>
#include "src/utils/file_read.cpp"
#include "src/Mreg.cpp"
#include "src/utils/cllist.cpp"

#include "languages/current_language.h"


#define var_container std::vector

/*
var_t generate_var();
void add_line(var_container<var_t>);
var_t new_struct_name();
void def_trans_optimize();
void trans_lang_optimize();
void language_optimize();
*/
void add_line(var_container<var_t> lines)
	__attribute__((always_inline));

int main(int argc, char const *argv[]) {
    if(argc < 1){
        printf("Minimal usage: compile <filename>");
        return -1;
    }

    std::fstream tree;
    tree.open(LANG_LANGUAGES_FOLDER 
                LANG_SYNTAX_PATH 
                LANG_SYNTAX_TREE_FOLDER
                LANG_FROM
                ".tree"
                , std::ios::in);

    Mreg<uint_fast32_t> mreg = Mreg<uint_fast32_t>();
    mreg.load(tree);
    tree.close();

    std::string file = read_str_file(argv[1]);
    // This does only accept only one line
    mreg.match_and_subgroups(file.c_str());

    uint_fast32_t path = 0, pos = 0;
    C_linked_list<uintptr_t> & data = mreg.result_subgr;

    switch(data[0]){
        definition_generated
    }

    return 0;
}

void add_line(var_container<var_t> lines){
	for(var_t & v : lines){
		printf("%s\n", v);
	}
}