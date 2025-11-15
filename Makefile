
CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O3
LDFLAGS :=
LDLIBS  := -lm -lpthread
TARGET  := main
OBJS    := build/main.o build/game.o build/car.o build/list.o build/input.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

build/%.o: src/%.c lib/game.h lib/car.h lib/list.h lib/input.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f $(TARGET).zip

bundle: clean
	zip -r $(TARGET).zip Makefile src lib build

