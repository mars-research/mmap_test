

CXX := g++
CXXFLAGS := -std=c++2a -Wall -g
PERF_FLAGS := -g --call-graph dwarf
PERF_REPORT_FLAGS := -s overhead --stdio

SRC := mmap_test.cpp
OBJ := mmap_test

all:
	$(CXX) -o $(OBJ) $(CXXFLAGS) $(SRC)

perf: all
	sudo perf record $(PERF_FLAGS) -- ./$(OBJ)

report:
	sudo perf report $(PERF_REPORT_FLAGS) ./perf.data

clean:
	@rm -f $(OBJ)
