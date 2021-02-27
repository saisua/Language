#ifndef LANG_MATCHER
#define LANG_MATCHER


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


// External includes
#include <hyperscan/src/hs.h>

// forward declarations
static int onMatch(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *data);


inline void compile_db(const char* const* regexps, const uint* ids, const uint num_regexps,
                                hs_database_t* database){
    hs_compile_error_t* error;
                                
    // In the future, once tested and everything we may want to change the mode to
    // vector mode to operate on different lines of code
    // https://intel.github.io/hyperscan/dev-reference/api_files.html#c.hs_compile_multi
    /*  Compile the generated regexs into a hyperscan database */
    if(hs_compile_multi(regexps, NULL, ids, num_regexps, HS_MODE_BLOCK, NULL,
                    &database, &error)  != HS_SUCCESS){
        fprintf(stderr, "ERROR: Unable to compile patterns: %s\n", error->message);

        hs_free_compile_error(error);
        throw std::runtime_error("Error when compiling the regex");
    }
}

inline void alloc_scratch(hs_database_t* database, hs_scratch_t* scratch){
    /*  Allocate a scratch (Must per concurrent thread/process) */
    if (hs_alloc_scratch(database, &scratch) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");

        hs_free_database(database);
        throw std::runtime_error("Error when allocating the scratch");
    }
}


inline void match_line(const std::string& line, hs_database_t* database, 
                                hs_scratch_t* scratch, void* reg_data){
    #ifdef DEBUG
    printf("\n%s\nStart scan:\n", line.c_str());
    #endif

    // onEvent: https://intel.github.io/hyperscan/dev-reference/api_files.html#c.match_event_handler
    // https://intel.github.io/hyperscan/dev-reference/api_files.html#c.hs_scan
    if (hs_scan(database, line.c_str(), line.length(), 0, scratch, onMatch,     reg_data) 
            != HS_SUCCESS) {
    fprintf(stderr, "ERROR: Unable to scan %s. Exiting.\n", line.c_str());

    hs_free_scratch(scratch);
    hs_free_database(database);
    throw std::runtime_error("Error when matching lines");
    }
}

static int onMatch(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *data) {
    #ifdef DEBUG
    std::cout << "\tMatch for pattern " << id << " : " << 
            (*(std::unordered_map<uint, const char*>*)data)[id] << std::endl;
    #endif
    return 0;
}

#endif