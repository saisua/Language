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

// External includes
#include <hyperscan/src/hs.h>
#include <rapidjson/document.h>
#include <omp.h>

// type definitions
using val_iterator = std::pair<rapidjson::Value::ConstMemberIterator,rapidjson::Value::ConstMemberIterator>;

// Forward declarations
std::queue<std::string> rj_all_string_in_obj(std::queue<val_iterator>&remaining);

// global compiler time settings
//#define DEBUG
#define TIMEIT

/*  
    TODO: replace string duplication with or-s. String duplication
    will not work with multiple instances of a recursively infinite
    sub-match. Per example:
    if(a and b or c):
    should work by using %bool%* but it does not right now.
    It also would improve overall RAM usage and allow us to use a
    pre-declared array as a return value.
    As stated in the hyperscan library, this will not affect performance
    of the search

    IMPORTANT: ASSUME ALL VALUES IN JSON ARE STRING

    This function takes the generated rapidjson::Document& 
    and returns a vector containing all permutations of
    the standalones ready-to-be-compiled. The function assumes
    all data in the .json provided is right,
    As an optimization, in the future this function may return
    a pair<const char* const*, const size_t>
*/
std::vector<std::string> clean(rapidjson::Document &lang){
    std::queue<val_iterator> start = std::queue<val_iterator>();

    /*  Add the initial and last Members of the JSON file to start searching as standalones */
    start.push(val_iterator{lang.FindMember("standalones"), lang.MemberEnd()});
    /*  Recursively search in the root of lang to search all standalones' final strings */
    std::queue<std::string> raw_regex = rj_all_string_in_obj(start);

    /*  The vector to be returned, will contain the resulting strings */
    std::vector<std::string> result;

    /*  The regex construct. Checks in the string any instance of
        %tag% */
    std::regex REF_REG ("(?=[^\\\\]|^)%(.*?[^\\\\])%");
    /*  Regex string matches */
    std::smatch matches;
    /*  Fake matches */
    std::smatch fm; 
    /*  The substring of the regex remaining to be checked */
    std::string regex_sub;
    /*  Permutation groups */
    std::vector<std::string> perm_groups;
    std::vector<std::string> temp;
    int pos = 0;
    
    #ifdef DEBUG
    printf("%i raw regex\n", raw_regex.size());
    #endif

    // TODO: Parallelize
    // This is a n^4 algorithm
    // Although usually it is like a n^3

    /*  Checks all found regex standalones for any tag
        to be replaced */
    while(!raw_regex.empty()){
        /*  Groups of permutations. Initialized as an only variation
            of the regex */
        perm_groups = std::vector<std::string>{""};
        temp = std::vector<std::string>();

        /*  The substring of the regex remaining to be checked
            Initialized as the next regexto be computed */
        regex_sub = raw_regex.front();
        raw_regex.pop();

        /*  maybe should be changed to regex_match? This actually works
            or regex_iterator */

        /*  Checks for any %tag% in the regex_sub string.
            For every tag, replace it with the appropiate regex */
        while(regex_search(regex_sub, matches, REF_REG)){
            /*  Add the non-tag part of the original regex
                to all current variations of the regex */
            for(std::string &t : perm_groups)
                t += regex_sub.substr(0, matches.position());

            /*  Get the iterator of the members of the root of the JSON */
            rapidjson::Value::ConstMemberIterator search_iter = lang.MemberBegin();

            /*  Get the entire matched regex sequence. The compiled regex
                does not use any groups, si this is not a problem */
            std::string match = matches[0];
            /*  Remove the last '%' */
            match.pop_back();
            /*  Get the pointer of the match to iterate over it.
                Also, remove the first '%' */
            const char* char_match = match.c_str()+1;
            /*  Since we are using the match string to capture the path, 
                clear it from any previous stored data */
            match.clear();

            #ifdef DEBUG
            printf("\nPath: ");
            #endif
            /*  Extract the json path of the replace values
                Keep comparison to last '\0' just in case any
                tag in the file contains a "\\%" */
            for(; *char_match != '\0'; ++char_match){
                /*  If the next character is an underscore, the next 
                    step in the path is already known and as so, we
                    can already access it */
                if(*char_match == '_'){
                    #ifdef DEBUG
                    std::cout << match << " > " << std::flush;
                    #endif

                    /*  Search in the current path of the JSON file the
                        tag we are looking for */
                    for(; (*search_iter).name != match.c_str(); search_iter++);
                    /*  Get the iterator of the members of the new path */
                    search_iter = (*search_iter).value.MemberBegin();
                    
                    /*  Clear the string to look for the next path */
                    match.clear();
                } else 
                    match += *char_match;
            }
            #ifdef DEBUG
            std::cout << match << std::endl;
            #endif
            /*  Assuming the path is complete, look for the final folder to
                find in the found path */
            for(; (*search_iter).name != match.c_str(); search_iter++);
            
            const rapidjson::Value& found = (*search_iter).value;

            uint actual_str = perm_groups.size();
            // Permutate the original strings using the found replace values
            switch(found.GetType()){
                /*  If the value found is a string, we must only update
                    our current permutated regex with the found tag value */
                case 5: // String
                    {
                    #ifdef DEBUG
                    std::cout << "Generated strings from string:" << std::endl;
                    #endif
                    /*  For every current regex, add the tag value instead of the
                        tag */
                    for(std::string &t : perm_groups){
                        t += found.GetString();

                        #ifdef DEBUG
                        std::cout << "\t" << t << std::endl;
                        #endif
                    }
                    }
                    
                    break;

                /*  If the value found is an array, we must update
                    our current permutated regex with the found tag values.
                    The resulting vector size will be the original size osize
                    times the json array size jasize ( osize * jasize )
                    since all found tag values must be added to the permutated
                    regexs */
                case 4: // Array
                    {
                    #ifdef DEBUG
                    for(const std::string &t : perm_groups)
                        std::cout << ":" << t << std::endl;

                    std::cout << "Generated strings from array:" << std::endl;
                    #endif
                    /*  For all values in the array, */
                    for(auto val_iter=found.GetArray().Begin();
                                val_iter != found.GetArray().End(); ++val_iter){
                        /*  For all the current regexs, */
                        for(const std::string t : perm_groups){
                            /*  Concatenate them, and push them into a temporal vector
                                (as to not modify the original one with the new regex) */
                            temp.emplace_back(std::string(t+val_iter->GetString()));

                            #ifdef DEBUG
                            std::cout << "\t" << temp.back() << std::endl;
                            #endif
                        }
                    }

                    /*  Once all regexs are permutated, replace the permutation groups
                        with the new permutated regex */
                    perm_groups = temp;
                    /*  And clear the temporal vector */
                    temp = std::vector<std::string>();
                    }
                    break;
                /*  If the value found is an object, we must update
                    our current permutated regex with the found tag values.
                    The resulting vector size will be the original size osize
                    times the number of leaves found in the patch described nleaves
                    ( osize * nleaves )
                    since all found tag values must be added to the permutated
                    regexs, and the search in the JSON objects is done recursively */
                case 3: // Object
                    {
                    start = std::queue<val_iterator>();
                    /*  Just like when getting all standalones, we get the starting and last
                        members of the current path, to iterate over them in a recursive search */
                    start.push(val_iterator{found.GetObject().MemberBegin(), found.GetObject().MemberEnd()});
                    std::queue<std::string> found_perm = rj_all_string_in_obj(start);

                    #ifdef DEBUG
                    for(const std::string &t : perm_groups)
                        std::cout << ":" << t << std::endl;

                    std::cout << "Generated strings from object:" << std::endl;
                    #endif
                    /*  For every string found in the path, */
                    while(!found_perm.empty()){
                        /*  For every current regexs, */
                        for(std::string &t : perm_groups){
                            /*  Concatenate them, and push them into a temporal vector
                                (as to not modify the original one with the new regex) */
                            temp.push_back(t+found_perm.front());

                            #ifdef DEBUG
                            std::cout << "\t" << temp.back() << std::endl;
                            #endif
                        }

                        found_perm.pop();
                    }

                    /*  Once all regexs are permutated, replace the permutation groups
                        with the new permutated regex */
                    perm_groups = temp;
                    /*  And clear the temporal vector */
                    temp = std::vector<std::string>();
                    }
                    break;
                default:
                    /*  If the found type is invalid, print the error message
                        I should really start writing down some asserts heh */
                    printf("WRONG TYPE FOUND %i\n", found.GetType());
            }
            /*  Get the starting point of the non-computed substring */
            pos = matches.position() + matches.length();
            
            /*  If there's no string left to be checked, break */
            if(pos >= regex_sub.length()) {
                regex_sub = "";
                break;
            }

            /*  Since there was string left, get the substring, and check again
                for any tag */
            regex_sub = regex_sub.substr(pos);
        }
        
        /*  If there was any char left in the regex, with no tags in it,
            add it to the final permutated regex */
        for(std::string &t : perm_groups)
            t += regex_sub;

        #ifdef DEBUG
        printf("###### CHECK #######\n");
        #endif
        // Really slow, should parallelize
        for(std::string &t : perm_groups){
            #ifdef DEBUG
            std::cout << "? " << t << std::endl;
            #endif
            /*  If there are any tags left in the generated regex, 
                push it to-be-computed again */
            if(regex_search(t, fm, REF_REG))
                raw_regex.push(t);
            /*  Otherwise, add it to the resulting regex */
            else
                result.push_back(t);
        }
        
    }

    return result;
}

static int onMatch(unsigned int id, unsigned long long from,
                        unsigned long long to, unsigned int flags, void *data) {
    #ifdef DEBUG
    printf("\tMatch for pattern %i : %s\n", 
            id, 
            (*(std::unordered_map<uint, const char*>*)data)[id]);
    #endif
    return 0;
}

int main(){
    std::string line;
    std::string language_str;
    std::fstream lang_file;

    #ifdef TIMEIT
    auto begin = std::chrono::high_resolution_clock::now();
    #endif

    /*  Open the language JSON file as a fstream */
    lang_file.open("aucpp.json",std::ios::in);
    if (lang_file.is_open()){
        /*  Start concatenating the lines found in the resulting string
            "language_str". The '\0' parameter should make it get the entire
            file all at once, but just in case, I keep it running in the while */
        while(getline(lang_file, line, '\0')){
            language_str += line;
        }
        /*  Close the fstream */
        lang_file.close();
    } else 
        throw std::runtime_error("Fail on file reading");

    #ifdef TIMEIT
    auto end = std::chrono::high_resolution_clock::now();

    long int file_read = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    begin = std::chrono::high_resolution_clock::now();
    #endif
    
    /*  Define a Document for the JSON file to be parsed */
    rapidjson::Document lang;
    /*  Parse the loaded string JSON file as a char* using rapidjson */
    lang.Parse(language_str.c_str());
    
    if(lang.HasParseError())
        throw std::runtime_error("Error while parsing the json file. Check it and retry.");

    #ifdef TIMEIT
    end = std::chrono::high_resolution_clock::now();

    long int json_parse = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    begin = std::chrono::high_resolution_clock::now();
    #endif
    
    /*  Clean and extract from the document all regex to be compiled by
        the hyperscan library */
    std::vector<std::string> regexps = clean(lang);

    #ifdef TIMEIT
    end = std::chrono::high_resolution_clock::now();

    long int regex_extract = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    begin = std::chrono::high_resolution_clock::now();
    #endif

    /*  Since the hyperscan library is made in C, move all structs to pointers
        First, we generate a vector containing all data with te correct format,
        an later we will turn it into a pointer to pass it to hyperscan */
    std::vector<const char*> regexps_char_vector = std::vector<const char*>{};
    std::vector<uint> id_vector = std::vector<uint>{};
    std::unordered_map<uint, const char*> reg_data = std::unordered_map<uint, const char*>();

    /*  Since the final size of the vector is known, better to reallocate once
        than many times during the loop */
    regexps_char_vector.reserve(regexps.size());
    id_vector.reserve(regexps.size());

    uint id = 0;

    #ifdef DEBUG
    printf("\n\n+++++++ RESULT +++++++\n");
    #endif
    /*  For every permutated regex extracted earlier */
    for(std::vector<std::string>::const_iterator str_iter=regexps.cbegin(); 
                str_iter != regexps.cend(); ++str_iter){
        /*  Turn the string into a const char* and push it to the vector */
        regexps_char_vector.push_back((*str_iter).c_str());
        /*  Add the proper index to the function. Right now I set the ids of
            the regex to be in ascending order. Maybe later this can be changed
            to set it to the resulting hash */
        id_vector.push_back(++id);

        reg_data[id] = (*str_iter).c_str();

        #ifdef DEBUG
        std::cout << id << ": " << *str_iter << std::endl;
        #endif
    }
    #ifdef DEBUG
    printf("\n\n");
    #endif

    /*  Allocate the pointers of the sent data */
    const char* const* regexps_array = &regexps_char_vector[0];
    const unsigned int* regexps_id = &id_vector[0];

    /*  Define the pointers to the output data of the function */
    hs_database_t* database;
    hs_compile_error_t* error;
    /*  must be initialized to null, otherwise it will not work */
    hs_scratch_t* scratch = NULL;

    #ifdef TIMEIT
    begin = std::chrono::high_resolution_clock::now();
    #endif

    // In the future, once tested and everything we may want to change the mode to
    // vector mode to operate on different lines of code
    // https://intel.github.io/hyperscan/dev-reference/api_files.html#c.hs_compile_multi
    /*  Compile the generated regexs into a hyperscan database */
    if(hs_compile_multi(regexps_array, NULL, regexps_id, regexps.size(), HS_MODE_BLOCK, NULL,
                    &database, &error)  != HS_SUCCESS){
        fprintf(stderr, "ERROR: Unable to compile patterns: %s\n", error->message);
        hs_free_compile_error(error);
        return -1;
    }
    /*  Allocate a scratch (Must per concurrent thread/process) */
    if (hs_alloc_scratch(database, &scratch) != HS_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to allocate scratch space. Exiting.\n");
        hs_free_database(database);
        return -1;
    }

    #ifdef TIMEIT
    end = std::chrono::high_resolution_clock::now();

    long int HS_compilation = std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count();
    begin = std::chrono::high_resolution_clock::now();
    #endif

    // Test data
    std::vector<std::string> test {
        "if(var1>10):",
        "if  (  var1  > 10  and 3<40):",
        "if  (  var1  > 10  and var2 == var3 and 3<40  )  :",

        "if var1>10:",
        "if    var1  > 10  and 3<40:",
        "if    var1  > 10  and var2 == var3 and 3<40   :",

        "while(var1>10):",
        "while  (  var1  > 10  and 3<40):",
        "while  (  var1  > 10  and var2 == var3 and 3<40  )   :",

        "while var1>10:",
        "while    var1  > 10  and 3<40:",
        "while   var1  > 10  and var2 == var3   and    3<40    :",

        "a    =     10",
        "a=19",

        "int   a   =  10",
        "int a=19",

        "const    int    a   = 123",
        "const int a=124"
    };

    #ifdef TIMEIT
    begin = std::chrono::high_resolution_clock::now();
    #endif

    /*  This is not the best but allows me to print each scan the test data
        without passing on a void* containing a std::pair */
    for(std::string t : test){
        #ifdef DEBUG
        printf("\n%s\nStart scan:\n", t.c_str());
        #endif

        // onEvent: https://intel.github.io/hyperscan/dev-reference/api_files.html#c.match_event_handler
        // https://intel.github.io/hyperscan/dev-reference/api_files.html#c.hs_scan
        if (hs_scan(database, t.c_str(), t.length(), 0, scratch, onMatch, &reg_data) 
                    != HS_SUCCESS) {
            fprintf(stderr, "ERROR: Unable to scan %s. Exiting.\n", t.c_str());
            hs_free_scratch(scratch);
            hs_free_database(database);
            return -1;
        }
    }

    #ifdef TIMEIT
    end = std::chrono::high_resolution_clock::now();

    printf("\n\n");
    std::cout << "+ Timings:" << std::flush;
    #ifndef DEBUG
    std::cout << "(prints disabled)" << std::flush;
    #endif
    std::cout << std::endl;
    std::cout << "\tFile reading time: " << file_read << "ns" << std::endl;
    std::cout << "\tJSON parsing time: " << json_parse << "ns" << std::endl;
    std::cout << "\tRegex extraction time: " << regex_extract << "ns" << std::endl;
    std::cout << "\tHS database compilation time (" << regexps.size() << " regex):" << HS_compilation << "ns" << std::endl;
    std::cout << "\tRegex matching time (" << test.size() << " tests): " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count() << "ns" << std::endl;
    std::cout << "\tRegex matching time (avg): " << std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count()/test.size() << "ns" << std::endl;
    #endif

    /*  If the variables were originally generated, free them */
    hs_free_scratch(scratch);
    hs_free_database(database);

    return 0;
}

/*  Recursive search on a queue of iterators of members of a JSON object.
    The search algorithm is Breadth-first */
std::queue<std::string> rj_all_string_in_obj(std::queue<val_iterator> &remaining){
    std::queue<std::string> result = std::queue<std::string>();

    /*  For every iterator got as an argment, */
    while(!remaining.empty()){
        #ifdef DEBUG
        printf("###\n");
        #endif
        /*  Iterate recursivelly over them from start to end, extracting all strings
            in the selected range */
        for (rapidjson::Value::ConstMemberIterator memb_iter = remaining.front().first;
                    memb_iter != remaining.front().second; ++memb_iter)
        {
            #ifdef DEBUG
            printf("Found Member %s", memb_iter->name.GetString());
            #endif
            switch(memb_iter->value.GetType()){
                /*  If the found member is an object, get the first and last member
                    that it contains, and add them to the to-be-searched queue */
                case 3: // Object
                    #ifdef DEBUG
                    printf(" (object)\n");
                    #endif
                    remaining.push(val_iterator{
                            memb_iter->value.GetObject().MemberBegin(),
                            memb_iter->value.GetObject().MemberEnd()});
                    continue;
                /*  If the found member is an array, iterate over it, extracting all
                    strings that it contains into the resulting queue */
                case 4: // Array
                    #ifdef DEBUG
                    printf(" (array)\n");
                    #endif
                    
                    /*  For every string in the array, */
                    for(auto val_iter=memb_iter->value.GetArray().Begin();
                                val_iter != memb_iter->value.GetArray().End(); ++val_iter){
                        /*  Add it to the result queue */
                        result.push(val_iter->GetString());
                    }
                    continue;
                /*  If the found member is a string, just add it to the result */
                case 5: // string
                    #ifdef DEBUG
                    printf(" (string)\n");
                    #endif
                    result.push(memb_iter->value.GetString());
            }
        }
        /*  Since the iteration has finished, remove the work from the queue */
        remaining.pop();
    }
    #ifdef DEBUG
    printf("\n");
    #endif

    /*  Return the found results */
    return result;
}