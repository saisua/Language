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


std::string read_str_file(const std::string& filename);
json11::Json get_object_from_path(json11::Json & doc, const std::string & path);
inline uint_fast8_t file_id(const std::string id);
std::string get_string_from_path(json11::Json & doc, const std::string & path);
inline std::string get_path(std::string path, uint id);
std::vector<std::string> get_recursive_strings(json11::Json &doc, const std::string path);

inline void generate_codes(Mreg_gen<uintptr_t> & data){
    std::fstream out;

    out.open((LANG_DEFINITION_PATH "codes//" LANG_FROM ".h"), std::ios::out);

    {
    std::string ward_start = "#ifndef LANG_DEFINITION_CODES_H\n#define LANG_DEFINITION_CODES_H\n\n";
    out.write(ward_start.c_str(), ward_start.length());
    }

    for(auto nickname_pair : data.added_nicknames){
        std::string line = ("#define " + nickname_pair.first + " " 
                            + std::to_string(nickname_pair.second) + "\n");
        
        out.write(line.c_str(), line.length());
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

    // This maps a nickname to its corresponding capture groups.
    // When a subgroup needs to be set in a different order,
    // this map should project that. RN unused
    std::unordered_map<std::string, std::unordered_map<uint, uint>> nick_groups_order {};

    for(auto & nickname_pair : data.added_nicknames){
        printf("Get object of definition : %s\n", nickname_pair.first.c_str());
        json11::Json active = get_object_from_path(definition, nickname_pair.first);

        if(active.is_object())
            for(auto & group_pair : active.object_items()){
                int capture_pos = group_pair.second["groups"].int_value();

                std::string group_name = nickname_pair.first + "_" + group_pair.first;

                printf("  Defined: %s\n", group_name.c_str());

                definitions += ("#define definition_" + group_name + " " + 
                                            std::to_string(capture_pos) + "\n");
            }
    }

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

        // Check for sub-group types

        std::stack<json11::Json> conditions = std::stack<json11::Json>({active_trans});
        std::stack<uint> objects_seen = std::stack<uint>({0});
        std::stack<bool> was_condition = std::stack<bool>({true});

        std::string condition = "";

        uint depth = 0;

        while (!conditions.empty()){
            printf(" %d remaining conditions.\n", conditions.size());
            json11::Json active = conditions.top();
            uint seen = objects_seen.top();
            bool is_condition = was_condition.top();

            printf("1st ~ %d %d ~ depth: %u\n", seen, active.object_items().size(), depth);
            if(seen == active.object_items().size() || !active.object_items().size()){
                if(depth){
                    --depth;
                    switch_cases += "\t}\\\n";
                }

                conditions.pop();
                objects_seen.pop();
                was_condition.pop();

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
                    // I have to think if there is any way of generating any other
                    // code that is not from language. Maybe from trans??
                    for(std::string & final_code : get_recursive_strings(
                                                        files[final_id-1], 
                                                        get_path(lang_path, final_id))){
                        switch_cases += std::string(depth+1, '\t') + 
                                        "\"" + final_code + "\"\\\n";
                    }

                }

                if(depth){
                    --depth;

                    switch_cases += std::string(depth+1, '\t') + "}\\\n";
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

                    condition.assign(group_pair.first);
                } else {
                    // Since this was not a condition,
                    // next is either a condition
                    // or directly code.
                    was_condition.push(true);

                    uint id = file_id(group_pair.first);

                    printf("Detected path %s in file %d\n", get_path(group_pair.first, id).c_str(), id);

                    switch_cases += std::string(depth+1, '\t') + "if(data[";

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
                    if(found_sub_conditions.size()){
                        found_sub_conditions.push_back(condition.substr(last));

                        auto rit = found_sub_conditions.crbegin();
                        bool first = true;
                        for(; rit != found_sub_conditions.crend(); ++rit){
                            if(! first)
                                switch_cases += "data[";
                            else
                                first = false;
                        
                            switch_cases += "definition_" + nickname_pair.first + "_" + *rit + " + ";
                        }

                        switch_cases += "pos";
                        switch_cases += std::string(found_sub_conditions.size(), ']');
                    } else {
                        switch_cases += "definition_" + nickname_pair.first + "_" + condition + " + pos]";
                    }
                    switch_cases += " == " + get_path(group_pair.first, id) +"){ \\\n";

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

        switch_cases += "\tbreak;\\\n";
    }
    switch_cases += "\n";

    printf("END\n"); fflush(stdout);

    std::fstream out;
    out.open((LANG_DEFINITION_PATH "generated//" LANG_FROM ".h"), std::ios::out);

    {
    std::string ward_start = "#ifndef LANG_DEFINITION_GENERATED_H\n#define LANG_DEFINITION_GENERATED_H\n\n";
    out.write(ward_start.c_str(), ward_start.length());
    }

    out.write(definitions.c_str(), definitions.length());
    out.write("\n", 1);
    out.write(switch_cases.c_str(), switch_cases.length());

    std::string ward_end = "\n#endif\n\n";
    out.write(ward_end.c_str(), ward_end.length());
}

#endif