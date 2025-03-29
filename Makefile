CXX = g++
CXXFLAGS = -std=c++17

all: byz

byz: byz.cpp
	$(CXX) $(CXXFLAGS) -o byz byz.cpp

clean:
	rm -f byz
