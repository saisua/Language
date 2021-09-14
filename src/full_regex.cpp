#ifndef FREGEX_CPP
#define FREGEX_CPP

#include <regex>
#include <string>

typedef std::regex_constants::syntax_option_type flag_type;

// Just a class to enable std::regex as 
// the key of a map.
class Fregex : public std::regex{
    private:
        typedef std::regex super;
    public:
        std::string pattern;

        Fregex(const char* constr_regex) : super(constr_regex) {
            this->pattern = constr_regex;
        }

        Fregex(const char* constr_regex, flag_type f) : super(constr_regex, f) {
            this->pattern = constr_regex;
        }

        friend inline bool operator<(const Fregex& self, const Fregex& other) noexcept {
            return self.pattern < other.pattern;
        }
};

#endif