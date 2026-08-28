CXX = g++
CXXFLAGS = -std=c++11 -I headers
SRC = $(wildcard src/*.cpp)
HDR = $(wildcard headers/*.hpp)
TARGET = bin/interpreter

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# kept because README documents this name as the build command
fastcreate: $(TARGET)

test1: $(TARGET)
	$(TARGET) < tests/test1.txt --verbose
test2: $(TARGET)
	$(TARGET) < tests/test2.txt --verbose
test3: $(TARGET)
	$(TARGET) < tests/test3.txt --verbose
test4: $(TARGET)
	$(TARGET) < tests/test4.txt --verbose
test5: $(TARGET)
	$(TARGET) < tests/test5.txt --verbose

clean:
	rm -f $(TARGET)

.PHONY: all fastcreate test1 test2 test3 test4 test5 clean
