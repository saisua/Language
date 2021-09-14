touch ./callgrind.out. && \
touch ./cachegrind.out. && \

rm ./callgrind.out.* && \
rm ./cachegrind.out.* && \

#-no-pie -O0 -fno-builtin
./compile.sh -lprofiler -DFRB_PROFILE -pg -g3 && \


valgrind --tool=cachegrind --read-inline-info=yes ./mreg;

# --toggle-collect=match
valgrind --tool=callgrind --read-inline-info=yes \
    --dump-instr=yes --collect-jumps=yes -v \
    --num-callers=500 --dsymutil=yes --simulate-cache=no \
     ./mreg;

sleep 1s && \
callgrind_annotate callgrind.out.* --inclusive=yes --tree=both \
    --threshold=100 --show-percs=yes \
    | grep -v build | grep -v $(arch) | less -p"match\("
