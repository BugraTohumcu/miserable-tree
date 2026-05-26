CXX = g++
CXXFLAGS = -std=c++17 -Wall

SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = tests

# --- Main build ---
# All sources EXCEPT main.cpp goes to shared objects
MAIN_SRC = main.cpp
SRCS     = $(shell find $(SRC_DIR) -name "*.cpp")
OBJS     = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = program

# --- Test build ---
TEST_SRCS   = $(shell find $(TEST_DIR) -name "*.cpp")
TEST_OBJS   = $(patsubst $(TEST_DIR)/%.cpp, $(OBJ_DIR)/tests/%.o, $(TEST_SRCS))
TEST_TARGET = run_tests

# Shared objects: everything except main.cpp (ui has main, we can't link it twice)
SHARED_OBJS = $(filter-out $(OBJ_DIR)/$(basename $(MAIN_SRC)).o, $(OBJS))

# ──────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS) $(OBJ_DIR)/main.o
	$(CXX) $(CXXFLAGS) $(OBJS) $(OBJ_DIR)/main.o -o $(TARGET)

$(OBJ_DIR)/main.o: main.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c main.cpp -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ──────────────────────────────────────────────
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(SHARED_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(SHARED_OBJS) $(TEST_OBJS) -o $(TEST_TARGET)

$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

# ──────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_TARGET)

.PHONY: all test clean