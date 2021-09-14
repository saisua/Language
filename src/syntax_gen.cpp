#ifndef LANG_SYNTAX_GEN_CPP
#define LANG_SYNTAX_GEN_CPP


#include <vector>
#include <string>
#include <json11.hpp>
#include "Mreg-gen.cpp"
#include <regex>
#include <unordered_set>
#include "generated_tag_list.h"
#include "regex_perm.cpp"
#include "utils/file_read.cpp"
#include "../lang_compile.h"


// Forward declarations
std::vector<std::pair<std::string, std::string>> get_recursive_strings_path(
                                json11::Json & doc, std::string & path);
inline void regex_warp_solve(std::string & matcher, std::regex & reg, 
                            Mreg_gen<uintptr_t> & data, json11::Json & doc,
                            std::vector<std::regex> & regex_matcher,
                            std::unordered_set<int> & warp_ids);
std::string read_str_file(const std::string& filename);


inline void _solve_generate_syntax(Mreg_gen<uintptr_t> & data, json11::Json & doc,
                                std::vector<std::regex> & regex_matcher,
                                std::vector<std::pair<std::string, std::string>> & reg_pairs,
                                std::unordered_set<int> & warp_ids){
    for(auto regex_pair : reg_pairs){
        std::string syntax_value = regex_pair.second;

        for(std::regex warp : regex_matcher)
            regex_warp_solve(syntax_value, warp, data, doc, regex_matcher, warp_ids);

        warp_ids.insert(data.append(syntax_value.c_str(), regex_pair.first.c_str()));
    }
}

inline void generate_syntax(Mreg_gen<uintptr_t> & data){
    std::string err = "";
    std::string syntax_file = "languages/1-Syntax/" current_compilation ".json";

    json11::Json doc = json11::Json::parse(read_str_file(syntax_file), err);

    std::vector<std::regex> regex_matcher {};
    for(std::string warp_symbol : {
                    Syntax_match_symbol,
                    Definition_match_symbol,
                    Translation_match_symbol,
                    Language_match_symbol
                }){
        std::string matcher = warp_symbol;
        matcher.append("(.*?)");
        matcher.append(warp_symbol);

        regex_matcher.emplace_back(matcher);
    }

    std::string path = std::string("standalones");
    std::vector<std::pair<std::string, std::string>> reg_pairs = 
                                get_recursive_strings_path(doc, path);
    std::unordered_set<int> warp_ids {};


    _solve_generate_syntax(data, doc, regex_matcher, reg_pairs, warp_ids);


    data.clean();
    std::fstream store_data;
    
    std::string tree_path {LANG_SYNTAX_PATH "trees//" LANG_FROM ".tree"};

    store_data.open(tree_path.c_str(), std::ios::out);
    data.store(store_data);
    store_data.close();
}

#define int_to_str(x) std::string(1, static_cast<char>(x))
inline void regex_warp_solve(std::string & matcher, std::regex & reg, 
                            Mreg_gen<uintptr_t> & data, json11::Json & doc,
                            std::vector<std::regex> & regex_matcher,
                            std::unordered_set<int> & warp_ids){
    std::smatch matched_id;
    std::string replacement = int_to_str(an_warp);

    std::string analized_str = matcher;

    bool matched = false;
    
    while(std::regex_search(analized_str, matched_id, reg)){
        matched = true;
        std::string matched_str = matched_id[1];
        
        analized_str.assign(matched_id.suffix().str());

        printf("Matched str: %s in \"%s\"\n", matched_str.c_str(), matched_str.c_str());

        std::vector<std::pair<std::string, std::string>> reg_pairs = 
                get_recursive_strings_path(doc, matched_str);
        
        // If after looking up for the sub_group in the regex,
        // it contains more than one group, add the regex to
        // data and set the "warp code" in  
        if(reg_pairs.size() > 1)
            _solve_generate_syntax(data, doc, regex_matcher, reg_pairs, warp_ids);
        else{
            replacement.assign(reg_pairs[0].second);
            
            regex_warp_solve(replacement, reg, data, doc, regex_matcher, warp_ids);
        }

        if(matched){
            if(replacement == int_to_str(an_warp) && warp_ids.size()){
                replacement.append(warp_ids.begin(), warp_ids.end());
            }

            matcher = std::regex_replace(matcher, reg, replacement, 
                                                    std::regex_constants::format_first_only);
        }
    }
}


#endif