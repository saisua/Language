#ifndef LANG_GEN
#define LANG_GEN

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

// External includes
#include <json11.hpp>
#include <omp.h>

// Local includes



// To get a json11::JSON from a string: json11::Json::parse(string)
// to get a deeper object from an json11::JSON you can just doc[key]
// that also return a json11::JSON, which can be used as root in the
// following functions

// Forward declarations
json11::Json get_object_from_path(json11::Json & doc, const std::string & path);

std::string get_string_from_path(json11::Json & doc, const std::string & path){
    if(path.size() == 0) return "";

    size_t path_num = path.find(LANG_PATH_SEPARATOR);
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
        path_num = path.find(LANG_PATH_SEPARATOR, last_match);
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
                printf("Searching in path %s\n", (active_path+LANG_PATH_SEPARATOR+branch.first).c_str());
                remaining.push(branch.second);
                remaining_path.push(active_path+LANG_PATH_SEPARATOR+branch.first);
            }

            break;

        default:
            break;
        }
        
    }

    return result;
}

json11::Json get_object_from_path(json11::Json &doc, const std::string & path){
    size_t path_num = path.find(LANG_PATH_SEPARATOR);
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
        path_num = path.find(LANG_PATH_SEPARATOR, last_match);
    }

    #if LANG_VERBOSE
    printf("%s\n", path.substr(last_match).c_str());
    #endif

    return actual_path[path.substr(last_match)];
}

std::vector<json11::Json> get_tree_from_path(json11::Json &doc, const std::string & path){
    size_t path_num = path.find(LANG_PATH_SEPARATOR);
    size_t last_match = 0;
    
    std::vector<json11::Json> tree {doc};
    json11::Json actual_path = doc;
    
    #if LANG_VERBOSE
    printf("Get tree from path \"%s\":\n", path.c_str());
    #endif

    while(path_num < path.size()){
        actual_path = actual_path[path.substr(last_match, path_num-last_match)];
        tree.push_back(actual_path);

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
        path_num = path.find(LANG_PATH_SEPARATOR, last_match);
    }

    #if LANG_VERBOSE
    printf("%s\n", path.substr(last_match).c_str());
    #endif

    tree.push_back(actual_path[path.substr(last_match)]);
    return tree;
}

std::vector<json11::Json> find_in_tree(json11::Json &doc, const std::string & path, std::string & search){
    size_t path_num = path.find(LANG_PATH_SEPARATOR);
    size_t last_match = 0;
    
    // TODO: add a check for all other functions
    if(doc.type() != json11::Json::Type::OBJECT){
        return {};
    }

    std::vector<json11::Json> tree {};
    json11::Json actual_path = doc;

    // Get the map and count if the search is in it
    if(actual_path.object_items().find(search) != actual_path.object_items().end()){
        tree.push_back(actual_path[search]);
    }
    
    #if LANG_VERBOSE
    printf("Get tree from path \"%s\":\n", path.c_str());
    #endif

    while(path_num < path.size()){
        actual_path = actual_path[path.substr(last_match, path_num-last_match)];

        if(actual_path.object_items().find(search) != actual_path.object_items().end()){
            tree.push_back(actual_path[search]);
        }
    
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
        path_num = path.find(LANG_PATH_SEPARATOR, last_match);
    }

    #if LANG_VERBOSE
    printf("%s\n", path.substr(last_match).c_str());
    #endif

    tree.push_back(actual_path[path.substr(last_match)]);
    return tree;
}

json11::Json find_last_in_tree(json11::Json &doc, const std::string & path, std::string & search){
    size_t path_num = path.find(LANG_PATH_SEPARATOR);
    size_t last_match = 0;
    

    json11::Json match;
    json11::Json actual_path = doc;

    // Get the map and count if the search is in it
    if(actual_path.object_items().find(search) != actual_path.object_items().end()){
        match = actual_path[search];
    }
    
    #if LANG_VERBOSE
    printf("Get tree from path \"%s\":\n", path.c_str());
    #endif

    while(path_num < path.size()){
        actual_path = actual_path[path.substr(last_match, path_num-last_match)];

        if(actual_path.object_items().find(search) != actual_path.object_items().end()){
            match = actual_path[search];
        }
    
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
        path_num = path.find(LANG_PATH_SEPARATOR, last_match);
    }

    #if LANG_VERBOSE
    printf("%s\n", path.substr(last_match).c_str());
    #endif

    if(actual_path.object_items().find(search) != actual_path.object_items().end())
        return actual_path[search];
    
    return match;
}

#endif