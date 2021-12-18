g++ -o language -std=c++20 -Ofast \
	-floop-nest-optimize \
	-fwhole-program \
	-Wno-coverage-mismatch \
	-march=native generator.cpp -lstdc++ $@
