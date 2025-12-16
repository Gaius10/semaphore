
CC      := gcc
CFLAGS  := -std=c2x -Wall -Wextra -Wpedantic -O3
LDFLAGS :=
LDLIBS  := -lm -lpthread -lc
TARGET  := semaphore

OBJS := \
	build/main.o \
	build/config.o \
	build/game.o

LIBS := \
	src/config/config.h \
	src/game/game.h

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

build/config.o: src/config/config.c $(LIBS)
	$(CC) $(CFLAGS) -c src/config/config.c -o build/config.o

build/game.o: src/game/game.c $(LIBS)
	$(CC) $(CFLAGS) -c src/game/game.c -o build/game.o

# @todo
