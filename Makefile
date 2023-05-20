INC = -Iinc $(addprefix -I,$(wildcard external/tree-similarity/src/*/))
SRC = $(wildcard src/*.cpp)
OBJ = $(subst src/,obj/,$(SRC:.cpp=.o))
CPPFLAGS = $(INC)
CXXFLAGS = -std=c++20 -Wall -O3 -march=native

build: main.cpp ${OBJ} | bin/
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o bin/ted
	chmod +x bin/ted

rebuild: clean build

obj/%.o: src/%.cpp | obj/make
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

%/:
	mkdir -p $@

.PHONY: clean

clean:
	rm -rf obj bin