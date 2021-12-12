#ifndef LANG_GEN_COMPILE_CPP
#define LANG_GEN_COMPILE_CPP

// Internal includes
#include <stdio.h>
#include <bits/stdc++.h>
#include <string.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <regex>
#include <stdexcept>

#include "lang_compile.h"
#include "src/step_gen.cpp"
#include "src/regex_perm.cpp"

int find_argv(int argc, const char * argv[], const char * search)
    __attribute__((always_inline));
/*
inline void generate_step_char(std::fstream& out,
                            const std::string & from_lang_name,
                            const std::string & to_lang_name,
                            const std::vector<std::string>& from_filenames = {{LANG_GEN_RENAME_FROM}},
                            const std::vector<std::string>& to_filenames = {{LANG_GEN_RENAME_TO}},
                            const std::string& tag_path="Reserved-regex_Tag",
                            const std::string& trans_access="Reserved-regex_Line-Attribute-Access");
*/


int main(int argc, const char * argv[]){
    int errors = 0;

    std::string from, to;

    if(argc < 5){
        /*
        printf("Minimal usage: %s -from <language> -to <language>\n", argv[0]);
        return 1;
        */
        from = "aucpp";
        to = "mips";
    } else {
        printf("Setting up preparations:\n\n");
        from = std::string(argv[find_argv(argc, argv, "-from") + 1]);
        to = std::string(argv[find_argv(argc, argv, "-to") + 1]);
    }
    printf("  %s -> %s\n\n", from.c_str(), to.c_str());

    generate_step_char(from, to);

    printf("Starting the compilation of the language...\n");
    // Compile the files (unnecessary if not testing)
    errors += system("./builder2.sh");
    // Run the binary of the compiler builder
    errors += system("./builder");

    printf("Language compilated.\n");
    printf("Generating the compiler...\n");

    // Compile3 will load the mreg and start
    // passing values to 
    errors += system("./generator3.sh");
    return errors // + system("./language");
        ;
}   

// Returns n if found, 0 otherwise.
// It is meant to take argc and argv as arguments.
// As so, the first argument will always be the name of the program.
int find_argv(int argc, const char * argv[], const char * search){
    for(int flag = 1; flag != argc; ++flag)
        if(strcmp(argv[flag], search) == 0)
            return flag;

    return 0;
}

#endif