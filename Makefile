# Compiler and flags.
CXX := g++ -std=c++17 -pthread
FLAGS := -Wall -Werror -Wextra -Wconversion -pedantic -Wfloat-equal -Wduplicated-branches -Wduplicated-cond -Wshadow -Wdouble-promotion -Wundef
OPT := -O3 -DNDEBUG
DEBUG := -g3 -DDEBUG

# Executable name and linked files without extensions.
EXE := benchmark
LINKED := framework

# Build optimized executable.
release : $(EXE).cpp $(LINKED).cpp
	$(CXX) $(FLAGS) $(OPT) -c $(EXE).cpp $(LINKED).cpp
	$(CXX) $(FLAGS) $(OPT) $(EXE).o $(LINKED).o -o $(EXE)

# Build with debug features.
debug : $(EXE).cpp $(LINKED).cpp
	$(CXX) $(FLAGS) $(DEBUG) -c $(EXE).cpp $(LINKED).cpp
	$(CXX) $(FLAGS) $(DEBUG) $(EXE).o $(LINKED).o -o $(EXE)

# Remove executable binary and generated objected files.
.PHONY : clean
clean : 
	rm -f $(EXE) $(EXE).o $(LINKED).o
