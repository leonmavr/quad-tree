CC = gcc
SRC_DIR = src
INC_DIR = include
TEST_DIR = test
DEMO_DIR = examples
CFLAGS = -I$(INC_DIR) -Wall -O3
LDFLAGS = -lm
TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
# select plotter; PLOTTER=PPM to plot as ppm frames
ifeq ($(PLOTTER), PPM)
    CFLAGS += -DUSE_PPM
endif

ifneq (,$(filter test,$(MAKECMDGOALS)))
	# If `test` is passed as a cmd argument, extend flags to handle unit tests 
	CFLAGS += -DNODE_CAPACITY=2
	TARGET_SRC = $(TEST_SRC)
	TARGETS = $(patsubst $(TEST_DIR)/%.c, %, $(TEST_SRC))
	TARGET_DIR = $(TEST_DIR)
else
	TARGET_SRC = $(wildcard $(TARGET_DIR)/*.c)
	TARGET_DIR = $(DEMO_DIR)
	# Strip file path so each demo source gets a target,
	# e.g. examples/01_demo.c -> 01_demo
	TARGETS = $(patsubst $(TARGET_DIR)/%.c, %, $(TARGET_SRC))
endif

# What to do by default (no arguments)
all: $(TARGETS)
test: all

$(TARGETS): %: $(TARGET_DIR)/%.o $(wildcard $(SRC_DIR)/*.c)
	$(CC) $(CFLAGS) $(SRC_DIR)/*.c $< -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
RM = rm -rf
clean:
	$(RM) $(TARGETS) $(SRC_DIR)/*.o $(TARGET_DIR)/*.o $(basename $(notdir $(TEST_SRC)))
