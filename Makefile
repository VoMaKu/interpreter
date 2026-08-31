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

# run one program with the debug trace: make test3
test%: $(TARGET)
	$(TARGET) < tests/test$*.txt --verbose

# run every program and compare with the recorded output
check: $(TARGET)
	@tests/run_tests.sh

# rewrite the recorded output after a deliberate change
update-tests: $(TARGET)
	@UPDATE=1 tests/run_tests.sh
	@echo "recorded output rewritten, review it with git diff"

clean:
	rm -f $(TARGET)

.PHONY: all fastcreate check update-tests clean
