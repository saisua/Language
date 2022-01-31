#ifndef LANG_UMAP_SET
#define LANG_UMAP_SET

#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#define hash_num 5

template <typename T>
auto hash = [] (const std::unordered_set<T> & s){
    std::size_t res = 17 + s.size() * 31 + std::hash<std::size_t>()(s.size());
    auto it = s.begin();

    #if hash_num > 0
    if(hash_num >= s.size()){
    #endif
        for(uint i = 0; i != s.size(); ++i, ++it){
            res += std::hash<T>()(*it) * 31;
        }
    #if hash_num > 0
    } else {
        // Select only the first hash_num elements
        for(uint i = 0; i != hash_num; ++i, ++it){
            res += std::hash<T>()(*it) * 31;
        }
    }
    #endif

    return res;
};

template <typename T>
auto equal = [] (const std::unordered_set<T> & s1, const std::unordered_set<T> & s2){
    if(s1.size() != s2.size()) return false;
    for(auto & i : s1) if(!s2.count(i)) return false;
    
    return true;
};

#define unordered_map_set(T1, T2) std::unordered_map<std::unordered_set<T1>, T2, decltype(hash<T1>), decltype(equal<T1>)>

#endif