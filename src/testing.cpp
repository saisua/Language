#ifndef LANG_TESTING
#define LANG_TESTING


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

// Local includes
#include "matcher.cpp"

inline const void test(std::vector<std::string>& test_battery, hs_database_t* database, 
                                hs_scratch_t* scratch, void* reg_data){
    for(std::string test : test_battery)
        match_line(test, database, scratch, reg_data);
}

inline std::vector<std::string> read_vec_file(const std::string& filename){
    // Test data
    std::vector<std::string> test{};
    std::string line;
    std::fstream vector_file;

    /*  Open the language JSON file as a fstream */
    vector_file.open(filename,std::ios::in);
    if (vector_file.is_open()){
        /*  Start adding the testing lines found in the resulting string
            vector test */
        while(getline(vector_file, line)){
            if(line.length()) // "don't get empty lines"
                test.push_back(line);
        }
        /*  Close the fstream */
        vector_file.close();
    } else 
        throw std::runtime_error("Fail on file reading the testing file");

    return test;

}

#endif