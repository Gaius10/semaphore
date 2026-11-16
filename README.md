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
│   ├── mode/                  One file per mode: human_player (game + input threads), ai_player (single loop),
│   │                          training (genetic algorithm, see [Training mode](#training-mode))
│   ├── agents/                The AI player: ai_decide(ai, game) answers with the next command
│   │                          ai.c picks the agent from the config; each agent has its own file and
│   │                          struct (noop.c, fixed_toggle.c, linear_combination.c) behind the
│   │                          interface in agent.h
│   ├── recording/             recorder.c: writes a game, tick by tick, to a JSON Lines file (`record_dir`)
│   ├── training/              What training mode is made of, with no game logic of its own:
│   │                          genetic.c (the generic genetic algorithm), rng.c (its own random numbers),
│   │                          candidate.c (plays one genome in headless games), evaluator.c (plays a whole
│   │                          population in parallel worker processes), results_csv.c (the CSV file) and
│   │                          dashboard.c (the live terminal panel)
│   └── game/
│       ├── game.c / game.h    Game state and the per-tick update
│       ├── view.c / view.h    Terminal view: draws the two roads as a character grid
│       └── entities/          car, road and traffic_light (opaque types, create/destroy style)
├── tests/                     Unit tests (Check), one check_<module>.c per module
├── analysis/                  plot_training.py: plots a training CSV (`make plot`); output in plots/ (git-ignored)
│                              replay.html + make_replay.py: game replays (`make replay`); output in replays/ (git-ignored)
├── recordings/                Game recordings from `record_dir` (git-ignored)
├── data/                      Training CSVs (git-ignored)
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

The tests cover `car`, `road`, `traffic_light`, `config`, `view`, `game`, `input`, `ai` (the dispatch), the
`noop`, `fixed_toggle` and `linear_combination` agents, and the training code (`rng`, `genetic`, `evaluator`
with the candidate wrapper, `training` with the results file, and `dashboard`), and live in `tests/check_*.c`. Each module has its own
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

[training]
population_size=30
generations=40
episodes_per_candidate=3
episode_ticks=500
crash_penalty=5
elite_count=2
tournament_size=3
mutation_rate=0.1
mutation_sigma=0.1
coeficient_max=2
threshold_max=4
seed=1
workers=4
training_output_dir=data
```

| Key                       | Required                                                            | Description                                                                              |
|---------------------------|---------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| `mode`                    | always                                                              | `human`, `ai` or `training`                                                              |
| `car_spawn_probability_h` | when `mode=human`, `ai` or `training`                                           | Chance of a car spawning on the horizontal road each tick                                |
| `car_spawn_probability_v` | when `mode=human`, `ai` or `training`                                           | Chance of a car spawning on the vertical road each tick                                  |
| `road_length_h`           | when `mode=human`, `ai` or `training`                                           | Length of the horizontal road, in cars (a positive whole number)                         |
| `road_length_v`           | when `mode=human`, `ai` or `training`                                           | Length of the vertical road, in cars (a positive whole number)                           |
| `view_width`              | when `mode=human` or `ai`                                           | Width of the terminal view, in cells (a positive whole number)                           |
| `view_height`             | when `mode=human` or `ai`                                           | Height of the terminal view, in cells (a positive whole number)                          |
| `tick_ms`                 | when `mode=human` or `ai`                                           | How long a game tick lasts, in milliseconds (a positive whole number)                    |
| `agent`                   | when `mode=ai`                                                      | Which AI plays: `noop`, `fixed_toggle` or `linear_combination` (see [AI mode](#ai-mode)) |
| `toggle_interval_ticks`   | when `agent=fixed_toggle`                                           | Ticks a light stays green before the lights swap (a positive whole number)               |
| `binarization_method`     | no (defaults to `threshold`)                                        | How the linear combination becomes a light decision. Only `threshold` exists             |
| `threshold`               | when `agent=linear_combination` and `binarization_method=threshold` | How much busier the other road must be before the green switches (a number, 0 or more)   |
| `coeficients_h`           | when `agent=linear_combination`                                     | Comma-separated numbers, exactly `road_length_h` of them                                 |
| `coeficients_v`           | when `agent=linear_combination`                                     | Comma-separated numbers, exactly `road_length_v` of them                                 |
| `population_size`         | when `mode=training`                                                | Candidates per generation (a whole number, at least 2)                                   |
| `generations`             | when `mode=training`                                                | How many generations the run lasts (a positive whole number)                             |
| `episodes_per_candidate`  | when `mode=training`                                                | Games played to score one candidate (a positive whole number)                            |
| `episode_ticks`           | when `mode=training`                                                | Ticks in each of those games (a positive whole number)                                   |
| `crash_penalty`           | when `mode=training`                                                | Fitness of a game = cars passed - `crash_penalty` * crashes (a number, 0 or more)        |
| `elite_count`             | when `mode=training`                                                | Best candidates copied unchanged into the next generation (positive, less than `population_size`) |
| `tournament_size`         | when `mode=training`                                                | Candidates drawn at random for each parent selection (positive, at most `population_size`) |
| `mutation_rate`           | when `mode=training`                                                | Chance that each number of a child mutates (0 to 1)                                      |
| `mutation_sigma`          | when `mode=training`                                                | Size of a mutation as a fraction of the range of the number (a number above 0)           |
| `coeficient_max`          | when `mode=training`                                                | Coefficients are searched in `[0, coeficient_max]` (a number above 0)                    |
| `threshold_max`           | when `mode=training`                                                | The threshold is searched in `[0, threshold_max]` (a number above 0)                     |
| `seed`                    | when `mode=training`                                                | Seed of the run: the same seed gives the same run (a positive whole number)              |
| `workers`                 | when `mode=training`                                                | Worker processes that play candidates at the same time (a positive whole number)         |
| `training_output_dir`     | when `mode=training`                                                | Folder where the results file goes, created if missing (a non-empty path)                |

There are no defaults: a missing required key is an error, and so is a value that is not valid for
its key (an unknown `mode`, a probability that is not a number, or a length or size that is not a
positive whole number). Unknown keys are ignored and key order does not matter. All three modes are
implemented. `human` and `ai` use the same world, view and tick keys. `training` plays the same world but
has no view, so it does not need the view keys or `tick_ms`; the training keys are ignored by the other modes.

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
| `linear_combination` | Scores each road from its queue and gives the green to the busier one.              |

`fixed_toggle` also needs `toggle_interval_ticks` (a positive whole number): how many ticks a light
stays green. A swap closes the green light and opens the other one in the same tick, so the lights
are never both green. If the lights are ever in the same color, it fixes that on its first tick,
leaving the horizontal light green. This always happens at the start of a game, since both lights
start green.

There is no input in this mode, so stop it with Ctrl-C.

To add an agent: write `src/agents/<name>.c/.h` with its own struct (an `ai_t base` as the first member)
and an `agent_ops_t`, add a `config_agent_t` value and its `agent=` name in the config, add a case to
`ai_create` in `ai.c`, add its object to `OBJS`, `LIBS` and `TEST_SRC_OBJS` in the Makefile (with a
build rule), and add a `tests/check_<name>.c` (the shared setup is in `tests/ai_fixture.h`).

`linear_combination` scores each road. For a road, the score is the sum of
`coeficient[k] * exp(-distance)` over the cars that have not passed the light yet, where `k` is the
car's place in the queue (0 is the car closest to the light) and `distance` is the straight-line
distance, in cells, from that car to the light. A car on the light's stop cell weighs 1, one cell
back about 0.37, two cells back about 0.14. Cars that already passed the light are ignored.

The two scores are then compared: `difference = horizontal score - vertical score`.

- `difference > threshold`: horizontal green, vertical red.
- `difference < -threshold`: vertical green, horizontal red.
- Otherwise the lights stay as they are, so they do not flicker when the roads are about equally busy.
  If both lights show the same color (this happens at the start of a game, since both start green),
  the busier road gets the green, and a tie goes to the horizontal one.

So after the first tick exactly one light is green, and the lights never both show green or both
red. A negative `threshold` is treated as 0. With the example config (`threshold=0.5`, every
coefficient 0.5), the green switches when the waiting road's score is 0.5 above the other's, which
takes about two more queued cars on one road than on the other. Cars already in the crossing never
stop, so a switch can still crash with cars from the old green road. The agent does not add an
all-red step.

### Training mode

With `mode=training`, a genetic algorithm looks for the `threshold` and the coefficients of the
`linear_combination` agent that pass the most cars. Nothing is drawn by the game: candidates are scored by
playing real games, in memory, with the same agent that `mode=ai` uses, so a trained genome pasted into
`config.ini` behaves the way it did in training (up to the 10 digits that are printed).

**The search.** A candidate (a genome) is the list `[threshold, coeficients_h..., coeficients_v...]`, 1 plus
`road_length_h` plus `road_length_v` numbers (21 with the example roads). The first generation is random,
the threshold in `[0, threshold_max]` and each coefficient in `[0, coeficient_max]`. To get the next one:

1. The `elite_count` best candidates are copied unchanged.
2. Every other slot gets a child. Two parents are picked by tournament selection (the best of
   `tournament_size` candidates drawn at random), each number comes from either parent with equal
   chance, and then each number mutates with probability `mutation_rate` by a random step of
   `mutation_sigma` times its range. Numbers are kept inside their ranges.

**The fitness** of a candidate is the average over `episodes_per_candidate` games of `episode_ticks` ticks of
`cars passed - crash_penalty * crashes`. Before each game the random numbers of the game are reset
with a seed made from `seed`, the generation and the game number. So every candidate of a generation plays
exactly the same traffic (which makes them comparable), and the traffic changes from one generation to the
next (so the search can't memorize one sequence of cars). The same `seed` always gives the same run, whatever
`workers` is.

**Parallelism.** The candidates of a generation are played by `workers` processes at the same time. They are
processes and not threads on purpose: the game draws its cars with `rand()`, whose state is shared by all the
threads of a process, so threads would each get a different slice of the numbers and the traffic would
change from run to run. A worker process has its own copy of it. The results come back through shared
memory, and a worker that fails or dies stops the run (the other workers are killed and nothing is left
running).

**What you see.** On a terminal at least 70 columns wide and tall enough, training shows a panel that is
redrawn in place: the run summary, the progress of the whole run and of every worker, the fitness of the last
generation with its change from the one before, sparklines of the champion and the mean fitness over the
generations, and the genome of the champion drawn as bars (one block per coefficient, as high as its value
compared with `coeficient_max`). When the output is redirected to a file, or the terminal is too small, it
prints one plain line per generation instead.

Ctrl-C stops the run cleanly: the generation in progress finishes, its row is written to the results file, and
the best genome found so far is printed. Press it twice to quit at once.

At the end, it prints the best genome seen as lines to paste into `[ai_player]` (with `mode=ai` and
`agent=linear_combination`):

```ini
threshold=0.08666263244
coeficients_h=0.7977561936, 1.448589795, 2, ...
coeficients_v=1.182966079, 0.8207992701, 0.9044922325, ...
```

The fitness of different generations is measured on different traffic, so the "best seen" can be a candidate
that was lucky once. To compare a genome fairly, run it against several seeds, for instance in a copy of the
config with a different `seed` and more `episodes_per_candidate`.

**The results file.** Every run writes a new file, `<training_output_dir>/training_<YYYYmmdd_HHMMSS>.csv`
(an existing file is never overwritten), with a header and one row per generation, flushed as soon as the
generation is done. Each row has the champion of that generation in full:

| Column                                       | Meaning                                                                          |
|----------------------------------------------|----------------------------------------------------------------------------------|
| `generation`                                 | Generation number, starting at 0 (the number the traffic seeds are made from)    |
| `champion_fitness`                           | Fitness of the best candidate of the generation                                  |
| `mean_fitness`, `worst_fitness`              | Mean and lowest fitness of the generation                                        |
| `champion_cars_passed`, `champion_crashes`   | Averages per game of the two numbers the champion's fitness is made of           |
| `threshold`                                  | The champion's threshold                                                         |
| `h0` ... `h<road_length_h - 1>`              | The champion's coefficients for the horizontal road                              |
| `v0` ... `v<road_length_v - 1>`              | The champion's coefficients for the vertical road                                |

Numbers have 17 significant digits, so they read back exactly: playing a row's genome again in the same
generation (same `seed` and `generation`) gives the same fitness. To plot a run (needs Python 3 with
pandas and matplotlib):

```bash
make plot                                  # latest data/training_*.csv
make plot CSV=data/training_20261116_011205.csv
```

This runs `analysis/plot_training.py`, which saves `analysis/plots/<run name>.png`: the champion, mean
and worst fitness per generation. Run the script directly with `--show` to also open a window.

**What to expect.** With busy roads (`car_spawn_probability` 0.5 on both) the crossing is saturated: only one
road can pass cars at a time, so the cars passed in a game can't be much more than its number of ticks, and
even a simple setting (threshold 0, every coefficient 1) gets close to that. Training mostly learns to use a
threshold near 0 and to switch toward the busier road, and gains a few percent over hand-picked values.
The linear combination agent also never crashes (it always shows exactly one green light, and a car that has
passed its light is out of the crossing on the next tick), so `crash_penalty` has no effect on it today. A
fitness that also charged for waiting time or queue length would give training more to optimize.

### Recording and replay

Add `record_dir` to the config to record every game played in `human` or `ai` mode. It is the one
optional key, and it can go in any section:

```ini
[game]
record_dir=recordings
```

Each game is saved to `<record_dir>/game_YYYYMMDD_HHMMSS.jsonl` (the folder is created if it is missing,
and a recording is never overwritten). The file is written as the game runs, one line per tick, so stop
the game as usual (`q` in human mode, Ctrl-C in ai mode) and the recording keeps every tick up to there.
The path is printed when the game stops.

To watch a recording:

```bash
make replay                                   # latest recordings/game_*.jsonl
make replay REC=recordings/game_20261116_201412.jsonl
```

This writes `analysis/replays/<recording name>.html`, a self-contained page (no server, no internet) that
plays the game back: cars glide between cells, brake lights show which cars are waiting, and crashes and
cars passing the crossing are marked. Below it, a timeline shows which light was green and the length of
each queue over the whole game; click or drag it to jump to a tick. Keys: `Space` play/pause, arrows step
one tick (with `Shift`, ten), `+`/`-` change the speed, `Home`/`End` jump to the start/end. Add `#t=120`
to the URL to open it paused at tick 120. `analysis/replay.html` on its own works too: open it and drop a
recording on it.

The format is JSON Lines: a header with the world (road lengths, spawn probabilities, mode, agent, tick),
then one line per tick (tick 0 is the game before its first tick) with both lights, the cars of each
road as `[id, position along the road]` and the totals so far. `src/recording/recorder.h` describes it in
full. Every car gets an id when it spawns (1, 2, 3, ... across both roads), which is how the replay
follows each car from tick to tick.

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
light), and the AIs build their commands the same way (`agent_current_command` in `src/agents/agent.h`
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