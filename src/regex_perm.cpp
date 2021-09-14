#ifndef LANG_GEN
#define LANG_GEN
// FILE TO BE RENAMED

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
#include <map>
#include <utility> 

#include <json11.hpp>

// External includes
#include <omp.h>

// Local includes
#include "Mreg.cpp"

// Forward declarations
json11::Json get_object_from_path(json11::Json & doc, const std::string & path);

std::string get_string_from_path(json11::Json & doc, const std::string & path){
    if(path.size() == 0) return "";

    size_t path_num = path.find('_');
    size_t last_match = 0;
    
    json11::Json actual_path = doc;
    
    while(path_num < path.size()){
        actual_path = actual_path[path.substr(last_match, path_num-last_match)];

        //printf("%s ", path.substr(last_match, path_num-last_match).c_str());

        switch(actual_path.type()){
            case json11::Json::Type::ARRAY:
                return actual_path.array_items()[0].string_value();
            case json11::Json::Type::STRING:
                return actual_path.string_value();
            default:
                break;
        }

        last_match = path_num+1;
        path_num = path.find('_', last_match);
    }


    actual_path = actual_path[path.substr(last_match)];

    json11::Json::object obj_map;

loop_obj_type_path:

    //printf("%s\n", path.substr(last_match).c_str());

    switch (actual_path.type())
    {
        case json11::Json::Type::ARRAY:
            return actual_path.array_items()[0].string_value();
        
        case json11::Json::Type::STRING:
            return actual_path.string_value();

        case json11::Json::Type::OBJECT:
            obj_map = actual_path.object_items();
            
            if(! obj_map.size()) break;

            actual_path = obj_map.begin()->second;
            goto loop_obj_type_path;

        default:
            break;
    }
    return "<3";
}

std::vector<std::string> get_recursive_strings(json11::Json &doc, const std::string path){
    json11::Json root = get_object_from_path(doc, path);

    std::queue<json11::Json> remaining {{root}};

    std::vector<std::string> result {};

    //json11::Json leaf;
    while( ! remaining.empty()){
        json11::Json active = remaining.front();
        remaining.pop();

        switch (active.type())
        {
        case json11::Json::Type::STRING:
            result.push_back(active.string_value());

            break;
        
        case json11::Json::Type::ARRAY:
            for(auto leaf : active.array_items())
                remaining.push(leaf);
            
            break;

        case json11::Json::Type::OBJECT:
            for(auto branch : active.object_items()){
                printf("Looking in %s\n", branch.first.c_str());

                remaining.push(branch.second);
            }

            break;

        default:
            break;
            
        }
    }

    return result;
}

std::vector<std::pair<std::string, std::string>> get_recursive_strings_path(json11::Json & doc, std::string & path){
    json11::Json root = get_object_from_path(doc, path);

    std::queue<json11::Json> remaining {{root}};
    std::queue<std::string> remaining_path {{path}};

    std::vector<std::pair<std::string, std::string>> result {};

    printf("P: %s\n", path.c_str());

    //json11::Json leaf;
    while( ! remaining.empty()){
        json11::Json active = remaining.front();
        remaining.pop();
        std::string active_path = remaining_path.front();
        remaining_path.pop();

        printf("p: %s\n", active_path.c_str());

        switch (active.type())
        {
        case json11::Json::Type::STRING:
            printf("Got string \"%s\" in %s\n", active.string_value().c_str(), active_path.c_str());

            result.emplace_back(
                    active_path,
                    active.string_value());

            break;
        
        case json11::Json::Type::ARRAY:
            printf("Got array in %s\n", active_path.c_str());
            for(auto leaf : active.array_items()){
                remaining.push(leaf);
                remaining_path.push(active_path);
            }
            
            break;

        case json11::Json::Type::OBJECT:
            for(auto branch : active.object_items()){
                printf("Searching in path %s\n", (active_path+'_'+branch.first).c_str());
                remaining.push(branch.second);
                remaining_path.push(active_path+'_'+branch.first);
            }

            break;

        default:
            break;
        }
        
    }

    return result;
}

json11::Json get_object_from_path(json11::Json &doc, const std::string & path){
    size_t path_num = path.find('_');
    size_t last_match = 0;
    
    json11::Json actual_path = doc;
    
    #if LANG_VERBOSE
    printf("Get object from path \"%s\":\n", path.c_str());
    #endif

    while(path_num < path.size()){
        actual_path = actual_path[path.substr(last_match, path_num-last_match)];

        #if LANG_VERBOSE
        printf("%s ", path.substr(last_match, path_num-last_match).c_str());
        #endif

        switch(actual_path.type()){
            case json11::Json::Type::OBJECT:
                break;
            default:
                #if LANG_VERBOSE
                printf(" #WRONG TYPE %d# | remaining: %s\n",
                            actual_path.type(), 
                            path.substr(path_num+1).c_str());
                #endif

                return actual_path;
        }

        last_match = path_num+1;
        path_num = path.find('_', last_match);
    }

    #if LANG_VERBOSE
    printf("%s\n", path.substr(last_match).c_str());
    #endif

    return actual_path[path.substr(last_match)];
}

/*  
    Recursive search on a queue of iterators of members of a JSON object.
    The search algorithm is Breadth-first 
std::queue<std::string> rj_all_string_in_obj(std::queue<val_iterator> &remaining){
    std::queue<std::string> result = std::queue<std::string>();

    /*  For every iterator got as an argment, 
    while(!remaining.empty()){
        #ifdef LANG_DEBUG
        printf("###\n");
        #endif
        /*  Iterate recursivelly over them from start to end, extracting all strings
            in the selected range 
        for (rapidjson::Value::ConstMemberIterator memb_iter = remaining.front().first;
                    memb_iter != remaining.front().second; ++memb_iter)
        {
            #ifdef LANG_DEBUG
            printf("Found Member %s", memb_iter->name.GetString());
            #endif
            switch(memb_iter->value.GetType()){
                /*  If the found member is an object, get the first and last member
                    that it contains, and add them to the to-be-searched queue 
                case 3: // Object
                    #ifdef LANG_DEBUG
                    printf(" (object)\n");
                    #endif
                    remaining.push(val_iterator{
                            memb_iter->value.GetObject().MemberBegin(),
                            memb_iter->value.GetObject().MemberEnd()});
                    continue;
                /*  If the found member is an array, iterate over it, extracting all
                    strings that it contains into the resulting queue 
                case 4: // Array
                    #ifdef LANG_DEBUG
                    printf(" (array)\n");
                    #endif
                    
                    /*  For every string in the array, 
                    for(auto val_iter=memb_iter->value.GetArray().Begin();
                                val_iter != memb_iter->value.GetArray().End(); ++val_iter){
                        /*  Add it to the result queue 
                        result.push(val_iter->GetString());
                    }
                    continue;
                /*  If the found member is a string, just add it to the result 
                case 5: // string
                    #ifdef LANG_DEBUG
                    printf(" (string)\n");
                    #endif
                    result.push(memb_iter->value.GetString());
            }
        }
        /*  Since the iteration has finished, remove the work from the queue 
        remaining.pop();
    }
    #ifdef LANG_DEBUG
    printf("\n");
    #endif

    /*  Return the found results 
    return result;
}

/*  
    TODO: parallelize

    IMPORTANT: ASSUME ALL VALUES IN JSON ARE STRING

    This function takes the generated rapidjson::Document& 
    and returns a vector containing all permutations of
    the standalones ready-to-be-compiled. The function assumes
    all data in the .json provided is right.

std::vector<std::string> extract_reg(rapidjson::Document &lang, 
                        std::queue<std::string>& raw_regex,
                        std::vector<std::string>& result){
    /*  The regex construct. Checks in the string any instance of
        %tag% 
    std::regex TAG_REG ("(?=[^\\\\]|^)%(.*?[^\\\\])%");
    /*  The end of an iterator 
    std::regex_iterator<std::string::iterator> r_iter_end;
    /*  Regex string matches 
    std::smatch matches;
    /*  Fake matches 
    std::smatch fm; 
    /*  The substring of the regex remaining to be checked 
    std::string regex_sub;
    /*  actual regex 
    std::string original_regex;
    
    #ifdef LANG_DEBUG
    printf("%zu raw regex\n", raw_regex.size());
    #endif

    // TODO: Parallelize
    // This is a n^4 algorithm
    // Although usually it is like a n^3

    /*  Checks all found regex standalones for any tag
        to be replaced 
    while(!raw_regex.empty()){
        /*  The actual regex to be calculated 
        original_regex = "";
        uint last_m_pos = 0;

        #ifdef LANG_DEBUG
        std::cout << "\n\nOriginal regex: " << raw_regex.front() << std::endl;
        #endif

        /*  Checks for any %tag% in the regex_sub string.
            For every tag, replace it with the appropiate regex 
        for(std::regex_iterator<std::string::iterator> reg_iter ( raw_regex.front().begin(), raw_regex.front().end(), TAG_REG );
                    reg_iter != r_iter_end; ++reg_iter){
            /*  Add the non-tag part of the original regex
                to all current variations of the regex 
            original_regex += raw_regex.front().substr(last_m_pos, reg_iter->position()-last_m_pos);

            /*  Get the iterator of the members of the root of the JSON 
            rapidjson::Value::ConstMemberIterator search_iter = lang.MemberBegin();

            {
            /*  Get the pointer of the match to iterate over it.
                Also, remove the first '%' 
            const std::string captured_match = reg_iter->str();
            const char* char_match = (captured_match.c_str())+1;
            std::string match = "";

            #ifdef LANG_DEBUG
            std::cout << "\n" << reg_iter->empty() << " "
                         << reg_iter->str() << ", " << reg_iter->str().c_str() << " " << *char_match << 
                        "\n" << reg_iter->str().length() << ", " << reg_iter->length() << ": " << reg_iter->str().c_str()+1;
            printf("\nPath: ");
            #endif

            // TODO: add support for [,*\d]
            // Could be done into a array<vector<str>>
            // Every regex has a vector. If found a comma, remove the comma, and wait
            // for the next iteration. When no more commas, get the generated regex in the
            // position. This should be added after any other regex are generated,
            // as a new separated regex. This should actually improve performance,
            // and grant during-compilertime final data.
            // Instead of adding the string to the regex, first get all pointers
            // to the lang object. Later, concat all strings. If any of the values
            // got is to be referenced, duplicate the string for each new value.

            /*  Extract the json path of the replace values
                Keep comparison to last '\0' just in case any
                tag in the file contains a "\\%" 
            for(uint actual_char = 0; actual_char < (reg_iter->length() - 2); ++actual_char, ++char_match){
                //std::cout << *char_match << std::flush;
                /*  If the next character is an underscore, the next 
                    step in the path is already known and as so, we
                    can already access it 
                if(*char_match == '_'){
                    #ifdef LANG_DEBUG
                    std::cout  << match << " > " << std::flush;
                    #endif

                    // REPLACE WITH FindMember() - HARD : Somehow starts looking from root
                    /*  Search in the current path of the JSON file the
                        tag we are looking for 
                    for(; (*search_iter).name != match.c_str(); search_iter++);
                    /*  Get the iterator of the members of the new path 
                    search_iter = (*search_iter).value.MemberBegin();
                    
                    /*  Clear the string to look for the next path 
                    match.clear();
                } else {
                    match += *char_match;
                }
            }
            #ifdef LANG_DEBUG
            std::cout << match << std::endl;
            #endif
            /*  Assuming the path is complete, look for the final folder to
                find in the found path 
            for(; (*search_iter).name != match.c_str(); search_iter++);
            
            }
            const rapidjson::Value& found = (*search_iter).value;

            // Permutate the original strings using the found replace values
            switch(found.GetType()){
                /*  If the value found is a string, we must only update
                    our current permutated regex with the found tag value 
                case 5: // String
                    {
                    #ifdef LANG_DEBUG
                    std::cout << "Generated regex from string:" << std::endl;
                    #endif
                    original_regex += '(';
                    original_regex += found.GetString();
                    original_regex += ')';

                    #ifdef LANG_DEBUG
                    //std::cout << "\t" << found.GetString() << std::endl;
                    std::cout << "\t" << original_regex << std::endl;
                    #endif
                    }
                    
                    break;

                /*  If the value found is an array, we must update
                    our current permutated regex with the found tag values.
                    The resulting vector size will be the original size osize
                    times the json array size jasize ( osize * jasize )
                    since all found tag values must be added to the permutated
                    regexs 
                case 4: // Array
                    {
                    #ifdef LANG_DEBUG
                    std::cout << ":" << original_regex << std::endl;

                    std::cout << "Generated regex from array:" << std::endl;
                    #endif

                    /*  Start the new regex-or group (a|b|c...) 
                    std::string concat = "(";

                    /*  For all values in the array, 
                    for(auto val_iter=found.GetArray().Begin();
                                val_iter != found.GetArray().End(); ++val_iter){
                        concat += (*val_iter).GetString();
                        concat += '|';
                    }
                    
                    /*  Replace the last '|' with a ')' 
                    concat[concat.length()-1] = ')';

                    /*  Concatenate them, to generate the new regex 
                    original_regex += concat;

                    #ifdef LANG_DEBUG
                    std::cout << "\t" << original_regex << std::endl;
                    #endif
                    }
                    break;
                /*  If the value found is an object, we must update
                    our current permutated regex with the found tag values.
                    The resulting vector size will be the original size osize
                    times the number of leaves found in the patch described nleaves
                    ( osize * nleaves )
                    since all found tag values must be added to the permutated
                    regexs, and the search in the JSON objects is done recursively 
                case 3: // Object
                    {
                    std::queue<val_iterator> start = std::queue<val_iterator>();
                    /*  Just like when getting all standalones, we get the starting and last
                        members of the current path, to iterate over them in a recursive search 
                    start.push(val_iterator{found.GetObject().MemberBegin(), found.GetObject().MemberEnd()});
                    std::queue<std::string> found_perm = rj_all_string_in_obj(start);

                    #ifdef LANG_DEBUG
                    std::cout << ":" << original_regex << std::endl;

                    std::cout << "Generated regex from object:" << std::endl;
                    #endif

                    /*  Start the new regex-or group (a|b|c...) 
                    std::string concat = "(";

                    /*  For every string found in the path, 
                    while(!found_perm.empty()){
                        concat += found_perm.front();
                        concat += '|';

                        found_perm.pop();
                    }

                    /*  Replace the last '|' with a ')' 
                    concat[concat.length()-1] = ')';


                    /*  Concatenate them, to generate the new regex 
                    original_regex += concat;

                    #ifdef LANG_DEBUG
                    std::cout << "\t" << original_regex << std::endl;
                    #endif
                    }
                    break;
                default:
                    /*  If the found type is invalid, print the error message
                        I should really start writing down some asserts heh 
                    printf("WRONG TYPE FOUND %i\n", found.GetType());
            }

            /*  Get the starting point of the non-computed substring 
            last_m_pos = reg_iter->position() + reg_iter->str().length();
        }

        /*  If there was any char left in the regex, with no tags in it,
            add it to the final permutated regex 
        original_regex += raw_regex.front().substr(last_m_pos, raw_regex.front().length()-last_m_pos);

        /*  Remove the already computed regex 
        raw_regex.pop();

        #ifdef LANG_DEBUG
        printf("###### CHECK #######\n");
        #endif
        /*  If there are any tags left in the generated regex, 
            push it to-be-computed again 
        if(regex_search(original_regex, fm, TAG_REG))
            raw_regex.push(original_regex);
        /*  Otherwise, add it to the resulting regex 
        else{
            #ifdef LANG_DEBUG
            std::cout << "Added regex to the result\n?\t" << original_regex << std::endl;
            #endif

            result.push_back(original_regex);
        }
    }
    #ifdef LANG_DEBUG
    std::cout << "END (clear)" << std::endl;
    #endif

    return result;
}
*/

#endif