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

# Test harnesses live in tests/ so the wildcard above never links their
# main() into the game. tabletest checks the probability tables statistically;
# gametest drives a headless Game over string streams.
GAME_OBJECTS = $(filter-out main.o,$(OBJECTS))

tabletest: tests/tabletest.cpp cards.o
	$(CXX) $(CXXFLAGS) -I. tests/tabletest.cpp cards.o -o tabletest

gametest: tests/gametest.cpp $(GAME_OBJECTS)
	$(CXX) $(CXXFLAGS) -I. tests/gametest.cpp $(GAME_OBJECTS) -o gametest

.PHONY: test
test: tabletest gametest
	./tabletest
	./gametest

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(DEPS) $(EXEC) tabletest tabletest.d gametest gametest.d
