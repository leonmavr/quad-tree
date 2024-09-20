CC = gcc
TARGET = demo
SRC_DIR = src
INC_DIR = include
EXAMPLES_DIR = examples
CFLAGS = -O3 -I$(INC_DIR) -Wall
LDFLAGS = -lm
TEST_SRC = test/test.c
EXAMPLES_SRC = $(wildcard $(EXAMPLES_DIR)/*.c)
SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(EXAMPLES_SRC)/*.c)

# Each .c demo file in `example` folder gets a target, e.g. examples/01.c -> 01
EXAMPLES = $(patsubst $(EXAMPLES_DIR)/%.c,%,$(EXAMPLES_SRC))

# `make test` command: Overwrite SRC to include tests and library
ifeq ($(MAKECMDGOALS), test)
    CFLAGS += -DRUN_UNIT_TESTS
    SRC = $(wildcard $(SRC_DIR)/*.c) $(TEST_SRC)
    TARGET = test/test
endif

OBJECTS = $(SRC:%.c=%.o)

all: $(TARGET) $(EXAMPLES)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(EXAMPLES): %: $(EXAMPLES_DIR)/%.c $(wildcard $(SRC_DIR)/*.c)
	$(CC) $(CFLAGS) $(SRC_DIR)/*.c $< -o $@ $(LDFLAGS)

# Test target: build and run tests
test: $(OBJECTS)
	$(CC) $(OBJECTS) -o test/test $(LDFLAGS)
	./test/test

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

RM = rm -rf
clean:
	$(RM) $(TARGET) $(SRC_DIR)/*.o $(EXAMPLES) test/test
	$(RM) $(TARGET).o
	$(RM) test/*.o

