CC = gcc
SRC_DIR = src
INC_DIR = include
# This is where the targets will be compiled from
TARGET_DIR = examples
TEST_DIR = test
CFLAGS = -g -I$(INC_DIR) -Wall
LDFLAGS = -lm
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
TARGET_SRC = $(wildcard $(TARGET_DIR)/*.c)

# If `test` is passed as a cmd argument, extend flags to handle unit tests 
ifeq ($(MAKECMDGOALS), test)
    SRC = $(wildcard $(SRC_DIR)/*.c)
    TARGETS = $(patsubst $(TEST_DIR)/%.c,%,$(TEST_SRC))
    TARGET_DIR = test
	OBJECTS = $(SRC:%.c=%.o) $(TEST_SRC)
else
    # Strip file path so each demo source gets a target,
    # e.g. examples/01_demo.c -> 01_demo
    TARGETS = $(patsubst $(TARGET_DIR)/%.c, %, $(TARGET_SRC))
    SRC = $(wildcard $(SRC_DIR)/*.c)
	OBJECTS = $(SRC:%.c=%.o)
endif


# What to do by default (no arguments)
all: $(TARGETS)

$(TARGETS): %: $(TARGET_DIR)/%.c $(wildcard $(SRC_DIR)/*.c)
	$(CC) $(CFLAGS) $(SRC_DIR)/*.c $< -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: all
.PHONY: clean

RM = rm -rf
clean:
	$(RM) $(TARGETS) $(SRC_DIR)/*.o test/*.o tests

