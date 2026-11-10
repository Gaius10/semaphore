# Semaphore Traffic Simulation

A multithreaded C simulation of a traffic intersection controlled by semaphores and traffic lights. This project demonstrates concurrent programming concepts using POSIX threads and semaphores.

## Overview

This application simulates a traffic intersection where cars arrive from two perpendicular roads (horizontal and vertical).
Also there is a evolutionary system model that learns optimal ways to control the traffic lights.

## Project Structure

```
.
├── config.ini                 Simulation settings, read at startup
├── Makefile
├── src/
│   ├── main.c                 Entry point: loads the config and starts a game
│   ├── config/                Config file loading (opaque config_t + getters)
│   └── game/
│       ├── game.c / game.h    Game state and the per-tick update
│       ├── view.c / view.h    Terminal view: draws the two roads as a character grid
│       └── entities/          car, road and traffic_light (opaque types, create/destroy style)
├── tests/                     Unit tests (Check), one check_<module>.c per module
└── build/                     Object files and the test runner (git-ignored)
```

## Building

### Prerequisites
- GCC compiler
- POSIX-compatible system (Linux, macOS, etc.)
- pthread library
- math library
- [Check](https://libcheck.github.io/check/) and `pkg-config` (only to run the unit tests)

### Build Commands

Build the executable:
```bash
make
```

Clean build artifacts:
```bash
make clean
```

## Testing

Run the unit tests (from the repository root):
```bash
make test
```

The tests cover `car`, `road`, `traffic_light`, `config`, `view` and `game`, and live in `tests/check_*.c`. Each module has its own
suite, registered in `tests/check_main.c`. To add one, write `tests/check_<module>.c` with a
`<module>_suite()` function, declare it in `tests/suites.h`, add it to `check_main.c`, and add the
object to `TEST_OBJS` (and the module's object to `TEST_SRC_OBJS`) in the Makefile.

Check runs each test in its own process. To run them in one process, for example under a debugger
or `valgrind`, set `CK_FORK=no`:
```bash
CK_FORK=no ./build/test_runner
```

## Running the Simulation

```bash
make run
```

Or directly:
```bash
./semaphore
```

The simulation reads `config.ini` from the current directory, so run it from the repository root.
There are no command-line options yet.

## Configuration

Settings live in an ini-style file, `config.ini`. Lines starting with `#` or `;` are comments, and
`[section]` headers are ignored: only the `key=value` lines count, whatever section they are in.

```ini
[game]
mode=human

[human_player]
car_spawn_probability_h=0.5
car_spawn_probability_v=0.5
road_length_h=10
road_length_v=10

[view]
view_width=21
view_height=21
```

| Key                       | Required          | Description                                                              |
|---------------------------|-------------------|--------------------------------------------------------------------------|
| `mode`                    | always            | `human`, `ai` or `training`                                              |
| `car_spawn_probability_h` | when `mode=human` | Chance of a car spawning on the horizontal road each tick                |
| `car_spawn_probability_v` | when `mode=human` | Chance of a car spawning on the vertical road each tick                  |
| `road_length_h`           | when `mode=human` | Length of the horizontal road, in cars (a positive whole number)         |
| `road_length_v`           | when `mode=human` | Length of the vertical road, in cars (a positive whole number)           |
| `view_width`              | when `mode=human` | Width of the terminal view, in cells (a positive whole number)           |
| `view_height`             | when `mode=human` | Height of the terminal view, in cells (a positive whole number)          |

There are no defaults: a missing required key is an error, and so is a value that is not valid for
its key (an unknown `mode`, a probability that is not a number, or a length or size that is not a
positive whole number). Unknown keys are ignored and key order does not matter. Only `human` mode is
implemented so far.

### Coordinates

The game and the view share one coordinate system, like the screen: `(0, 0)` is the top-left cell,
`x` grows to the right and `y` grows downwards. Horizontal cars spawn at `(0, road_length_v)` and
move right, so the horizontal road is the row `y = road_length_v`. The view draws a
`view_width` x `view_height` grid and skips cars that fall outside of it, so make it at least as big
as the roads.

## Traffic Lights

The roads cross at `(road_length_h, road_length_v)`, and each road has its own traffic light. The
horizontal light starts green and the vertical one starts red. The game commands
`GAME_COMMAND_HORIZONTAL_TOGGLE` and `GAME_COMMAND_VERTICAL_TOGGLE` switch a light between green and red.
For now the simulation only sends `GAME_COMMAND_NOOP`: there is no keyboard control yet.

Each light has a stop cell, the last cell of its road before the crossing:
`(road_length_h - 1, road_length_v)` for the horizontal road and `(road_length_h, road_length_v - 1)`
for the vertical one. On every tick:

1. The command is applied.
2. Every car moves one cell, except a car on its stop cell while the light is red, and a car whose
   next cell is taken by the car ahead of it. Those wait, so a red light builds a queue. Cars
   already in the crossing or past it never stop.
3. If both roads have a car in the crossing, the game counts a crash (`game_get_crashes`). The game
   keeps running.
4. New cars spawn, unless the queue reaches back to the spawn cell. A road therefore holds at most
   its length in cars before the crossing.

Both lights can be green at the same time, so it is up to the player to avoid crashes.

### View symbols

| Symbol | Meaning                                   |
|--------|-------------------------------------------|
| `>`    | Car on the horizontal road (moves right)  |
| `v`    | Car on the vertical road (moves down)     |
| `X`    | Cars of both roads in the same cell       |
| `G`    | Green light                               |
| `R`    | Red light                                 |
| `\|`   | Right edge of the grid                    |

Each light is drawn next to its stop cell, on the drivers' right: below the horizontal road's stop
cell and left of the vertical road's stop cell.

## Author

Caio Corrêa Chaves (15444406)

## License

This is an educational project created for learning concurrent programming concepts.