CC = gcc
SRC_DIR = src
INC_DIR = include
# This is where the targets will be compiled from
TARGET_DIR = examples
TEST_DIR = test
CFLAGS = -g -I$(INC_DIR) -Wall
LDFLAGS = -lm
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)

ifeq ($(MAKECMDGOALS), test)
	# If `test` is passed as a cmd argument, extend flags to handle unit tests 
	TARGET_SRC = $(wildcard $(TEST_DIR)/*.c)
	TARGETS = $(patsubst $(TEST_DIR)/%.c,%,$(TEST_SRC))
	TARGET_DIR = test
else
	# Strip file path so each demo source gets a target,
	# e.g. examples/01_demo.c -> 01_demo
	TARGET_SRC = $(wildcard $(TARGET_DIR)/*.c)
	TARGETS = $(patsubst $(TARGET_DIR)/%.c, %, $(TARGET_SRC))
	TARGET_DIR = examples
endif

# What to do by default (no arguments)
all: $(TARGETS)
test: all

$(TARGETS): %: $(TARGET_DIR)/%.c $(wildcard $(SRC_DIR)/*.c)
	$(CC) $(CFLAGS) $(SRC_DIR)/*.c $< -o $@ $(LDFLAGS)

.PHONY: clean

RM = rm -rf
clean:
	$(RM) $(TARGETS) $(SRC_DIR)/*.o $(TARGET_DIR)/*.o tests

