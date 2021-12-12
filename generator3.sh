g++ -o language -std=c++20 -Ofast -fprofile-arcs \
        -floop-parallelize-all -floop-nest-optimize \
        -march=native generator.cpp -lstdc++ $@