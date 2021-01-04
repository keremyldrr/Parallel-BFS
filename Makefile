all:
	g++ hybrid-prefix2.cpp -fopenmp -O3 -std=c++11 -o hybrid -mavx -mavx2
