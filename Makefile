CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -g -O2 -I./include
LDFLAGS = -pthread

SRC_DIR  = src
OBJ_DIR  = build
BIN_DIR  = bin

SRCS     = $(wildcard $(SRC_DIR)/*.c)
OBJS     = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET   = $(BIN_DIR)/cache-server

.PHONY: all clean dirs test

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

test: all
	@echo "Running tests..."
	python3 tests/test_basic.py

.PHONY: run
run: all
	./$(TARGET)
