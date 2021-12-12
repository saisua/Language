#ifndef LANG_SYNTAX_GEN_CPP
#define LANG_SYNTAX_GEN_CPP


#include <vector>
#include <string>
#include <json11.hpp>
#include <regex>
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
json11::Json get_object_from_path(json11::Json & doc, const std::string & path);
inline void regex_warp_solve(std::string & matcher, std::regex & reg, 
                            Mreg_gen<uintptr_t> & data, json11::Json & doc,
                            std::vector<std::regex> & regex_matcher,
                            std::unordered_set<int> & warp_ids,
                            std::stack<std::unordered_map<uintptr_t, void*>*> & states);
std::string read_str_file(const std::string& filename);
void store_states(Mreg_gen<uintptr_t> &data);

inline void _solve_generate_syntax(Mreg_gen<uintptr_t> & data, json11::Json & doc,
                                std::vector<std::regex> & regex_matcher,
                                std::vector<std::pair<std::string, std::string>> & reg_pairs,
                                std::unordered_set<int> & warp_ids,
                                std::stack<std::unordered_map<uintptr_t, void*>*> & states){
    for(auto regex_pair : reg_pairs){
        std::string syntax_value = regex_pair.second;
        printf("_solve %s\n", syntax_value.c_str());

        for(std::regex warp : regex_matcher)
            regex_warp_solve(syntax_value, warp, data, doc, regex_matcher, warp_ids, states);

        printf("Insert %s\n", syntax_value.c_str());
        warp_ids.insert(data.append(syntax_value.c_str(), regex_pair.first.c_str()));
    }
}

inline void generate_syntax(Mreg_gen<uintptr_t> & data, json11::Json  & doc){
    std::string err = "";
    std::string syntax_file = LANG_SYNTAX_PATH LANG_FROM ".json";

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
    std::stack<std::unordered_map<uintptr_t, void *>*> states{};

    _solve_generate_syntax(data, doc, regex_matcher, reg_pairs, warp_ids, states);


    data.clean();
    std::fstream store_data;
    
    std::string tree_path {LANG_SYNTAX_PATH 
                            LANG_SYNTAX_TREES_FOLDER 
                            LANG_FROM
                            ".tree"};

    store_data.open(tree_path.c_str(), std::ios::out);
    data.store(store_data);
    store_data.close();
}

void generate_states(Mreg_gen<uintptr_t> & data, std::vector<json11::Json> & files){
    std::string stand = "standalones";
    std::vector<std::pair<std::string, std::string>> standalones = 
                                get_recursive_strings_path(files[0], stand);

    for (std::pair<std::string, std::string> standalone : standalones)
    {
        std::stack<std::unordered_map<uintptr_t, void*> *> states 
                    { {&data.tmp_states} };
        std::stack<std::string> path_stack { {standalone.second} };
        std::stack<std::string> nick_stack { {standalone.first} };

        while(!path_stack.empty()){
            std::unordered_map<uintptr_t, void*> * state = states.top();
            std::string path = path_stack.top();
            std::string nick = nick_stack.top();

            std::smatch match;

            if(regex_search(path, match, std::regex(Syntax_match_symbol"(.*?)"Syntax_match_symbol))){
                if(match.prefix().length() > 0){
                    data.append_state(state, data.added_nicknames[nick]);
                }
                
            } 
        }
    }

    store_states(data);
}

void store_states(Mreg_gen<uintptr_t> & data){
    std::string state_codes = "#ifndef STATE_CODES_H\n#define STATE_CODES_H\n"
                                "#define STATE_CODES \\\n\t";
    auto state_value_it = data.all_states.begin();
    for (int state = 1; state_value_it != data.all_states.end(); ++state, ++state_value_it)
    {
        state_codes += "case " + std::to_string(*state_value_it) + ": \\\n\t\t";
        state_codes += "return " + std::to_string(state) + "; \\\n\t";
    }
    state_codes += "\n\n#endif";


    std::fstream store_data;
    store_data.open(LANG_SYNTAX_PATH 
                    LANG_SYNTAX_TREES_FOLDER 
                    LANG_SYNTAX_STATE_CODES_FOLDER 
                    LANG_FROM "_states.h", std::ios::out);
    store_data << state_codes;
    store_data.close();

    data.setup_states();

    store_data.open(LANG_SYNTAX_PATH
                    LANG_SYNTAX_TREES_FOLDER
                    LANG_FROM ".states",
                    std::ios::out);
    data.store_states(store_data);
    store_data.close();   
}

#define int_to_str(x) std::string(1, static_cast<char>(x))
inline void regex_warp_solve(std::string & matcher, std::regex & reg, 
                            Mreg_gen<uintptr_t> & data, json11::Json & doc,
                            std::vector<std::regex> & regex_matcher,
                            std::unordered_set<int> & warp_ids,
                            std::stack<std::unordered_map<uintptr_t, void*>*> & states){
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
            _solve_generate_syntax(data, doc, regex_matcher, reg_pairs, warp_ids, states);
        else{
            replacement.assign(reg_pairs[0].second);
            
            regex_warp_solve(replacement, reg, data, doc, regex_matcher, warp_ids, states);
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