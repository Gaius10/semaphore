CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O3 -D_POSIX_C_SOURCE=199309L
LDFLAGS :=
LDLIBS  := -lm -lpthread -lc
TARGET  := main
OBJS    := \
	build/main.o \
	build/game.o \
	build/car.o \
	build/list.o \
	build/input.o \
	build/traffic_light.o \
	build/game_utils.o \
	build/game_runner.o \
	build/ai_commanders.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

build/%.o: src/%.c lib/game.h lib/car.h lib/list.h lib/input.h lib/traffic_light.h lib/game_utils.h lib/ai_commanders.h lib/config.h lib/game_runner.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f $(TARGET).zip

bundle: clean
	zip -r $(TARGET).zip Makefile src lib build

docker:
	docker compose build

docker-run:
	docker compose run main
