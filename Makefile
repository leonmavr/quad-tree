CC = gcc
SRC_DIR = src
INC_DIR = include
TEST_DIR = test
CFLAGS = -O3 -I$(INC_DIR) -Wall
LDFLAGS = -lm
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)

ifeq ($(MAKECMDGOALS), test)
	# If `test` is passed as a cmd argument, extend flags to handle unit tests 
	TARGET_SRC = $(TEST_SRC)
	TARGETS = $(patsubst $(TEST_DIR)/%.c,%,$(TEST_SRC))
	TARGET_DIR = test
else
	TARGET_SRC = $(wildcard $(TARGET_DIR)/*.c)
	TARGET_DIR = examples
	# Strip file path so each demo source gets a target,
	# e.g. examples/01_demo.c -> 01_demo
	TARGETS = $(patsubst $(TARGET_DIR)/%.c, %, $(TARGET_SRC))
endif

# What to do by default (no arguments)
all: $(TARGETS)
test: all

$(TARGETS): %: $(TARGET_DIR)/%.o $(wildcard $(SRC_DIR)/*.o)
	$(CC) $(CFLAGS) $(SRC_DIR)/*.c $< -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

RM = rm -rf
clean:
	$(RM) $(TARGETS) $(SRC_DIR)/*.o $(TARGET_DIR)/*.o tests
