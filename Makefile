CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -Werror -O3

all: byz

byz: byz.cpp
	$(CXX) $(CXXFLAGS) -o byz byz.cpp

clean:
	rm -f byz

