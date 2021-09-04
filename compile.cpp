#ifndef LANG_GEN_COMPILE_CPP
#define LANG_GEN_COMPILE_CPP

#include <stdio.h>
#include <bits/stdc++.h>
#include <string.h>
#include <string>
#include <filesystem>

// This should not be redefined.
// However, just in case there is one case
// that needs another compilation step, I'll
// semi-hardcode it here.
#ifndef LANG_GEN_RENAME_FROM
#define LANG_GEN_RENAME_FROM { \
    "languages//1-Syntax//%s.json", \
    "languages//2-Definition//%s.json", \
    }

#endif
#ifndef LANG_GEN_RENAME_TO
#define LANG_GEN_RENAME_TO { \
    "languages//3-Translation//%s.json", \
    "languages//4-Language//%s.json", \
    }
#endif
#ifndef LANG_GEN_RENAME_OTHER
#define LANG_GEN_RENAME_OTHER {}
#endif
#define current_compilation "MAIN"

#define len(str) (sizeof(str)/sizeof(char))
int find_argv(int argc, const char * argv[], const char * search)
    __attribute__((always_inline));

int main(int argc, const char * argv[]){
    if(argc < 5){
        printf("Minimal usage: %s -from <language> -to <language>\n", argv[0]);
        return 1;
    }

    printf("Setting up preparations:\n\n");
    const char* from = argv[find_argv(argc, argv, "-from") + 1];
    const char* to = argv[find_argv(argc, argv, "-to") + 1];
    printf("  %s -> %s\n\n", from, to);

    for(std::string file_format : LANG_GEN_RENAME_FROM){
        int length = file_format.length();

        char old_path [length + len(from)], 
            new_path [length + len(current_compilation)];

        sprintf(old_path, file_format.c_str(), from);
        sprintf(new_path, file_format.c_str(), current_compilation);

        std::filesystem::copy_file(&old_path[0], &new_path[0],
                                std::filesystem::copy_options::overwrite_existing);

        #if LANG_GEN_VERBOSE
        printf("  Copied \"%s\" to \"%s\"\n", old_path, new_path);
        #endif
    }
    
    for(std::string file_format : LANG_GEN_RENAME_TO){
        int length = file_format.length();

        char old_path [length + len(to)], 
            new_path [length + len(current_compilation)];

        sprintf(old_path, file_format.c_str(), to);
        sprintf(new_path, file_format.c_str(), current_compilation);
        
        std::filesystem::copy_file(&old_path[0], &new_path[0],
                                std::filesystem::copy_options::overwrite_existing);

        #if LANG_GEN_VERBOSE
        printf("  Copied \"%s\" to \"%s\"\n", old_path, new_path);
        #endif
    }
    

    printf("Starting the compilation of the language...\n");
    return system("./compile2.sh");
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