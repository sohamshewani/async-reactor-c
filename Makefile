CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -Iinclude -pthread
LDFLAGS = -pthread

SRC_DIR = src
TEST_DIR = tests
OBJ_DIR = obj
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c, $(OBJ_DIR)/test_%.o, $(TEST_SRCS))

# Exclude main.o from test library if it exists, or build a static lib
LIB_OBJS = $(filter-out $(OBJ_DIR)/main.o, $(OBJS))

TARGET = $(BIN_DIR)/async-reactor
TEST_TARGET = $(BIN_DIR)/run-tests

.PHONY: all clean test directories

all: directories $(TARGET) $(TEST_TARGET)

directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJS)
	$(CC) $(LIB_OBJS) $(TEST_OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)