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
json11::Json get_object_from_path(json11::Json &doc, const std::string &path);
int apply_step(std::fstream& out, const std::vector<std::string> & file_list, 
                                    const std::string & filename_replace,
                                    const std::string & tag_path, 
                                    const std::string & trans_access,
                                    int file_id);
                                    
#define generated_tag_path "src//"
#define generated_tag_name "generated_tag_list"
#define generated_tag_extension ".h"
inline void generate_step_char(
                            std::string & from_lang_name,
                            std::string & to_lang_name,
                            const std::vector<std::string> & from_filenames = {LANG_GEN_RENAME_FROM},
                            const std::vector<std::string> & to_filenames = {LANG_GEN_RENAME_TO},
                            const std::string & tag_path="Reserved-regex_Tag",
                            const std::string & trans_access="Reserved-regex_Line-Attribute-Access"){

    std::fstream out;

    //printf("TESTTEST\n");

    out.open(generated_tag_path generated_tag_name generated_tag_extension, std::ios::out);

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

    int files_added = apply_step(out, from_filenames, LANG_FROM, tag_path, trans_access, 0);
    apply_step(out, to_filenames, LANG_TO, tag_path, trans_access, files_added);

    printf("\n");
    out << "\n\n#define LANG_FROM \"" << from_lang_name 
            << "\"\n#define LANG_TO \"" << to_lang_name
            << "\"\n\n#endif";
    out.close();
}

int apply_step(std::fstream& out, const std::vector<std::string> & file_list, 
                                    const std::string & filename_replace,
                                    const std::string & tag_path, 
                                    const std::string & trans_access,
                                    int file_id = 0){

    std::smatch m;
    std::regex name_extract ("\\d+-(\\w+)\\/+\\w+");
    std::regex str_format ("%s");

    std::string err = "";

    for(std::string filename : file_list){
        filename = std::regex_replace(filename, str_format, filename_replace);
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

            if(filename.find("3-Translation")){
                json11::Json modifiers = get_object_from_path(doc, trans_access);
                std::string pos_modifier = std::string();
                
                for(auto & modifier : modifiers.object_items())
                    pos_modifier += "#define PATH_" + modifier.first + " '" + modifier.second.string_value() + "'\n";
                
                out.write(pos_modifier.c_str(), pos_modifier.length());
            }

            std::string to_write2 = "#define " + m[1].str() + "_id " + std::to_string(++file_id) + "\n";

            out.write(to_write2.c_str(), to_write2.length());
        }
    }
    return file_id;
}

#ifdef Syntax_match_symbol
// Instead of store Syntax_match and id, store a constexpr array of symbols. Its id is the index of the array.
std::regex Syntax_match_symbol_regex (Syntax_match_symbol".*?"Syntax_match_symbol);
std::regex Definition_match_symbol_regex (Definition_match_symbol".*?"Definition_match_symbol);
std::regex Translation_match_symbol_regex (Translation_match_symbol".*?"Translation_match_symbol);
std::regex Language_match_symbol_regex (Language_match_symbol".*?"Language_match_symbol);
inline uint_fast8_t find_id(const std::string id){
    std::smatch m;
    uint_fast8_t id_num;
    uint start = id.length();

    if(std::regex_search(id, m, Syntax_match_symbol_regex)){
        start = m.position();
        id_num = Syntax_id;
    }
    if(std::regex_search(id, m, Definition_match_symbol_regex) && m.position() < start){
        start = m.position();
        id_num = Definition_id;
    }
    if(std::regex_search(id, m, Translation_match_symbol_regex) && m.position() < start){
        start = m.position();
        id_num = Translation_id;
    }
    if(std::regex_search(id, m, Language_match_symbol_regex) && m.position() < start){
        id_num = Language_id;
    }

    return id_num;
}

inline std::string find_path(std::string path, uint start = 0){
    if(start != 0)
        if(start > path.length()) return "";
        else 
            path = path.substr(start);

    start = path.length();

    std::smatch m;
    uint end;

    if(std::regex_search(path, m, Syntax_match_symbol_regex)){
        start = m.position();
        end = m.length();
    }
    if(std::regex_search(path, m, Definition_match_symbol_regex) && m.position() < start){
        start = m.position();
        end = m.length();
    }
    if(std::regex_search(path, m, Translation_match_symbol_regex) && m.position() < start){
        start = m.position();
        end = m.length();
    }
    if(std::regex_search(path, m, Language_match_symbol_regex) && m.position() < start){
        start = m.position();
        end = m.length();
    }

    return std::string(path.begin() + start + 1, path.begin() + start + end - 1);
}


inline std::pair<uint, std::string> find_id_path(std::string path, uint & start){
    if(start != 0)
        if(start > path.length()) return std::make_pair(0, "");
        else 
            path = path.substr(start);
        
    start = path.length();

    printf("Searching for an id in %s\n", path.c_str());

    std::smatch m;
    uint_fast8_t id;
    uint end;

    if(std::regex_search(path, m, Syntax_match_symbol_regex)){
        start = m.position();
        end = m.length();
        id = Syntax_id;
    }
    if(std::regex_search(path, m, Definition_match_symbol_regex) && m.position() < start){
        start = m.position();
        end = m.length();
        id = Definition_id;
    }
    if(std::regex_search(path, m, Translation_match_symbol_regex) && m.position() < start){
        start = m.position();
        end = m.length();
        id = Translation_id;
    }
    if(std::regex_search(path, m, Language_match_symbol_regex) && m.position() < start){
        start = m.position();
        end = m.length();
        id = Language_id;
    }

    if(start == path.length())
        return std::make_pair(0, std::string());

    printf("  Found %s\n", std::string(path.begin() + start + 1, path.begin() + start + end - 1).c_str());

    return std::make_pair<uint, std::string>(id, std::string(path.begin() + start + 1, path.begin() + start + end - 1));
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
#endif

#ifdef Syntax_id
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
#endif