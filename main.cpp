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
#include <rapidjson/document.h>
#include <omp.h>
// In case of failed line matching use fuzzy regex with tre:
// https://laurikari.net/tre/documentation/
#include <tre/tre.h>

// global compiler time settings
#define DEBUG
#define TIMEIT

// Local includes
#include "src/lang_gen.cpp"
#include "src/matcher.cpp"
#include "src/testing.cpp"


// type definitions
using val_iterator = std::pair<rapidjson::Value::ConstMemberIterator,rapidjson::Value::ConstMemberIterator>;



// Forward declarations
std::string read_str_file(const std::string& filename);
rapidjson::Document parse(const std::string& parsed_str);


int main(){
    #ifdef TIMEIT
    auto begin = std::chrono::high_resolution_clock::now();
    #endif

    const std::string language_str = read_str_file("aucpp.json");

    #ifdef TIMEIT
    auto end = std::chrono::high_resolution_clock::now();

    long int file_read = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    begin = std::chrono::high_resolution_clock::now();
    #endif
    
    rapidjson::Document lang = parse(language_str);

    #ifdef TIMEIT
    end = std::chrono::high_resolution_clock::now();

    long int json_parse = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    begin = std::chrono::high_resolution_clock::now();
    #endif
    
    std::queue<val_iterator> start = std::queue<val_iterator>();

    /*  Add the initial and last Members of the JSON file to start searching as standalones */
    start.push(val_iterator{lang.FindMember("standalones"), lang.MemberEnd()});
    /*  Recursively search in the root of lang to search all standalones' final strings */
    std::queue<std::string> raw_regex = rj_all_string_in_obj(start);

    /*  The vector to be returned, will contain the resulting strings.
        Must be static since we don't want a local address */
    std::vector<std::string> extracted_vector = std::vector<std::string>();
    extracted_vector.reserve(raw_regex.size());


    /*  extract_reg and extract from the document all regex to be compiled by
        the hyperscan library */
    extract_reg(lang, raw_regex, extracted_vector);

    #ifdef TIMEIT
    end = std::chrono::high_resolution_clock::now();

    long int regex_extract = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    begin = std::chrono::high_resolution_clock::now();
    #endif

    /*  Since the hyperscan library is made in C, move all structs to pointers
        First, we generate a vector containing all data with te correct format,
        an later we will turn it into a pointer to pass it to hyperscan */
    std::vector<uint> id_vector = std::vector<uint>{};
    std::unordered_map<uint, const char*> reg_data = std::unordered_map<uint, const char*>();
    std::vector<const char*> r_vec = std::vector<const char*>{};

    /*  Since the final size of the vector is known, better to reallocate once
        than many times during the loop */
    id_vector.reserve(extracted_vector.size());
    r_vec.reserve(extracted_vector.size());

    uint id = 0;

    #ifdef DEBUG
    printf("\n\n+++++++ RESULT +++++++\n");
    #endif
    /*  For every permutated regex extracted earlier */
    for(std::string str : extracted_vector){
        #ifdef DEBUG
        std::cout << id << ": " << std::flush << str.c_str() << std::endl;
        #endif

        /*  Turn the string into the const char* needed for the creation
            of the database */
        r_vec.push_back(str.c_str());
        /*  Add the proper index to the function. Right now I set the ids of
            the regex to be in ascending order. Maybe later this can be changed
            to set it to the resulting hash */
        id_vector.push_back(++id);
        /*  Add the reference id -> regex to the unordered array passed
            to the matching function */
        reg_data[id] = str.c_str();
    }

    #ifdef DEBUG
    printf("\n\n");
    #endif

    /*  Define the pointers to the output data of the function */
    hs_database_t* database;
    /*  must be initialized to null, otherwise it will not work */
    hs_scratch_t* scratch = NULL;

    #ifdef TIMEIT
    begin = std::chrono::high_resolution_clock::now();
    #endif

    for(std::string a : extracted_vector)
        std::cout << a << "\n";
    std::cout << "\n";

    for(const char* a : r_vec)
        std::cout << a << "\n";
    std::cout << std::flush;

    compile_db(&(r_vec[0]), &(id_vector[0]), r_vec.size(), database);
    alloc_scratch(database, scratch);


    #ifdef TIMEIT
    end = std::chrono::high_resolution_clock::now();

    long int HS_compilation = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    #endif

    std::vector<std::string> testing_vector = read_vec_file("testing_regex.txt");

    #ifdef TIMEIT
    begin = std::chrono::high_resolution_clock::now();
    #endif

    /*  This is not the best but allows me to print each scan the test data
        without passing on a void* containing a std::pair */
    test(testing_vector, database, scratch, &reg_data);

    #ifdef TIMEIT
    end = std::chrono::high_resolution_clock::now();

    printf("\n\n");
    std::cout << "+ Timings:" << std::flush;
    #ifndef DEBUG
    std::cout << "(prints disabled)" << std::flush;
    #endif
    std::cout << "\n\tFile reading time: " << file_read << "ns" << std::endl;
    std::cout << "\tJSON parsing time: " << json_parse << "ns" << std::endl;
    std::cout << "\tRegex extraction time: " << regex_extract << "ns" << std::endl;
    std::cout << "\tHS database compilation time (" << r_vec.size() << " regex): " << HS_compilation << "ns" << std::endl;
    std::cout << "\tRegex matching time (" << testing_vector.size() << " tests): " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count() << "ns" << std::endl;
    std::cout << "\tRegex matching time (avg): " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count()/testing_vector.size() << "ns" << std::endl;
    #endif

    /*  If the variables were originally generated, free them */
    hs_free_scratch(scratch);
    hs_free_database(database);

    return 0;
}

std::string read_str_file(const std::string& filename){
    std::string line;
    std::string result;
    std::fstream file;

    /*  Open the language JSON file as a fstream */
    file.open(filename,std::ios::in);
    if (file.is_open()){
        /*  Start concatenating the lines found in the resulting string
            "language_str". The '\0' parameter should make it get the entire
            file all at once, but just in case, I keep it running in the while */
        while(getline(file, line, '\0')){
            result += line;
        }
        /*  Close the fstream */
        file.close();
    } else 
        throw std::runtime_error("Fail on file reading");

    return result;
}

rapidjson::Document parse(const std::string& parsed_str){
    /*  Define a Document for the JSON file to be parsed */
    rapidjson::Document doc;
    /*  Parse the loaded string JSON file as a char* using rapidjson */
    doc.Parse(parsed_str.c_str());
    
    if(doc.HasParseError())
        throw std::runtime_error("Error while parsing the json file. Check it and retry.");

    return doc;
}