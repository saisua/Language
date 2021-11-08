#include <string>
#include <cmath>

std::string product(std::string l, int r)
{
    std::string res = "";
    for (; r; --r)
        res += l;
    return res;
}

std::string product(std::string l, float r){
    std::string res = "";
    int fl_r = (int)std::floor(r);
    r -= fl_r;
    for (; fl_r; --fl_r)
        res += l;

    res += l.substr(0, (int)std::floor(r * l.size()));

    return res;
}