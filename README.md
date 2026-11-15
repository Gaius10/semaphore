# Semaphore Traffic Simulation

A multithreaded C simulation of a traffic intersection controlled by semaphores and traffic lights. This project demonstrates concurrent programming concepts using POSIX threads and semaphores.

## Overview

This application simulates a traffic intersection where cars arrive from two perpendicular roads (horizontal and vertical).
Also there is an evolutionary system model that learns optimal ways to control the traffic lights.

## Project Structure

```
.
├── config.ini                 Simulation settings, read at startup
├── Makefile
├── src/
│   ├── main.c                 Entry point: loads the config and starts the mode it asks for
│   ├── config/                Config file loading (opaque config_t + getters)
│   ├── input/                 Keyboard input: single key presses, without waiting for Enter
│   ├── mode/                  One file per mode: human_player (game + input threads), ai_player (single loop)
│   ├── ai/                    The AI player: ai_decide(ai, game) answers with the next command
│   │                          ai.c picks the agent from the config; each agent has its own file and
│   │                          struct (noop.c, fixed_toggle.c, linear_combination.c) behind the
│   │                          interface in agent.h
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

The tests cover `car`, `road`, `traffic_light`, `config`, `view`, `game`, `input`, `ai` (the dispatch), and the
`noop`, `fixed_toggle` and `linear_combination` agents, and live in `tests/check_*.c`. Each module has its own
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
mode=ai
tick_ms=200

[human_player | ai_player]
car_spawn_probability_h=0.5
car_spawn_probability_v=0.5
road_length_h=10
road_length_v=10

[view]
view_width=21
view_height=21

[ai_player]
; agent: noop | fixed_toggle | linear_combination
agent=fixed_toggle

; fixed_toggle agent parameters
toggle_interval_ticks=10

; linear_combination agent parameters (one coefficient per cell of each road)
binarization_method=threshold
threshold=0.5
coeficients_h=0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5
coeficients_v=0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5
```

| Key                       | Required                                                            | Description                                                                              |
|---------------------------|---------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| `mode`                    | always                                                              | `human`, `ai` or `training`                                                              |
| `car_spawn_probability_h` | when `mode=human` or `ai`                                           | Chance of a car spawning on the horizontal road each tick                                |
| `car_spawn_probability_v` | when `mode=human` or `ai`                                           | Chance of a car spawning on the vertical road each tick                                  |
| `road_length_h`           | when `mode=human` or `ai`                                           | Length of the horizontal road, in cars (a positive whole number)                         |
| `road_length_v`           | when `mode=human` or `ai`                                           | Length of the vertical road, in cars (a positive whole number)                           |
| `view_width`              | when `mode=human` or `ai`                                           | Width of the terminal view, in cells (a positive whole number)                           |
| `view_height`             | when `mode=human` or `ai`                                           | Height of the terminal view, in cells (a positive whole number)                          |
| `tick_ms`                 | when `mode=human` or `ai`                                           | How long a game tick lasts, in milliseconds (a positive whole number)                    |
| `agent`                   | when `mode=ai`                                                      | Which AI plays: `noop`, `fixed_toggle` or `linear_combination` (see [AI mode](#ai-mode)) |
| `toggle_interval_ticks`   | when `agent=fixed_toggle`                                           | Ticks a light stays green before the lights swap (a positive whole number)               |
| `binarization_method`     | no (defaults to `threshold`)                                        | How the linear combination becomes a light decision. Only `threshold` exists             |
| `threshold`               | when `agent=linear_combination` and `binarization_method=threshold` | The value the combination is compared with (a number)                                    |
| `coeficients_h`           | when `agent=linear_combination`                                     | Comma-separated numbers, exactly `road_length_h` of them                                 |
| `coeficients_v`           | when `agent=linear_combination`                                     | Comma-separated numbers, exactly `road_length_v` of them                                 |

There are no defaults: a missing required key is an error, and so is a value that is not valid for
its key (an unknown `mode`, a probability that is not a number, or a length or size that is not a
positive whole number). Unknown keys are ignored and key order does not matter. `human` and `ai` modes are
implemented; `training` is not yet. Both use the same world, view and tick keys.

The coefficient lists are checked once the whole file is read, so key order still does not matter:
`coeficients_h` and `coeficients_v` must have exactly one number per cell of their road (the road
lengths), whitespace around the numbers is fine, and an empty entry (like `1,,2` or a trailing
comma) or a value that is not a finite number is an error. Like any other line, the list must fit in
256 characters. `fixed_toggle` and `noop` ignore these keys.

### Human mode

With `mode=human`, you control the lights with the keyboard. A key press is read without waiting
for Enter, and applies on the next tick. If you press more than one key within a tick, only the last
one counts:

| Key            | Action                                  |
|----------------|-----------------------------------------|
| `h`            | Flip the horizontal road's light        |
| `v`            | Flip the vertical road's light          |
| `q` or Ctrl-C  | Quit                                    |

### AI mode

With `mode=ai`, an AI picks the command of every tick instead of the keyboard. The `agent` key
says which one, and it is required in this mode:

| `agent`              | Behavior                                                                           |
|----------------------|------------------------------------------------------------------------------------|
| `noop`               | Never touches a light. It only exists to exercise the architecture.                |
| `fixed_toggle`       | Swaps which light is green every `toggle_interval_ticks` ticks, ignoring the cars. |
| `linear_combination` | Scores each light from the cars queued at it and opens it above `threshold`.       |

`fixed_toggle` also needs `toggle_interval_ticks` (a positive whole number): how many ticks a light
stays green. A swap closes the green light and opens the other one in the same tick, so the lights
are never both green. If the lights are ever in the same color, it fixes that on its first tick,
leaving the horizontal light green. This always happens at the start of a game, since both lights
start green.

There is no input in this mode, so stop it with Ctrl-C.

To add an agent: write `src/ai/<name>.c/.h` with its own struct (an `ai_t base` as the first member)
and an `agent_ops_t`, add a `config_agent_t` value and its `agent=` name in the config, add a case to
`ai_create` in `ai.c`, add its object to `OBJS`, `LIBS` and `TEST_SRC_OBJS` in the Makefile (with a
build rule), and add a `tests/check_<name>.c` (the shared setup is in `tests/ai_fixture.h`).

`linear_combination` scores each light on its own. For a road, the score is the sum of
`coeficient[k] * exp(-distance)` over the cars that have not passed the light yet, where `k` is the
car's place in the queue (0 is the car closest to the light) and `distance` is the straight-line
distance, in cells, from that car to the light. A car on the light's stop cell weighs 1, one cell
back about 0.37, two cells back about 0.14. Cars that already passed the light are ignored. The light
is green when the score is greater than `threshold`, and red otherwise. The two lights are decided
independently, so both can be green at once and cause crashes. With the example config (`threshold=0.5`,
every coefficient 0.5), one car on the stop cell scores exactly 0.5, which is not enough, so a light
opens once about two cars are queued.

### Coordinates

The game and the view share one coordinate system, like the screen: `(0, 0)` is the top-left cell,
`x` grows to the right and `y` grows downwards. Horizontal cars spawn at `(0, road_length_v)` and
move right, so the horizontal road is the row `y = road_length_v`. The view draws a
`view_width` x `view_height` grid and skips cars that fall outside of it, so make it at least as big
as the roads.

## Traffic Lights

The roads cross at `(road_length_h, road_length_v)`, and each road has its own traffic light. Both
lights start green. A game command (`game_command_t`) is the whole controllable state of the game as
it should be from that tick on: the color of each light. It is a state, not a change, so running it
twice gives the same result. To leave the game alone, send the colors the lights already show; to
change one light, change only its color. The human mode does this for the `h` and `v` keys (flip that
light), and the AIs build their commands the same way (`agent_current_command` in `src/ai/agent.h`
gives them the current state).

Each light has a stop cell, the last cell of its road before the crossing:
`(road_length_h - 1, road_length_v)` for the horizontal road and `(road_length_h, road_length_v - 1)`
for the vertical one. On every tick:

1. The command is applied.
2. Every car moves one cell, except a car on its stop cell while the light is red, and a car whose
   next cell is taken by the car ahead of it. Those wait, so a red light builds a queue. Cars
   already in the crossing or past it never stop.
3. The first car of each road is removed once it is past the end of its road (twice the road length).
4. If both roads have a car in the crossing, the game counts a crash (`game_get_crashes`). The game
   keeps running.
5. New cars spawn, unless the queue reaches back to the spawn cell. A road therefore holds at most
   its length in cars before the crossing.

The game also counts the cars that have passed a light (`game_get_cars_passed`): a car counts once,
when it moves out of its light's stop cell.

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