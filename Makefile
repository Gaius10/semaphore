
CC      := gcc
# CFLAGS  := -std=c2x -Wall -Wextra -Wpedantic -O3
CFLAGS  := -std=c2x
LDFLAGS :=
LDLIBS  := -lm -lpthread -lc
TARGET  := semaphore

OBJS := \
	build/main.o \
	build/config.o \
	build/game.o \
	build/view.o \
	build/road.o \
	build/car.o \
	build/traffic_light.o

LIBS := \
	src/config/config.h \
	src/game/game.h \
	src/game/view.h \
	src/game/entities/road.h \
	src/game/entities/car.h \
	src/game/entities/traffic_light.h

.PHONY: all clean run test

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_TARGET)

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

build/view.o: src/game/view.c $(LIBS)
	$(CC) $(CFLAGS) -c src/game/view.c -o build/view.o

build/road.o: src/game/entities/road.c $(LIBS)
	$(CC) $(CFLAGS) -c src/game/entities/road.c -o build/road.o

build/car.o: src/game/entities/car.c $(LIBS)
	$(CC) $(CFLAGS) -c src/game/entities/car.c -o build/car.o

build/traffic_light.o: src/game/entities/traffic_light.c $(LIBS)
	$(CC) $(CFLAGS) -c src/game/entities/traffic_light.c -o build/traffic_light.o

# Unit tests (Check). Run with `make test` from the repo root.
CHECK_CFLAGS := $(shell pkg-config --cflags check)
CHECK_LIBS   := $(shell pkg-config --libs check)

TEST_TARGET := build/test_runner

# Only the units under test: no main.o (it has its own main()).
TEST_SRC_OBJS := \
	build/config.o \
	build/game.o \
	build/view.o \
	build/road.o \
	build/car.o \
	build/traffic_light.o

TEST_OBJS := \
	build/tests/check_main.o \
	build/tests/check_car.o \
	build/tests/check_road.o \
	build/tests/check_config.o \
	build/tests/check_view.o \
	build/tests/check_game.o \
	build/tests/check_traffic_light.o

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(TEST_SRC_OBJS)
	$(CC) $(LDFLAGS) $(TEST_OBJS) $(TEST_SRC_OBJS) -o $@ $(CHECK_LIBS)

build/tests/%.o: tests/%.c tests/suites.h $(LIBS)
	@mkdir -p build/tests
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) -c $< -o $@

