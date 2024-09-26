CC = gcc
SRC_DIR = src
INC_DIR = include
# This is where the targets will be compiled from
EXAMPLES_DIR = examples
TEST_DIR = test
CFLAGS = -g -I$(INC_DIR) -Wall
LDFLAGS = -lm
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
EXAMPLES_SRC = $(wildcard $(EXAMPLES_DIR)/*.c)
# Each demo source gets a target, e.g. examples/01_demo.c -> 01_demo
EXAMPLES = $(patsubst $(EXAMPLES_DIR)/%.c,%,$(EXAMPLES_SRC))

# If `test` is passed as a cmd argument, extend flags to handle unit tests 
ifeq ($(MAKECMDGOALS), test)
    CFLAGS += -DRUN_UNIT_TESTS
    TARGET = test/test
    SRC = $(wildcard $(SRC_DIR)/*.c) $(TEST_SRC)
else
    TARGET = $(EXAMPLES)
    SRC = $(wildcard $(SRC_DIR)/*.c)
endif

OBJECTS = $(SRC:%.c=%.o)

# What to do by default (no arguments)
all: $(EXAMPLES)

$(EXAMPLES): %: $(EXAMPLES_DIR)/%.c $(wildcard $(SRC_DIR)/*.c)
	$(CC) $(CFLAGS) $(SRC_DIR)/*.c $< -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Phony to always rerun it

test: $(TARGET)
	./$(TARGET)

.PHONY: clean

RM = rm -rf
clean:
	$(RM) $(EXAMPLES) $(SRC_DIR)/*.o test/*.o

