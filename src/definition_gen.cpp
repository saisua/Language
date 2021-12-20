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


// We must have a prefix in case we collide
// with other generated definitions of gcc
// since we should not have lower case definitions
#define code_pref "_code_"

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

inline void generate_codes(Mreg_gen<uintptr_t> & data){
    std::fstream out;
    std::fstream current_language;

    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::ios_base::app);
    current_language << "#include \"";
    current_language << LANG_DEFINITION_FOLDER "codes//" LANG_FROM ".h";
    current_language << "\"\n";
    current_language.close();

    out.open(LANG_DEFINITION_PATH "codes//" LANG_FROM ".h", std::ios::out);

    {
    std::string ward_start = "#ifndef LANG_DEFINITION_CODES_H\n#define LANG_DEFINITION_CODES_H\n\n";
    out.write(ward_start.c_str(), ward_start.length());
    }

    printf("Got %d nicknames\n", data.added_nicknames.size());
    std::unordered_map<uint, std::unordered_set<std::string>> codes = 
                std::unordered_map<uint, std::unordered_set<std::string>>();
    std::unordered_map<std::string, std::string> parents = 
                std::unordered_map<std::string, std::string>();

    uint max_depth = 0;
    for (auto nickname_pair : data.added_nicknames)
    {
        uint total_depth = count(nickname_pair.first.begin(), nickname_pair.first.end(), '_');

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
            if (*nick == '_'){

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

        uint depth = count(nickname.begin(), nickname.end(), '_');
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
    out.write(ward_end.c_str(), ward_end.length());
    }

    out.close();
}

void update_mreg(Mreg_gen<uintptr_t> &mreg, std::string nickname, uintptr_t new_code){
    uintptr_t old_code = mreg.added_nicknames[nickname];
    mreg.added_nicknames[nickname] = new_code;
    mreg.contains_captures[new_code] = mreg.contains_captures[old_code];
    mreg.contains_captures.erase(old_code);

    for(auto & pair : mreg.added_ids){
        if(pair.second == old_code){
            pair.second = new_code;
        }
    }

    printf("Updating mreg code %d -> %d\n", old_code, new_code);

    for (uintptr_t branch : mreg.final_nicknames[nickname])
    {
        mreg.data[branch + FINAL] = new_code;
    }

    const uint max_size = mreg.data.size() - offset_positions;
    for (uint node = initial_position; node != max_size; node += node_length)
    {
        if(mreg.data[node+WARP_CAPTURES] >> mreg.captures_shift){
            capture_t<uintptr_t> * capture_list = reinterpret_cast<capture_t<uintptr_t> *>(
                                                    mreg.data[node + WARP_CAPTURES]);

                                                
            printf("  check in node %d (0x%X)[%d]\n", node, mreg.data[node+WARP_CAPTURES], capture_list->size());
            for (uintptr_t &capture : *capture_list)
                if(abs(capture) == old_code){
                    capture = capture > 0 ? new_code : -new_code;

                    printf("  Updated node %d\n", node);
                }
        }
    }

    mreg.all_states.insert(new_code);
}

inline void generate_definition_checks(Mreg_gen<uintptr_t> & data, 
                                        std::vector<json11::Json> & files){

    
    std::string err;
    // Since there is no id 0, files id must be -1
    json11::Json definition = files[Definition_id-1];
    json11::Json translation = files[Translation_id-1];
    json11::Json language = files[Language_id-1];

    std::string definitions = "";
    std::string switch_cases = "#define definition_generated \\\n";
    std::string translation_code = "#define COMMA ,\n\n";
    

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
            switch_cases += "case " + nickname_pair.first + ": \\\n\tbreak; \\\n";
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

        std::string case_id = std::string(nickname_pair.first);

        prettify_definition(case_id);
        switch_cases += ("case " code_pref + case_id +": \\\n");
        bool first_in_switch = true;

        // Check for sub-group types

        #define vec_stack(type) std::stack<type, std::vector<type>>
 
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
                    switch_cases += "\t} \\\n";
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

                    switch_cases += std::string(depth + 1, '\t') + "translation_" + print_path + "; \\\n";

                    translation_code += "#define translation_" + print_path;

                    // I have to think if there is any way of generating any other
                    // code that is not from language. Maybe from trans??
                    std::string tmp_code = "";
                    for (std::string &final_code : get_recursive_strings(
                                                                files[final_id - 1],
                                                                get_path(lang_path, final_id)))
                    {
                        solve_code(final_code, language_patterns, language);

                        tmp_code += " \\\n\t\"" + final_code + "\" COMMA";
                    }
                    for(char _ : "COMMA")
                        tmp_code.pop_back();

                    uint code_size = std::count(tmp_code.begin(), tmp_code.end(), '\n');

                    translation_code += " \\\n\tadd_line<" +
                                        std::to_string(code_size) + 
                                        ">({" + tmp_code + "})\n";
                }

                // There should always be depth > 0, but just in case
                if(depth){
                    --depth;

                    switch_cases += std::string(depth+1, '\t') + "} \\\n";

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
                        switch_cases += std::string(depth+1, '\t');
                        switch_cases += "pos = root; \\\n";
                    }
                    first_in_switch = false;
                    switch_cases += std::string(depth+1, '\t');
                    if(seen == 0)
                        switch_cases += "if(";
                    else
                        switch_cases += "else if(";


                    switch_cases += "data[data[";

                    std::vector<std::string> found_sub_conditions {};

                    uint last = 0, pos = 0;
                    while((pos = condition.find('_', last)) < condition.size()){
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
                                switch_cases += "data[";
                            else
                                first = false;

                            //if (active_path.rfind("standalones", 0) == 0)
                            sub_condition += "definition_";
                            sub_condition += active_path + "_" + *rit;

                            prettify_definition(sub_condition);
                            switch_cases += sub_condition + " + ";
                        }

                        /*
                        if(from_root)
                            switch_cases += "root";
                        else*/
                        switch_cases += "pos";

                        switch_cases += std::string(found_sub_conditions.size()-1, ']');
                    } else {
                        //if (active_path.rfind("standalones", 0) == 0)
                        sub_condition += "definition_";
                                
                        sub_condition += active_path + "_" + condition;

                        prettify_definition(sub_condition);
                        switch_cases += sub_condition + " + pos";
                    }

                    switch_cases += "]] ";

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
                                switch_cases += "& " code_pref + path + "_mask == ";
                            else
                                switch_cases += "== ";

                            switch_cases += code_pref + path + "){ \\\n" + std::string(depth + 2, '\t') + "pos = root; \\\n";

                            break;
                            
                        case PATH_PWD:
                            printf("New path from current\n");

                            path = get_path(group_pair.first.substr(1), id);

                            path_seen.push(path);

                            prettify_definition(path);

                            // If it is a parent (not max depth) I need to & slice the id 
                            if( ! max_depth_codes.count(path))
                                switch_cases += "& " code_pref + path + "_mask == ";
                            else
                                switch_cases += "== ";

                            switch_cases += code_pref + path + "){ \\\n";

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
                                    switch_cases += "& " code_pref + path + "_mask == ";
                                else
                                    switch_cases += "== ";

                                switch_cases += code_pref + path + "){ \\\n";

                                break;
                            }
                            [[fallthrough]];

                        default:
                            path = get_path(group_pair.first, id);

                            prettify_definition(path);

                            // If it is a parent (not max depth) I need to & slice the id 
                            if( ! max_depth_codes.count(path))
                                switch_cases += "& " code_pref + path + "_mask == ";
                            else
                                switch_cases += "== ";

                            switch_cases += code_pref + path +"){ \\\n";

                            path_seen.push(active_path);
                            break;
                    }


                    if(set_pos)
                        switch_cases += std::string(depth + 2, '\t') + "pos = data[" + sub_condition + " + pos]; \\\n";

                        ++depth;
                    }
            }
            }
        }

        switch_cases += "\tbreak; \\\n";
    }
    switch_cases += "\n";

    printf("END\n"); fflush(stdout);

    std::fstream current_language;

    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::fstream::app);
    current_language << "#include \"";
    current_language << LANG_DEFINITION_FOLDER "generated//" LANG_FROM ".h";
    current_language << "\"\n";
    current_language.close();

    std::fstream out;
    out.open(LANG_DEFINITION_PATH "generated//" LANG_FROM ".h", std::ios::out);

    {
    std::string ward_start = "#ifndef LANG_DEFINITION_GENERATED_H\n#define LANG_DEFINITION_GENERATED_H\n\n";
    out.write(ward_start.c_str(), ward_start.length());
    }

    out.write(definitions.c_str(), definitions.length());
    out.write("\n", 1);
    out.write(switch_cases.c_str(), switch_cases.length());

    std::string ward_end = "\n#endif\n\n";
    out.write(ward_end.c_str(), ward_end.length());

    out.close();
    
    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::ios_base::app);
    current_language << "#include \"";
    current_language << LANG_TRANSLATION_FOLDER "code_groups//" LANG_TO ".h";
    current_language << "\"\n";
    current_language.close();

    std::fstream translation_out;

    translation_out.open(LANG_TRANSLATION_PATH "code_groups//" LANG_TO ".h", std::ios::out);

    translation_out.write(translation_code.c_str(), translation_code.length());

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