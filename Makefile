
CC      := gcc
CFLAGS  := -std=c2x -Wall -Wextra -Wpedantic -O3
LDFLAGS :=
LDLIBS  := -lm -lpthread -lc
TARGET  := semaphore

OBJS := \
	build/main.o

LIBS :=

.PHONY: all clean run

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

# Builds main target
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

# Objects build
build/main.o: src/main.c $(LIBS)
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

# @todo
