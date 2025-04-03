CXX = g++
CXXFLAGS = -std=c++23

all: byz

byz: byz.cpp
	$(CXX) $(CXXFLAGS) -Wall -Wextra -Werror -o byz byz.cpp

clean:
	rm -f byz
