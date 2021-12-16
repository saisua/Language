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
inline void regex_warp_solve(std::string &matcher, std::regex &reg,
                             Mreg_gen<uintptr_t> &data, json11::Json &doc,
                             std::vector<std::regex> &regex_matcher,
                             std::unordered_set<int> &warp_ids,
                             std::stack<std::unordered_map<uintptr_t, void *> *> &states);
inline void _solve_generate_syntax(Mreg_gen<uintptr_t> &data, json11::Json &doc,
                                   std::vector<std::regex> &regex_matcher,
                                   std::vector<std::pair<std::string, std::string>> &reg_pairs,
                                   std::unordered_set<int> &warp_ids,
                                   std::stack<std::unordered_map<uintptr_t, void *> *> &states);
std::string read_str_file(const std::string &filename);
void store_states(Mreg_gen<uintptr_t> &data);


/*
    This one is easily the most spaguetti
    bs code in the whole project.

    The idea is to solve all the references to
    other regex in the json files.

    It is explained in comments because otherwise
    it would be too tedious to read.
*/

inline void generate_syntax(Mreg_gen<uintptr_t> & data, json11::Json  & doc){
    std::string err = "";
    std::string syntax_file = LANG_SYNTAX_PATH LANG_FROM ".json";

    // Regex matcher contains all the regexes that
    // are used to reference all other symbols.
    // That means that if a regex has a match,
    // it will be replaced by the corresponding
    // regex.
    std::vector<std::regex> regex_matcher {};
    for(std::string warp_symbol : {
                    Syntax_match_symbol,
                    Definition_match_symbol,
                    Translation_match_symbol,
                    Language_match_symbol
                }){
        regex_matcher.emplace_back(warp_symbol + "(.*?)" + warp_symbol);
    }

    // We start here. We need to solve all the references to other regex
    // in the json files. We start at the starting point,
    // which is the 'standalones' object in the syntax file.
    // 'standalones' is a json object that contains all the regexes
    // that can be in a separate line.
    std::string path = std::string("standalones");
    // We get all the regexes and their paths
    std::vector<std::pair<std::string, std::string>> reg_pairs = 
                                get_recursive_strings_path(doc, path);
    std::unordered_set<int> warp_ids {};
    std::stack<std::unordered_map<uintptr_t, void *>*> states{};

    // And we try to search if they have any, and if so, we solve them
    _solve_generate_syntax(data, doc, regex_matcher, reg_pairs, warp_ids, states);
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


inline void _solve_generate_syntax(Mreg_gen<uintptr_t> & data, json11::Json & doc,
                                std::vector<std::regex> & regex_matcher,
                                std::vector<std::pair<std::string, std::string>> & reg_pairs,
                                std::unordered_set<int> & warp_ids,
                                std::stack<std::unordered_map<uintptr_t, void*>*> & states){
    // We get here to try and solve all the references to other regex

    // We have all the regexes and their paths
    // so we iterate over them
    for(auto regex_pair : reg_pairs){
        // The 'syntax value' is the actual regex of the instruction.
        // and what we need to check and solve.
        std::string syntax_value = regex_pair.second;
        printf("_solve %s\n", syntax_value.c_str());

        // We search for references to other files.
        // Instead of doing so in place, I have 
        // separated the search and the replacement
        // in the funcion 'regex_warp_solve'.
        // I know calls inside a loop are not
        // a good practice, but it is for 
        // aesthetic reasons.
        for(std::regex warp : regex_matcher)
            regex_warp_solve(syntax_value, warp, data, doc, regex_matcher, warp_ids, states);

        // If we have a regex that contains no references,
        // we can just add it to the data.
        printf("Insert %s\n", syntax_value.c_str());
        warp_ids.insert(data.append(syntax_value.c_str(), regex_pair.first.c_str()));
    }
}

#define int_to_str(x) std::string(1, static_cast<char>(x))
inline void regex_warp_solve(std::string & matcher, std::regex & reg, 
                            Mreg_gen<uintptr_t> & data, json11::Json & doc,
                            std::vector<std::regex> & regex_matcher,
                            std::unordered_set<int> & warp_ids,
                            std::stack<std::unordered_map<uintptr_t, void*>*> & states){
    // In this file is where we search for references to other files.
    // as a fact, we only search for one file.
    
    std::smatch matched_id;

    std::string analized_str = matcher;

    std::string generated = "";


    // We search for any reference to the file that
    // the regex "reg" variable matches.
    while(std::regex_search(analized_str, matched_id, reg)){
        // We get the first captured group. That is
        // the reference itself, without the prefix and suffixes
        std::string matched_str = matched_id[1];
        
        // We add the text that was not a reference.
        generated += matched_id.prefix();

        // We assign the string to be matched to the
        // suffix of the match.
        analized_str.assign(matched_id.suffix().str());

        printf("Matched str: %s in \"%s\"\n", matched_str.c_str(), matched_str.c_str());

        // We get all the regexes of the reference, along with their paths.
        std::vector<std::pair<std::string, std::string>> reg_pairs = 
                get_recursive_strings_path(doc, matched_str);
        

        // If the reference contains only one regex,
        // we can just insert it to the main regex. 
        // Here is where we need to add the states, 
        // since it is where we will decide if we warp or not.
        if(reg_pairs.size() > 1){
            // If it contains more than one, "warp" is used.

            // As so, we add a "warp" code to the generated final regex
            generated += int_to_str(an_warp);

            // And we solve the regex individually, since we will
            // need it once we are matching it.
            _solve_generate_syntax(data, doc, regex_matcher, reg_pairs, warp_ids, states);
        } else {
            // Since it only contains one regex, we can just insert it.
            // but first, we have to solve it.

            // reg_pairs only contains one element, and its second
            // is the regex itself.
            std::string inserted_regex = reg_pairs[0].second;
            
            // We need to check if the regex contains any references.
            // Again, call in a loop, not a good practice.
            for(std::regex warp : regex_matcher)
                regex_warp_solve(inserted_regex, warp, data, doc, regex_matcher, warp_ids, states);

            // Since we know inserted regex is now solved,
            // we can insert it to the main regex.
            generated += inserted_regex;
        }
    }

    // Finally, if we generated a regex at all, 
    // we assign it to the matcher string, 
    // which was passed as a reference.
    // analized_str contains the final prefix
    if(!generated.empty())
        matcher = generated + analized_str;
}


#endif