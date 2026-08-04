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

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(DEPS) $(EXEC)
