#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <queue>
#include <algorithm>

#include "hyperscan/src/hs.h"
#include <rapidjson/document.h>
#include <omp.h>

using val_iterator = std::pair<rapidjson::Value::ConstMemberIterator,rapidjson::Value::ConstMemberIterator>;

// Forward declarations
std::queue<std::string> rj_all_string_in_obj(std::queue<val_iterator>&remaining);

// global compiler time settings
//#define DEBUG false

// IMPORTANT: ASSUME ALL VALUES IN standalones ARE STRING
std::vector<std::string> clean(rapidjson::Document &lang){
    std::queue<val_iterator> start = std::queue<val_iterator>();
    start.push(val_iterator{lang.FindMember("standalones"), lang.MemberEnd()});
    std::queue<std::string> raw_regex = rj_all_string_in_obj(start);

    std::vector<std::string> result;

    std::regex REF_REG ("(?=[^\\\\]|^)%(.*?[^\\\\])%");
    // Regex string matches
    std::smatch matches;
    // Fake matches
    std::smatch fm; 
    std::string modified;
    // Permutation groups
    std::vector<std::string> perm_groups;
    std::vector<std::string> temp;
    int pos = 0;
    
    #ifdef DEBUG
    printf("%i raw regex\n", raw_regex.size());
    #endif

    // TODO: Parallelize
    // This is a n^4 algorithm
    // Although usually it is like a n^3
    while(!raw_regex.empty()){
        perm_groups = std::vector<std::string>{""};
        temp = std::vector<std::string>();

        modified = raw_regex.front();
        raw_regex.pop();

        // maybe should be changed to regex_match? This actually works
        // or regex_iterator
        while(regex_search(modified, matches, REF_REG)){
            for(std::string &t : perm_groups)
                t += modified.substr(0, matches.position());

            // Search in json using groups
            // generate combinations, then
            // if % in json val, raw_regex.push
            // else, result.push_back
            rapidjson::Value::ConstMemberIterator search_iter = lang.MemberBegin();

            std::string match = matches[0];
            match.pop_back();
            const char* char_match = match.c_str()+1;
            match.clear();

            #ifdef DEBUG
            printf("\nPath: ");
            #endif
            // Extract the json path of the replace values
            // Keep comparison to last '\0' just in case any
            // tag in the file contains a "\\%"
            for(; *char_match != '\0'; ++char_match){
                if(*char_match == '_'){
                    #ifdef DEBUG
                    std::cout << match << " > " << std::flush;
                    #endif

                    for(; (*search_iter).name != match.c_str(); search_iter++);
                    search_iter = (*search_iter).value.MemberBegin();
                    
                    match.clear();
                } else 
                    match += *char_match;
            }
            #ifdef DEBUG
            std::cout << match << std::endl;
            #endif
            for(; (*search_iter).name != match.c_str(); search_iter++);
            
            const rapidjson::Value& found = (*search_iter).value;

            uint actual_str = perm_groups.size();
            // Permutate the original strings using the found replace values
            switch(found.GetType()){
                case 5: // String
                    {
                    #ifdef DEBUG
                    std::cout << "Generated strings from string:" << std::endl;
                    #endif
                    for(std::string &t : perm_groups){
                        t += found.GetString();

                        #ifdef DEBUG
                        std::cout << "\t" << t << std::endl;
                        #endif
                    }
                    }
                    
                    break;
                case 4: // Array
                    {
                    #ifdef DEBUG
                    for(const std::string &t : perm_groups)
                        std::cout << ":" << t << std::endl;

                    std::cout << "Generated strings from array:" << std::endl;
                    #endif
                    for(auto val_iter=found.GetArray().Begin();
                                val_iter != found.GetArray().End(); ++val_iter){
                        for(const std::string t : perm_groups){
                            temp.emplace_back(std::string(t+val_iter->GetString()));

                            #ifdef DEBUG
                            std::cout << "\t" << temp.back() << std::endl;
                            #endif
                        }
                    }

                    perm_groups = temp;
                    temp = std::vector<std::string>();
                    }
                    break;
                case 3: // Object
                    {
                    start = std::queue<val_iterator>();
                    start.push(val_iterator{found.GetObject().MemberBegin(), found.GetObject().MemberEnd()});
                    std::queue<std::string> found_perm = rj_all_string_in_obj(start);

                    #ifdef DEBUG
                    for(const std::string &t : perm_groups)
                        std::cout << ":" << t << std::endl;

                    std::cout << "Generated strings from object:" << std::endl;
                    #endif
                    while(!found_perm.empty()){
                        for(std::string &t : perm_groups){
                            temp.push_back(t+found_perm.front());

                            #ifdef DEBUG
                            std::cout << "\t" << temp.back() << std::endl;
                            #endif
                        }

                        found_perm.pop();
                    }

                    perm_groups = temp;
                    temp = std::vector<std::string>();
                    }
                    break;
                default:
                    printf("WRONG TYPE FOUND %i\n", found.GetType());
            }
            pos = matches.position() + matches.length();
            
            if(pos >= modified.length()) {
                modified = "";
                break;
            }

            modified = modified.substr(pos);
        }
        
        for(std::string &t : perm_groups)
            t += modified;

        #ifdef DEBUG
        printf("###### CHECK #######");
        #endif
        // Really slow, should parallelize
        for(std::string &t : perm_groups){
            #ifdef DEBUG
            std::cout << "? " << t << std::endl;
            #endif
            // if %% in text
            if(regex_search(t, fm, REF_REG))
                raw_regex.push(t);
            else
                result.push_back(t);
        }
        
    }

    return result;
}

int main(){
    std::string line;
    std::string language_str;
    std::fstream lang_file;

    lang_file.open("aucpp.json",std::ios::in);
    if (lang_file.is_open()){
        while(getline(lang_file, line, '\0')){
            language_str += line;
        }
        lang_file.close();
    } else 
        std::cout << "Fail on file reading" << std::endl;

    rapidjson::Document lang;
    lang.Parse(language_str.c_str());
    std::cout << (lang.HasParseError() ? "Fail on parse" : "File parse: ok") << std::endl;
    std::vector<std::string> regexps = clean(lang);

    printf("\n");
    for(std::vector<std::string>::const_iterator str_iter=regexps.cbegin(); 
                str_iter != regexps.cend(); ++str_iter){
        std::cout << *str_iter << std::endl;
    }
    //hs_compile();

    return 0;
}

std::queue<std::string> rj_all_string_in_obj(std::queue<val_iterator> &remaining){
    std::queue<std::string> result = std::queue<std::string>();

    while(!remaining.empty()){
        #ifdef DEBUG
        printf("###\n");
        #endif
        for (rapidjson::Value::ConstMemberIterator memb_iter = remaining.front().first;
                    memb_iter != remaining.front().second; ++memb_iter)
        {
            #ifdef DEBUG
            printf("Found Member %s", memb_iter->name.GetString());
            #endif
            switch(memb_iter->value.GetType()){
                case 3: // Object
                    #ifdef DEBUG
                    printf(" (object)\n");
                    #endif
                    remaining.push(val_iterator{
                            memb_iter->value.GetObject().MemberBegin(),
                            memb_iter->value.GetObject().MemberEnd()});
                    continue;
                case 4: // Array
                    #ifdef DEBUG
                    printf(" (array)\n");
                    #endif
                    
                    for(auto val_iter=memb_iter->value.GetArray().Begin();
                                val_iter != memb_iter->value.GetArray().End(); ++val_iter){
                        result.push(val_iter->GetString());
                    }
                    continue;
                case 5: // string
                    #ifdef DEBUG
                    printf(" (string)\n");
                    #endif
                    result.push(memb_iter->value.GetString());
            }
        }
        remaining.pop();
    }
    #ifdef DEBUG
    printf("\n");
    #endif

    return result;
}