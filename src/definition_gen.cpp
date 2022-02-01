#ifndef LANG_DEFINITION_GEN_CPP
#define LANG_DEFINITION_GEN_CPP

#include <fstream>
#include <string>
#include <json11.hpp>
#include <cmath>
#include <limits>
#include <algorithm>

#include "Mreg-gen.cpp"
#include "../lang_compile.h"
#include "generated_tag_list.h"
#include "utils/file_read.cpp"
#include "regex_perm.cpp"
#include "language_gen.cpp"
#include "utils/umap_set.cpp"


// We must have a prefix in case we collide
// with other generated definitions of gcc
// since we should not have lower case definitions
#define code_pref "_code_"
#define data_structure "data_struct"
#define label_pref "_label_"

std::string read_str_file(const std::string& filename);
json11::Json get_object_from_path(json11::Json & doc, const std::string & path);
inline uint_fast8_t file_id(const std::string id);
std::string get_string_from_path(json11::Json & doc, const std::string & path);
inline std::string get_path(std::string path, uint id);
std::vector<std::string> get_recursive_strings(json11::Json &doc, const std::string path);
void solve_code(std::string & code, json11::Json & doc);
void prettify_definition(std::string & definition);
void prettify_definition(char * definition);
void update_mreg(Mreg_gen<uintptr_t> &mreg, std::string nickname, uintptr_t new_code);

#define bits(len) ceil(log2(len))
#define num_bits(type) std::numeric_limits<type>::digits

std::unordered_set<std::string> max_depth_codes;

inline void reverse_string(std::string & str){
    uint n = str.length();
    char aux;

    for (uint i = 0; i < n / 2; i++){
        aux = str[i];

        str[i] = str[n - i - 1];
        str[n - i - 1] = aux;
    }
}

inline void generate_codes(Mreg_gen<uintptr_t> & data){
    std::fstream out;
    std::fstream current_language;

    std::string current_language_path = LANG_DEFINITION_FOLDER "codes//" LANG_FROM ".h";

    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::ios_base::app);
    current_language << "#include \"";
    current_language << current_language_path;
    current_language << "\"\n";
    current_language.close();

    out.open(current_language_path, std::ios::out);

    {
    std::string ward_start = "#ifndef LANG_DEFINITION_CODES_H\n#define LANG_DEFINITION_CODES_H\n\n";
    out << ward_start;
    }

    printf("Got %d nicknames\n", data.added_nicknames.size());
    std::unordered_map<uint, std::unordered_set<std::string>> codes = 
                std::unordered_map<uint, std::unordered_set<std::string>>();
    std::unordered_map<std::string, std::string> parents = 
                std::unordered_map<std::string, std::string>();

    uint max_depth = 0;
    for (auto nickname_pair : data.added_nicknames)
    {
        uint total_depth = count(nickname_pair.first.begin(), nickname_pair.first.end(), LANG_PATH_SEPARATOR);

        if(total_depth > max_depth)
            max_depth = total_depth;
    }

    for(auto nickname_pair : data.added_nicknames){
        std::string nickname = nickname_pair.first;

        std::string::iterator nick = nickname.begin();
        std::string last = "";
        std::stack<std::string> codes_stack = std::stack<std::string>();
        for (uint letter = 0; letter != nickname.length(); ++letter)
        {
            if (*nick == LANG_PATH_SEPARATOR){

                std::string current = nickname.substr(0, letter);
                codes_stack.emplace(current);

                if (!parents.count(current) && last.length())
                {
                    parents[current] = last;
                }

                last.assign(current);
            }
            ++nick;
        }

        uint depth = count(nickname.begin(), nickname.end(), LANG_PATH_SEPARATOR);
        codes_stack.emplace(nickname);
        while(!codes_stack.empty()){

            if(! codes.count(depth)){
                codes[depth] = std::unordered_set<std::string>{codes_stack.top()};
                
                printf(" Added in depth %d : %s\n", depth, codes_stack.top().c_str());
            }
            else{
                // This if is only for printing unique nicknames
                if(!codes[depth].count(codes_stack.top())){
                    codes[depth].emplace(codes_stack.top());
                    printf(" Added in depth %d : %s\n", depth, codes_stack.top().c_str());
                }
            }

            --depth;
            codes_stack.pop();
        }
    
        if(last.length())
            parents[nickname] = last;
    }
    uint id = data.added_nicknames.size();

    using bit_t = uint;

    std::unordered_map<std::string, std::vector<bit_t>> added = 
                std::unordered_map<std::string, std::vector<bit_t>>();
    std::unordered_map<uint, uint> s_bits = 
                std::unordered_map<uint, uint>();
    std::unordered_map<uint, uint> starting_bit =
                std::unordered_map<uint, uint>();
    std::unordered_map<uint, uint> current_id = 
                std::unordered_map<uint, uint>();


    s_bits[max_depth] = bits(codes[max_depth].size());
    starting_bit[max_depth] = 0;
    current_id[max_depth] = 0;

    printf("Setup depth %d(%d,0) ",max_depth, s_bits[max_depth]);
    for (int code_depth = max_depth - 1; code_depth != -1; --code_depth)
    {
        s_bits[code_depth] = bits(codes[code_depth].size()) + s_bits[code_depth + 1];
        starting_bit[code_depth] = s_bits[code_depth + 1];
        current_id[code_depth] = 0;
        printf("%d(%d,%d) ", code_depth, s_bits[code_depth], starting_bit[code_depth]);

    }
    printf("\n");

    // We add all codes to the map, in binary sectors.
    // LSB is depth 0, but the highest depth in the json
    printf("Added codes:\n");
    for(int code_depth = max_depth; code_depth != -1; --code_depth){
        for(std::string code : codes[code_depth]){
            // Add the code to the int, starting by the position.
            // so
            // we access the vector[(size_t) bit_depth / num_bits(bit_t)]
            // and we add the code.
            // We also add bit_depth to the amount of bits used.

            // We get the parents by going up the tree.
            std::stack<std::string> parents_stack = std::stack<std::string>();
            std::string current = code;
            printf("Looking up from %s\n", current.c_str());
            uint tmp_depth = max_depth;
            while(parents.count(current) && ! added.count(current)){
                parents_stack.emplace(current);
                current = parents[current];
                printf("  found parent %s\n", current.c_str());

                --tmp_depth;
            }
            
            if(added.count(current)){
                printf("   (seen)\n");
            }
            else{
                tmp_depth = 0;
                printf("   (root)\n");
                added[current] = std::vector<bit_t>();

                size_t pos = (size_t) s_bits[tmp_depth] / num_bits(bit_t);


                printf("  adding id %s [%d] with id ", current.c_str(), pos);

                while(added[current].size() <= pos)
                    added[current].emplace_back(0);

                // First check if the bits will be cut when adding to the int.
                size_t displacement = (size_t) 
                                        (starting_bit[tmp_depth] >= (num_bits(bit_t) * pos)) 
                                        * (starting_bit[tmp_depth] % num_bits(bit_t));
                added[current][pos] |= ((++current_id[tmp_depth]) << displacement);
                printf("%d << %d, depth %d\n", current_id[tmp_depth], displacement, tmp_depth);

                std::string printed = std::string(current);
                prettify_definition(printed);
                out << "#define " code_pref << printed << " ";
                std::string code = "";
                if(added[current].size() > 1)
                    code += "{";
                for(bit_t bit : added[current]){
                    code += std::to_string(bit) + ",";
                }
                code.pop_back();

                if(added[current].size() > 1)
                    code += "}";

                out << code << "\n";

                out << "#define " code_pref << printed << "_mask ";
                code = "";
                if(added[current].size() > 1)
                    code += "{";
                for (int f = added[current].size() - 1; f != -1; --f)
                {
                    // If we are in a parent
                    if(s_bits[tmp_depth] < num_bits(bit_t) * f){
                        code += std::to_string(std::numeric_limits<bit_t>::max()) + ",";
                    // If we are in a child
                    } else if(starting_bit[tmp_depth] >= num_bits(bit_t) * (f+1)){
                        code += "0,";
                        // If we are in an important bit
                    } else {
                        if(displacement == 0){
                            code += std::to_string(std::numeric_limits<bit_t>::max()) + ",";
                        } else {
                            code += std::to_string(~((bit_t)pow(2, displacement)-1)) + ",";
                        }
                    }
                }
                code.pop_back();

                if(added[current].size() > 1)
                    code += "}";

                out << code << "\n\n";

                if(data.added_nicknames.count(current))
                    update_mreg(data, current, added[current][0]);
            }

            ++tmp_depth;
            std::vector<bit_t> last_added = added[current];
            while (!parents_stack.empty())
            {
                current = parents_stack.top();
                parents_stack.pop();
            
                printf("  adding id %s with id ", current.c_str());

                size_t pos = (size_t) s_bits[tmp_depth] / num_bits(bit_t);
                added[current] = std::vector<bit_t>();
                added[current].assign(last_added.begin(), last_added.end());
                last_added = added[current];

                while(added[current].size() <= pos)
                    added[current].emplace_back(0);

                size_t displacement = (size_t) 
                                        (starting_bit[tmp_depth] >= (num_bits(bit_t) * pos)) 
                                        * (starting_bit[tmp_depth] % num_bits(bit_t));
                added[current][pos] |= ((++current_id[tmp_depth]) << displacement);
                printf("%d << %d, depth %d\n", current_id[tmp_depth], displacement, tmp_depth);

                std::string printed = std::string(current);
                prettify_definition(printed);
                out << "#define " code_pref << printed << " ";
                std::string code = "";
                if(added[current].size() > 1)
                    code += "{";
                for(bit_t bit : added[current]){
                    code += std::to_string(bit) + ",";
                }
                code.pop_back();
                if(added[current].size() > 1)
                    code += "}";
                out << code << "\n";

                // It has to be prettified.
                if (tmp_depth == max_depth)
                {
                    max_depth_codes.emplace(printed);
                }
                else
                {
                    out << "#define " code_pref << printed << "_mask ";
                    code = "";
                    if(added[current].size() > 1)
                        code += "{";
                    for (int f = added[current].size() - 1; f != -1; --f)
                    {
                        // If we are in a parent
                        if(s_bits[tmp_depth] < num_bits(bit_t) * f){
                            code += std::to_string(std::numeric_limits<bit_t>::max()) + ",";
                        // If we are in a child
                        } else if(starting_bit[tmp_depth] >= num_bits(bit_t) * (f+1)){
                            code += "0,";
                            // If we are in an important bit
                        } else {
                            if(displacement == 0){
                                code += std::to_string(std::numeric_limits<bit_t>::max()) + ",";
                            } else {
                                code += std::to_string(~((bit_t)pow(2, displacement)-1)) + ",";
                            }
                        }
                    }
                    code.pop_back();
                    if(added[current].size() > 1)
                        code += "}";
                    out << code << "\n";
                }
                out << '\n';

                if(data.added_nicknames.count(current))
                    update_mreg(data, current, added[current][0]);

                ++tmp_depth;
            }
        }
    }

    {
    std::string ward_end = "\n#endif";
    out << ward_end;
    }

    out.close();
}

void update_mreg(Mreg_gen<uintptr_t> &mreg, std::string nickname, uintptr_t new_code){
    uintptr_t old_code = mreg.added_nicknames[nickname];
    mreg.added_nicknames[nickname] = new_code;

    printf("Updating mreg code %d -> %d\n", old_code, new_code);

    for (uintptr_t branch : mreg.final_nicknames[nickname])
    {
        mreg.data[branch + FINAL] = new_code;
    }
    for(uint node = node_length; node < mreg.data.size(); node += node_length)
        if(mreg.data[node+WARP_CAPTURES] >> mreg.captures_shift)
            for(uintptr_t & capture : *reinterpret_cast<capture_t<uintptr_t>*>(
                                                            mreg.data[node+WARP_CAPTURES]) )
                if(abs(capture) == old_code){
                    capture = capture > 0 ? new_code : -new_code;

                    printf("  Updated node %d\n", node);
                }

    mreg.all_states.insert(new_code);
}

inline void generate_definition_checks(Mreg_gen<uintptr_t> & data, 
                                        std::vector<json11::Json> & files){

    
    std::string err;
    // Since there is no id 0, files id must be -1
    json11::Json syntax = files[Syntax_id-1];
    json11::Json definition = files[Definition_id-1];
    json11::Json translation = files[Translation_id-1];
    json11::Json language = files[Language_id-1];

    std::string definitions = "";
    std::string new_group_code = "#define next_group \\\nif(todo_jump.empty()) \\\n\tgoto end_definition_label; \\\n"
                                    "else { \\\n\tvoid * next = todo_jump.top(); \\\n\t"
                                    "todo_jump.pop(); \\\n\t\\\n\t"
                                    "mreg = todo_mreg.top(); \\\n\t"
                                    "todo_mreg.pop(); \\\n\t\\\n\t"
                                    "goto *next; \\\n}"
                                    "\n\n"
                                    "#define error printf(\"Error\");"
                                    "\n\n"
                                    "#define store_state(next_jump) todo_jump.push( && next_jump ); todo_mreg.push( mreg );";
    std::string initial_code = "#define definition_generated(" data_structure ") \\\nswitch(" data_structure "[0]){ \\\n";
    std::string switch_cases = "";
    std::string translation_code = "";
    std::string final_code = "";
    

    // This maps a nickname to its corresponding capture groups.
    // When a subgroup needs to be set in a different order,
    // this map should project that. RN unused
    std::unordered_map<std::string, std::unordered_map<uint, uint>> nick_groups_order {};
    std::vector<std::string> subcompiled_definitions;
    std::unordered_set<std::string> generated_nicknames;

    for(auto & nickname_pair : data.added_nicknames){
        printf("Get object of definition : %s\n", nickname_pair.first.c_str());
        json11::Json active = get_object_from_path(definition, nickname_pair.first);

        if(active.is_object() && !generated_nicknames.count(nickname_pair.first))
            for(auto & group_pair : active.object_items()){
                int capture_pos = group_pair.second["groups"].int_value();

                std::string group_name = nickname_pair.first + "_" + group_pair.first;
                
                prettify_definition(group_name);

                generated_nicknames.insert(group_name);

                printf("  Defined: %s\n", group_name.c_str());

                // Add 1 to capture_pos, since 0 is reserved for the id
                definitions += ("#define definition_" + group_name + " " + 
                                            std::to_string(capture_pos+1) + "\n");

                if(! group_pair.second["sub-compile"].is_null()){
                    std::smatch subcompile_match;

                    printf("  & has subcompile %s\n", group_pair.second["sub-compile"].string_value().c_str());

                    // Add to added_nicknames all strings that match "#(.*?)#"
                    if(std::regex_search(group_pair.second["sub-compile"].string_value(), subcompile_match, std::regex(
                                        Definition_match_symbol "(.*?)" Definition_match_symbol)))
                        for(uint i = 1; i < subcompile_match.size(); i++)
                            if(!generated_nicknames.count(subcompile_match[i])){
                                std::string subcompile_nickname = subcompile_match[i];
                                printf("    Subcompile: %s\n", subcompile_nickname.c_str());
                                subcompiled_definitions.emplace_back(subcompile_nickname);

                            }
                }
            }
    }
    for(auto & nickname : subcompiled_definitions){
        printf("Get object of definition : %s\n", nickname.c_str());
        json11::Json active = get_object_from_path(definition, nickname);

        if(active.is_object() && !generated_nicknames.count(nickname))
            for(auto & group_pair : active.object_items()){
                int capture_pos = group_pair.second["groups"].int_value();

                std::string group_name = nickname + "_" + group_pair.first;

                prettify_definition(group_name);
                
                printf("  Defined: %s\n", group_name.c_str());

                // Add 1 to capture_pos, since 0 is reserved for the id
                definitions += ("#define definition_" + group_name + " " + 
                                            std::to_string(capture_pos+1) + "\n");

                if(! group_pair.second["subcompile"].is_null()){
                    std::smatch subcompile_match;

                    // Add to added_nicknames all strings that match "#(.*?)#"
                    if(std::regex_search(group_pair.second["subcompile"].string_value(), subcompile_match, std::regex(
                                        Definition_match_symbol "(.*?)" Definition_match_symbol)))
                        for(uint i = 1; i < subcompile_match.size(); i++)
                            if(!generated_nicknames.count(subcompile_match[i])){
                                std::string subcompile_nickname = subcompile_match[i];
                                printf("    Subcompile: %s\n", subcompile_nickname.c_str());
                                subcompiled_definitions.emplace_back(subcompile_nickname);

                            }
                }
            }
    }

    // Add default checks
    for(auto & nickname_pair : data.added_nicknames){
        json11::Json active_def = get_object_from_path(definition, nickname_pair.first);
        json11::Json active_trans = get_object_from_path(translation, nickname_pair.first);
        
        if(active_trans.is_null()// && active_def.is_null()
                                    ){
            printf("Not found %s in any of the files (%d,%d)\n\n", nickname_pair.first.c_str(),
                                                    //active_def.is_null(), 
                                                    active_trans.is_null());
            continue;
        }
        else if(active_trans.is_null()){
            // So that at least matches
            initial_code += "\tcase " + nickname_pair.first + ": \\\n\tbreak; \\\n";
        }

        printf("\nGot object of type ");
        switch(active_trans.type()){
            case json11::Json::BOOL:
                printf("Bool");
                break;
            case json11::Json::STRING:
                printf("String");
                break;
            case json11::Json::NUMBER:
                printf("Number");
                break;
            case json11::Json::ARRAY:
                printf("Array");
                break;
            case json11::Json::OBJECT:
                printf("Object");
                break;
            default:
                break;
        }
        printf(" from translation:\n  %s\n", nickname_pair.first.c_str());

        std::string case_id (nickname_pair.first);

        
        prettify_definition(case_id);
        initial_code += "\tcase " code_pref + case_id +": \\\n";

        std::vector<std::pair<std::string, json11::Json>> found_paths = {};
        // START SHOULD ONLY BE USED IF __MERGES__
        json11::Json merge = find_last_in_tree(translation, case_id, "__merges__");
        if(!merge.is_null()){
            uint start = 1 + atoi(merge.string_value().c_str());
            initial_code += "\t\tswitch(" data_structure "[" data_structure "["+ std::to_string(start) + "]]){ \\\n";
        } else {
            found_paths = find_recursive_path(definition, case_id, "groups");
            printf("  Found %d groups\n", found_paths.size());

            int argument_pos = 0;
            for(auto & argument : found_paths){
                if(argument.second.is_number())
                    argument_pos = argument.second.int_value();
                else if(argument.second.is_string())
                    argument_pos = atoi(argument.second.string_value().c_str());
                else
                    // Since the argument is not good, we store it empty
                    argument = {};

                if(argument_pos < 0 || argument_pos >= found_paths.size())
                    // Since the argument is not good, we store it empty
                    argument = {};
            }

            /* Test, this must not be done here
            for(std::string & path : sorted_paths){
                if(path.empty())
                    continue;

                initial_code += "\t\tswitch(" data_structure "[" data_structure "[definition_" +  path + "]]){ \\\n";
            }
            */
        }

        final_code += label_pref + case_id + "_final: \\\n";
        // in switch_cases apply switch-goto
        bool first_in_switch = true;

        // Check for sub-group types

        #define vec_stack(type) std::stack<type, std::vector<type>>
 
        
        std::unordered_set<std::string> initial_cases = {};
        std::unordered_map<std::string, uint> path_id = std::unordered_map<std::string, uint>();
        std::vector<std::string> state = std::vector<std::string>();
        unordered_map_set(uint, uint) states = unordered_map_set(uint, uint)();
        vec_stack(uint) state_stack = vec_stack(uint)();
        vec_stack(uint) start_stack = vec_stack(uint)();
        
        // #####
        // STATE
        // #####

        // Using reversed case_string to make sure that the first case is the last in the switch
        std::string reversed_case_string;
        std::string path_result = "";

        printf("State generation start\n");
        for(std::string case_string : get_recursive_strings(syntax, case_id)){
            auto sorted_paths_it = found_paths.rbegin();
            uint start_find = 0;
            reversed_case_string = std::string(case_string);
            reverse_string(reversed_case_string);
    
            // Good naming could help here, but we are dealing with multiple tree paths
            // and I'm also doing this on different days.
            std::pair<uint, std::string> found_path = find_id_path(reversed_case_string, start_find);

            while(found_path.first){
                while(sorted_paths_it != found_paths.rend() && (*sorted_paths_it).first.empty()){
                    ++sorted_paths_it;
                }
                if(sorted_paths_it != found_paths.rend()){
                    initial_code += "\t\tswitch(" data_structure "[" data_structure "[definition_" +  (*sorted_paths_it).first + "]]){ \\\n";
                    
                    ++sorted_paths_it;
                }

                // Since the original case_string is reversed, we need to reverse the found_path
                path_result = found_path.second;
                reverse_string(path_result);

                printf("Found path: \"%s\" in id=%d [%d]\n", path_result.c_str(), found_path.first, start_find);
                printf("  Next: %s\n", reversed_case_string.substr(start_find).c_str());

                for(std::pair<std::string, std::string> & path : get_recursive_strings_path(files[found_path.first - 1], path_result)){
                    std::string total_path (path.first);
                    prettify_definition(total_path);
                    printf(" Got state %s\n", total_path.c_str());

                    if(!path_id.count(total_path)){
                        path_id[total_path] = path_id.size() + 1;
                    }

                    std::unordered_set<uint> state_set = std::unordered_set<uint>();

                    uint _ = 0;
                    // Generate the unordered_set of next possible states
                    // First find all possible first states, but only if HAS next states
                    // Keep adding strings to stack, then pop them off to initial_code.
                    std::pair<uint, std::string> next_states_path = find_id_path(path.second, _);
                    if(next_states_path.first){
                        printf("  Detected next states in %s\n", path.second.c_str());
                        bool first_hash = false;
                        for(std::pair<std::string, std::string> & next_path : get_recursive_strings_path(files[next_states_path.first - 1], next_states_path.second)){
                            if(!path_id.count(next_path.first)){
                                path_id.emplace(next_path.first, path_id.size() + 1);
                                state.emplace_back(path.first);
                                printf("%d ", path_id.size());

                                // No break. Need to compute everything to store
                                // it in the states map
                                first_hash = true;
                            } else {
                                printf("%d ", path_id[next_path.first]);
                            }

                            state_set.insert(path_id[next_path.first]);
                        }

                        // State set fails. Maybe i'm not getting something?
                        if(first_hash || !states.count(state_set)){
                            printf(" | Detected as new branch.\n");

                            states[state_set] = state.size();
                            state.emplace_back(case_id + "_" + total_path);
                            initial_code += "\t\t\tcase " code_pref + total_path + ": \\\n";

                            initial_code += "\t\t\t\tstore_state( " label_pref + case_id + "_final ); \\\n";
                            initial_code += "\t\t\t\tgoto " label_pref + case_id + "_" + total_path + "; \\\n";

                            // Generate state
                            switch_cases += label_pref + case_id + "_" + total_path + ": \\\n";
                        } else {
                            printf("} Detected as existing branch.\n");

                            std::string label = label_pref + state[states[state_set]];

                            // Redirect to existing state
                        }
                    } else {
                        if(! initial_cases.count(total_path)){
                            printf("  Detected that %s did not exist and had no children\n", total_path.c_str());
                            initial_cases.insert(total_path);

                            // Since it does not have next states we have to generate the code directly
                            // Check if this generates any code, or if it has to convert types.
                            initial_code += "\t\t\tcase " code_pref + total_path + ": \\\n";

                            initial_code += "\t\t\t\tgoto " label_pref + case_id + "_final; \\\n";
                        }
                    }
                    
                }

                initial_code += "\t\t\t[[unlikely]] \\\n\t\t\tdefault: error; \\\n\t\t} \\\n";


                uint start_offset = start_find;
                found_path = find_id_path(reversed_case_string, start_find);
                start_find += start_offset + path_result.length() + 2;
            }
        }
        printf("\n");

        std::unordered_map<std::string, vec_stack(std::string)> sub_groups;
        vec_stack(json11::Json) conditions = vec_stack(json11::Json)({active_trans});
        vec_stack(uint) objects_seen = vec_stack(uint)({0});
        vec_stack(bool) was_condition = vec_stack(bool)({true});
        vec_stack(bool) path_set_pos = vec_stack(bool){{true}};
        vec_stack(bool) path_from_root = vec_stack(bool){{true}};
        vec_stack(std::string) path_seen = vec_stack(std::string)({nickname_pair.first});
        std::string path_to_root = std::string(nickname_pair.first);

        std::string condition = "";

        uint depth = 0;

        std::vector<std::regex> language_patterns = generate_patterns(language);

        while (!conditions.empty()){
            printf(" %d remaining conditions.\n", conditions.size());
            json11::Json active = conditions.top();
            uint seen = objects_seen.top();
            bool is_condition = was_condition.top();
            std::string active_path = path_seen.top();

            printf("1st ~ %d %d ~ depth: %u\n", seen, active.object_items().size(), depth);
            if(seen == active.object_items().size() || !active.object_items().size()){
                printf(" ~ Reached end of object %d/%d.\n", seen, active.object_items().size());
                if(depth){
                    --depth;
                    final_code += "\t} \\\n";
                }

                conditions.pop();
                objects_seen.pop();
                was_condition.pop();
                path_seen.pop();

                continue;
            }

            ++objects_seen.top();

            std::map<std::string, json11::Json>::const_iterator it = active.object_items().cbegin();


            for(uint a = 0; a < seen; ++a, ++it);

            // If we get "code", don't generate any condition
            auto & group_pair = *it;
            {
            // I'm assuming only a code node can
            // exist in a leaf. Otherwise, I should
            // think about any other use and prepare
            // the algorithm
            printf("2nd ~ \"%s\" \"code\"\n", group_pair.first.c_str());
            if(group_pair.first == "code"){
                std::vector<std::string> paths {};
                switch(group_pair.second.type()){
                    case json11::Json::Type::STRING:
                        paths.push_back(group_pair.second.string_value());
                        break;
                    case json11::Json::Type::ARRAY:
                        for(auto & lang_json : group_pair.second.array_items())
                            paths.push_back(lang_json.string_value());
                        break;
                    case json11::Json::Type::OBJECT:
                        for(auto & lang_pair : group_pair.second.object_items())
                            paths.push_back(lang_pair.first);
                        break;

                    default:
                        break;
                }

                printf("Got %d paths\n", paths.size());
                // Generate code and add the proper calls
                // I have to fix calls from the code and 
                // variable generation
                for(std::string lang_path : paths){
                    uint final_id = file_id(lang_path);

                    printf("  Path: %s in file %u (%d strings)\n", get_path(lang_path, final_id).c_str(),
                                            final_id,
                                            get_recursive_strings(
                                                    files[final_id-1], 
                                                    get_path(lang_path, final_id)).size());

                    std::string print_path = get_path(lang_path, final_id);

                    prettify_definition(print_path);

                    final_code += std::string(depth + 1, '\t') + "translation_" + print_path + "; \\\n";
                    final_code += std::string(depth + 1, '\t') + "next_group; \\\n";

                    translation_code += "#define translation_" + print_path;

                    // I have to think if there is any way of generating any other
                    // code that is not from language. Maybe from trans??
                    std::string tmp_code = "";
                    for (std::string &final_code : get_recursive_strings(
                                                                files[final_id - 1],
                                                                get_path(lang_path, final_id)))
                    {
                        solve_code(final_code, language_patterns, language);

                        tmp_code += " \\\n\t\"" + final_code + "\",";
                    }
                    tmp_code.pop_back();

                    uint code_size = std::count(tmp_code.begin(), tmp_code.end(), '\n');

                    translation_code += " \\\n\tadd_line<"+
                                            std::to_string(code_size)+
                                            ">({" + tmp_code + "})\n";
                }

                // There should always be depth > 0, but just in case
                if(depth){
                    --depth;

                    final_code += std::string(depth+1, '\t') + "} \\\n";

                    conditions.pop();
                    objects_seen.pop();
                    was_condition.pop();
                    path_seen.pop();
                }
                

                continue;
            } else if(group_pair.first == "representation"){
                printf("Got a representation\n");
            } else {
                conditions.push(group_pair.second);
                objects_seen.push(0);

                printf("3rd ~ \"%s\" *condition*\n", condition.c_str());
                if(is_condition){
                    printf("New condition: \"%s\"\n", group_pair.first.c_str());

                    // Since this was a condition,
                    // next is not
                    was_condition.push(false);

                    switch(group_pair.first[0]){
                        /* This switch does nothing for now
                        case PATH_ROOT:
                            printf("New path from root:\n");

                            path_seen.push(active_path);
                            path_set_pos.push(true);
                            path_from_root.push(true);

                            condition.assign(group_pair.first.substr(1));

                            break;

                        case PATH_PWD:
                            printf("New path from current:\n");

                            path_seen.push(active_path);
                            path_set_pos.push(true);
                            path_from_root.push(false);

                            condition.assign(group_pair.first.substr(1));

                            break;

                        case PATH_SET:

                            if(std::regex_search(group_pair.first, match, std::regex(PATH_SET + "(.*?)" + PATH_SET))){
                                std::string new_path = match[1];

                                printf("New path set: %s\n", new_path.c_str());
                                
                                condition.assign(group_pair.first.substr(new_path.size()+2));
                                path_set_pos.push(true);
                                // Should it be from path?
                                path_from_root.push(false);

                                path_seen.push(new_path);

                                break;
                            }
                            // If for some reason, the regex fails,
                            // fall through to default
                            [[fallthrough]];
                        */

                        default:
                            path_seen.push(active_path);

                            condition.assign(group_pair.first);

                            break;
                    }
                } else {
                    // Since this was not a condition,
                    // next is either a condition
                    // or directly code.
                    was_condition.push(true);

                    bool set_pos;
                    switch (group_pair.first[0])
                    {
                        case PATH_ROOT:
                            [[fallthrough]];
                        case PATH_PWD:
                            [[fallthrough]];
                        case PATH_SET:
                            set_pos = true;
                            break;
                        default:
                            set_pos = false;
                            break;
                    }

                    uint id = file_id(group_pair.first);

                    printf("Detected (obj %d) path %s in file %d\n", seen, get_path(group_pair.first, id).c_str(), id);

                    if(depth == 0 && seen == 0 && !first_in_switch){
                        final_code += std::string(depth+1, '\t');
                        final_code += "pos = root; \\\n";
                    }
                    first_in_switch = false;
                    final_code += std::string(depth+1, '\t');
                    if(seen == 0)
                        final_code += "if(";
                    else
                        final_code += "else if(";


                    final_code += data_structure "[" data_structure "[";

                    std::vector<std::string> found_sub_conditions {};

                    uint last = 0, pos = 0;
                    while((pos = condition.find(LANG_PATH_SEPARATOR, last)) < condition.size()){
                        found_sub_conditions.push_back(condition.substr(last, pos-last));

                        last = pos+1;
                    }


                    std::string sub_condition = "";
                    // Fail when sub-conditions:
                    // Since the sub-condition is based on a sub-node,
                    // the path from nickname_pair.first is not valid.
                    // Per example, when doing "if_boolean_operation"
                    // "operation is actually from bool, not if".
                    // It could be updated if there is no detected 
                    // attribute in current path, altough I cannot
                    // foresee how it could be fixed.
                    //
                    // Update: Just keep current path and root, and
                    // allow the user to specify them by two prefixes
                    if(found_sub_conditions.size()){
                        found_sub_conditions.push_back(condition.substr(last));

                        auto rit = found_sub_conditions.crbegin();
                        bool first = true;
                        for(; rit != found_sub_conditions.crend(); ++rit){
                            if(! first)
                                final_code += data_structure "[";
                            else
                                first = false;

                            //if (active_path.rfind("standalones", 0) == 0)
                            sub_condition += "definition_";
                            sub_condition += active_path + "_" + *rit;

                            prettify_definition(sub_condition);
                            final_code += sub_condition + " + ";
                        }

                        /*
                        if(from_root)
                            final_code += "root";
                        else*/
                        final_code += "pos";

                        final_code += std::string(found_sub_conditions.size()-1, ']');
                    } else {
                        //if (active_path.rfind("standalones", 0) == 0)
                        sub_condition += "definition_";
                                
                        sub_condition += active_path + "_" + condition;

                        prettify_definition(sub_condition);
                        final_code += sub_condition + " + pos";
                    }

                    final_code += "]] ";

                    std::string path;
                    std::smatch match;
                    switch (group_pair.first[0])
                    {
                        case PATH_ROOT:
                            printf("New path from root\n");

                            path = get_path(group_pair.first.substr(1), id);

                            path_seen.push(path_to_root);
                            // + path] == x){ ???

                            prettify_definition(path);

                            // If it is a parent (not max depth) I need to & slice the id 
                            if( ! max_depth_codes.count(path))
                                final_code += "& " code_pref + path + "_mask == ";
                            else
                                final_code += "== ";

                            final_code += code_pref + path + "){ \\\n" + std::string(depth + 2, '\t') + "pos = root; \\\n";

                            break;
                            
                        case PATH_PWD:
                            printf("New path from current\n");

                            path = get_path(group_pair.first.substr(1), id);

                            path_seen.push(path);

                            prettify_definition(path);

                            // If it is a parent (not max depth) I need to & slice the id 
                            if( ! max_depth_codes.count(path))
                                final_code += "& " code_pref + path + "_mask == ";
                            else
                                final_code += "== ";

                            final_code += code_pref + path + "){ \\\n";

                            break;

                        case PATH_SET:
                            if(std::regex_search(group_pair.first, match, std::regex(PATH_SET + "(.*?)" + PATH_SET))){
                                std::string new_path = match[1];

                                printf("New path set: %s\n", new_path.c_str());

                                path_seen.push(new_path);;

                                path = get_path(group_pair.first.substr(new_path.size() + 2), id);

                                prettify_definition(path);

                                // If it is a parent (not max depth) I need to & slice the id 
                                if( ! max_depth_codes.count(path))
                                    final_code += "& " code_pref + path + "_mask == ";
                                else
                                    final_code += "== ";

                                final_code += code_pref + path + "){ \\\n";

                                break;
                            }
                            [[fallthrough]];

                        default:
                            path = get_path(group_pair.first, id);

                            prettify_definition(path);

                            // If it is a parent (not max depth) I need to & slice the id 
                            if( ! max_depth_codes.count(path))
                                final_code += "& " code_pref + path + "_mask == ";
                            else
                                final_code += "== ";

                            final_code += code_pref + path +"){ \\\n";

                            path_seen.push(active_path);
                            break;
                    }


                    if(set_pos)
                        final_code += std::string(depth + 2, '\t') + "pos = " data_structure "[" + sub_condition + " + pos]; \\\n";

                        ++depth;
                    }
            }
            }
        }

        initial_code += std::string(depth + 2, '\t') + "break; \\\n\t\\\n";
    }
    initial_code += "\t\\\n\t[[unlikely]] \\\n\tdefault: error; \\\n}";

    printf("END\n"); fflush(stdout);

    std::fstream current_language;

    std::string current_language_path = LANG_DEFINITION_FOLDER "generated//" LANG_FROM ".h";
    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::fstream::app);
    current_language << "#include \"";
    current_language << current_language_path;
    current_language << "\"\n";
    current_language.close();

    std::fstream out;
    out.open(LANG_DEFINITION_PATH "generated//" LANG_FROM ".h", std::ios::out);

    out << "#ifndef LANG_DEFINITION_GENERATED_H\n#define LANG_DEFINITION_GENERATED_H\n\n";

    out << new_group_code;
    out << "\n\n";
    out << definitions;
    out << "\n\n";
    out << initial_code;
    out << " \\\n\\\n";
    out << switch_cases;
    out << "\\\n\\\n";
    out << final_code;
    out << "\\\nend_definition_label:\n";

    std::string ward_end = "\n\n#endif\n\n";
    out << ward_end;

    out.close();

    current_language_path = LANG_TRANSLATION_FOLDER "code_groups//" LANG_TO ".h";
    
    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::ios_base::app);
    current_language << "#include \"";
    current_language << current_language_path;
    current_language << "\"\n";
    current_language.close();

    std::fstream translation_out;

    translation_out.open(LANG_TRANSLATION_PATH "code_groups//" LANG_TO ".h", std::ios::out);

    translation_out << translation_code;

    translation_out.close();
}

inline void generate_utils_dependencies(std::vector<json11::Json> & files){
    std::string dependencies = "";
    
    for(auto & file : files){
        json11::Json dependencies_object = get_object_from_path(file, "Reserved-regex_utils");
        if(dependencies_object.is_array()){
            for(auto & dependency : dependencies_object.array_items()){
                dependencies += {"#include \"" LANG_UTILS_FOLDER};
                dependencies += dependency.string_value() + ".h\"\n";
            }
        }
    }

    json11::Json var_struct = get_object_from_path(files.back(), "Reserved-regex_utils");
    dependencies += "\n#define VAR_GEN \\n";
    if (var_struct.is_array())
    {
        for(auto & dependency : var_struct.array_items()){
            const char * var_name = dependency.string_value().c_str();

            for (; *var_name; ++var_name)
            {
                if (*var_name == '*')
                {
                    dependencies += "\t";
                    dependencies += var_name;
                    dependencies += ", \\\n";
                    break;
                }
            }
        }
    }

    std::fstream out;
    out.open(LANG_LANGUAGES_FOLDER "current_language.h", std::ios::app);

    out << "\n";
    out << dependencies;

    out.close();
}

inline void prettify_definition(std::string & definition){
    prettify_definition(definition.data());
}

inline void prettify_definition(char * definition){
    for(char * c = definition; *c; ++c)
        switch(*c){
            case ' ':
                [[fallthrough]];
            case '-':
                *c = '_';
                break;
            }
}

#endif
