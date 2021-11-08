#ifndef LANG_FILE_READ_CPP
#define LANG_FILE_READ_CPP

#include <string>
#include <fstream>
#include <exception>

std::string read_str_file(const std::string &filename)
    __attribute__((always_inline));

std::string read_str_file(const std::string& filename)
{
    std::string line;
    std::string result;
    std::fstream file;

    /*  Open the language JSON file as a fstream */
    file.open(filename, std::ios::in);
    if (file.is_open()){
        /*  Start concatenating the lines found in the resulting string
            "language_str". The '\0' parameter should make it get the entire
            file all at once, but just in case, I keep it running in the while */
        while(getline(file, line, '\0')){
            result += line;
        }
        /*  Close the fstream */
        file.close();
    } else
        throw std::runtime_error("Fail on file reading");

    return result;
}

#endif