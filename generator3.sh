g++ -o language -std=c++20 -Ofast \
	-fprofile-arcs -fprofile-use \
	-floop-parallelize-all -floop-nest-optimize \
	-fwhole-program -fbranch-probabilities \
	-Wno-coverage-mismatch \
	-march=native generator.cpp -lstdc++ $@
