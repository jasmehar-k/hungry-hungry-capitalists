CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -MMD -MP
SOURCES  = $(wildcard *.cpp)
OBJECTS  = $(SOURCES:.cpp=.o)
DEPS     = $(OBJECTS:.o=.d)
EXEC     = arcanemarket

$(EXEC): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXEC)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Auto-generated header dependencies, one .d per object.
-include $(DEPS)

# Statistical check of the probability tables and the shuffled deck. Lives in
# tests/ so the wildcard above never links its main() into the game.
tabletest: tests/tabletest.cpp cards.o
	$(CXX) $(CXXFLAGS) -I. tests/tabletest.cpp cards.o -o tabletest

.PHONY: test
test: tabletest
	./tabletest

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(DEPS) $(EXEC) tabletest tabletest.d
