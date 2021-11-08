#ifndef LANG_DEFINITION_GEN_CPP
#define LANG_DEFINITION_GEN_CPP

#include <fstream>
#include <string>
#include <json11.hpp>

#include "Mreg-gen.cpp"
#include "../lang_compile.h"
#include "generated_tag_list.h"
#include "utils/file_read.cpp"
#include "regex_perm.cpp"
#include "language_gen.cpp"


std::string read_str_file(const std::string& filename);
json11::Json get_object_from_path(json11::Json & doc, const std::string & path);
inline uint_fast8_t file_id(const std::string id);
std::string get_string_from_path(json11::Json & doc, const std::string & path);
inline std::string get_path(std::string path, uint id);
std::vector<std::string> get_recursive_strings(json11::Json &doc, const std::string path);
void solve_code(std::string & code, json11::Json & doc);
void prettify_definition(std::string & definition);
void prettify_definition(char * definition);

inline void generate_codes(Mreg_gen<uintptr_t> & data){
    std::fstream out;
    std::fstream current_language;

    std::string current_language_path = LANG_DEFINITION_PATH "codes//" LANG_FROM ".h";

    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::ios_base::app);
    current_language << "#include \"";
    current_language << current_language_path;
    current_language << "\"\n";
    current_language.close();

    out.open(current_language_path, std::ios::out);

    {
    std::string ward_start = "#ifndef LANG_DEFINITION_CODES_H\n#define LANG_DEFINITION_CODES_H\n\n";
    out.write(ward_start.c_str(), ward_start.length());
    }

    printf("Got %d nicknames\n", data.added_nicknames.size());
    std::unordered_set<std::string> parents = std::unordered_set<std::string>();

    for(auto nickname_pair : data.added_nicknames){
        std::string nickname = nickname_pair.first;

        std::string::iterator reverse_nick = nickname.begin();
        for (uint letter = 0; letter != nickname.length(); ++letter){
            if (*reverse_nick == '_')
               parents.emplace(nickname.substr(0, letter));
            ++reverse_nick;
        }

        printf("Added nickname %s\n", nickname.c_str());
    
        prettify_definition(nickname);
        std::string line = ("#define " + nickname + " " 
                            + std::to_string(nickname_pair.second) + "\n");
        
        out.write(line.c_str(), line.length());
    }
    uint id = data.added_nicknames.size();

    printf(" from parents:\n");
    for(std::string parent : parents){
        if(! data.added_nicknames.count(parent)){
            printf("  %s\n", parent.c_str());
            
            prettify_definition(parent);
            std::string line = ("#define " + parent + " " 
                                + std::to_string(++id) + "\n");
            
            out.write(line.c_str(), line.length());
        }
    }

    {
    std::string ward_end = "\n#endif";
    out.write(ward_end.c_str(), ward_end.length());
    }

    out.close();
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
    std::string translation_code = "";
    

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
        
        if(active_trans.is_null() || active_def.is_null()){
            printf("Not found %s in any of the files\n\n", nickname_pair.first.c_str());
            continue;
        }

        printf("Got object from definition and translation:\n  %s\n\n", 
                                                        nickname_pair.first.c_str());

        switch_cases += ("case " + nickname_pair.first + ": \\\n");
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

                    translation_code += " \\\n\tadd_line({";
                    // I have to think if there is any way of generating any other
                    // code that is not from language. Maybe from trans??
                    for(std::string & final_code : get_recursive_strings(
                                                        files[final_id-1], 
                                                        get_path(lang_path, final_id))){
                        solve_code(final_code, language_patterns, language);

                        translation_code += " \\\n\t\"" + final_code + "\",";
                    }
                    translation_code.pop_back();
                    translation_code += "})\n";
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

                    if(set_pos)
                        switch_cases += "(pos = ";

                    switch_cases += "data[";

                    std::vector<std::string> found_sub_conditions {};

                    uint last = 0, pos = 0;
                    while((pos = condition.find('_', last)) < condition.size()){
                        found_sub_conditions.push_back(condition.substr(last, pos-last));

                        last = pos+1;
                    }

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

                            std::string sub_condition = "";

                            if (active_path.rfind("standalones", 0) == 0)
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

                        switch_cases += std::string(found_sub_conditions.size(), ']');
                    } else {
                        std::string full_cond_end = "";
                        if (active_path.rfind("standalones", 0) == 0)
                            full_cond_end += "definition_";
                                
                        full_cond_end += active_path + "_" + condition;

                        prettify_definition(full_cond_end);
                        switch_cases += full_cond_end + " + pos]";
                    }

                    if(set_pos)
                        switch_cases += ")";

                    switch_cases += " == ";

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
                            switch_cases += path + "){ \\\n" + std::string(depth + 2, '\t') + "pos = root; \\\n";

                            break;
                            
                        case PATH_PWD:
                            printf("New path from current\n");

                            path = get_path(group_pair.first.substr(1), id);

                            path_seen.push(path);

                            prettify_definition(path);
                            switch_cases += path + "){ \\\n";

                            break;

                        case PATH_SET:
                            if(std::regex_search(group_pair.first, match, std::regex(PATH_SET + "(.*?)" + PATH_SET))){
                                std::string new_path = match[1];

                                printf("New path set: %s\n", new_path.c_str());

                                path_seen.push(new_path);;

                                path = get_path(group_pair.first.substr(new_path.size() + 2), id);

                                prettify_definition(path);
                                switch_cases += path + "){ \\\n";

                                break;
                            }
                            [[fallthrough]];

                        default:
                            path = get_path(group_pair.first, id);

                            prettify_definition(path);
                            switch_cases += path +"){ \\\n";

                            path_seen.push(active_path);
                            break;
                        }

                        /*
                    json11::Json attribute = get_object_from_path(
                            files[id-1],
                            get_path(group_pair.first, id)
                        );
                    
                     I think i messed up. I only need id
                    // What to do in case it is an object?
                    if(attribute.is_string()){
                        printf("Added if clausule\n");

                        if(! seen ){
                            // Add if
                            switch_cases += ("if(" + condition + " == "
                                                + attribute.string_value() +"){ \\\n");
                        } else {
                            // Add else if
                            switch_cases += ("else if(" + condition + " == "
                                                + attribute.string_value() +"){ \\\n");
                        }
                    } else if(attribute.is_array()){
                        printf("Added array if clausules\n");

                        switch_cases += "if(";

                        bool first = true;
                        for(auto & value : attribute.array_items()){
                            if(! first){
                                switch_cases += " || \\\n\t";
                            } else {
                                first = false;
                            }
                                
                            switch_cases += (condition + " == \"" + value.string_value() + "\"");
                        }

                        switch_cases += "){ \\\n";
                    } else {
                        printf("Unknown value of type %d\n", attribute.type());
                    }
                    */

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

    std::string current_language_path = LANG_DEFINITION_PATH "generated//" LANG_FROM ".h";
    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::fstream::app);
    current_language << "#include \"";
    current_language << current_language_path;
    current_language << "\"\n";
    current_language.close();

    std::fstream out;
    out.open(current_language_path, std::ios::out);

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

    current_language_path = "code_groups//" LANG_TO ".h";
    
    current_language.open(LANG_LANGUAGES_FOLDER "current_language.h", std::ios_base::app);
    current_language << "#include \"";
    current_language << current_language_path;
    current_language << "\"\n";
    current_language.close();

    std::fstream translation_out;

    translation_out.open(current_language_path, std::ios::out);

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