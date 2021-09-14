g++ -std=c++20 -O0 main.cpp -o main -g -lstdc++ -ljson11 $@ && \
gdb ./main core