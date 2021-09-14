// Generate switch

#include <string>
#include <vector>
#include <iostream>
#include <regex>
#include <queue>
#include <algorithm>

#include <json11.hpp>

#include "../lang_compile.h"
#include "utils/file_read.cpp"
#include "generated_tag_list.h"

std::string get_string_from_path(json11::Json &doc, const std::string & path);
std::string read_str_file(const std::string& filename);

#define generated_tag_path "src//"
#define generated_tag_name "generated_tag_list"
#define generated_tag_extension ".h"
inline void generate_step_char(std::fstream& out,
                            const std::string & from_lang_name,
                            const std::string & to_lang_name,
                            const std::vector<std::string>& filenames = {{
                                    LANG_GEN_RENAME_FROM, LANG_GEN_RENAME_TO}},
                            const std::string& tag_path="Reserved-regex_Tag"){

    {
    std::string path = generated_tag_path;
    path.append(generated_tag_name);
    path.append(generated_tag_extension);

    out.open(path.c_str(), std::ios::out);
    }

    if(!out.is_open())
        printf("ERROR!");
    {
    std::string TMP = "";

    for(char gen_char : generated_tag_name)
        TMP.push_back(std::toupper(gen_char));
    TMP.pop_back();

    std::vector<std::string> to_write {{"#ifndef ", TMP, "_H\n#define ", TMP, "_H\n\n"}};
    for(std::string part : to_write)
        out.write(part.c_str(), part.length());
    }

    std::smatch m;
    std::regex name_extract ("\\d+-(\\w+)\\/+\\w+");
    std::regex str_format ("%s");

    std::string err = "";

    int file_id = 0;
    for(std::string filename : filenames){
        filename = std::regex_replace(filename, str_format, current_compilation);


        json11::Json doc = json11::Json::parse(read_str_file(filename), err);
        if(err.size())
            printf("%s\n", err.c_str());
            

        std::string tag = get_string_from_path(doc, tag_path);

        if(std::regex_search(filename, m, name_extract)){
            #if ! LANG_VERBOSE
            printf("Detected %s as a match in %s\n", tag.c_str(), m[1].str().c_str());
            #endif

            std::string to_write = "#define " + m[1].str() + "_match_symbol \"" + tag + "\"\n";
            
            out.write(to_write.c_str(), to_write.length());

            std::string to_write2 = "#define " + m[1].str() + "_id " + std::to_string(++file_id) + "\n";

            out.write(to_write2.c_str(), to_write2.length());
        }
    }


    printf("\n");
    out << "\n\n#define LANG_FROM \"" << from_lang_name 
            << "\"\n#define LANG_TO \"" << to_lang_name
            << "\"\n\n#endif";
    out.close();
}

#define check_id(x) (id.starts_with(x) && id.ends_with(x))
inline uint_fast8_t file_id(const std::string id){
    if(check_id(Syntax_match_symbol))
        return Syntax_id;
    else if(check_id(Definition_match_symbol))
        return Definition_id;
    else if(check_id(Translation_match_symbol))
        return Translation_id;
    else if(check_id(Language_match_symbol))
        return Language_id;
    
    return 0u;
}

inline std::string get_path(std::string path, uint id = 0){
    if(id == 0)
        id = file_id(path);

    if(id == Syntax_id)
        return std::regex_replace(path, std::regex(Syntax_match_symbol), "");
    else if(id == Definition_id)
        return std::regex_replace(path, std::regex(Definition_match_symbol), "");
    else if(id == Translation_id)
        return std::regex_replace(path, std::regex(Translation_match_symbol), "");
    else if(id == Language_id)
        return std::regex_replace(path, std::regex(Language_match_symbol), "");
}