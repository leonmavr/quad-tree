CC = gcc
TARGET = demo 
SRC_DIR = src
INC_DIR = include
CFLAGS = -I$(INC_DIR) -Wall
LDFLAGS = -lm
DEMO_SRC = main.c 
TEST_SRC = test/tests.c
SRC = $(wildcard $(SRC_DIR)/*.c) $(DEMO_SRC)

# Compare cmd arguments; if `test`, extend it to handle unit tests
ifeq ($(MAKECMDGOALS), test)
    CFLAGS += -DRUN_UNIT_TESTS
    SRC = $(wildcard $(SRC_DIR)/*.c) $(TEST_SRC)
    TARGET = test/test
endif

OBJECTS = $(SRC:%.c=%.o)
RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

.PHONY: clean

clean:
	$(RM) $(TARGET)
	$(RM) $(SRC_DIR)/*.o
	$(RM) $(TARGET).o
	$(RM) test/*.o
