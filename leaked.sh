./compile.sh $@ && \

valgrind --tool=memcheck --leak-check=full --track-origins=yes\
    --num-callers=500 ./mreg