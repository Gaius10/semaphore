# Semaphore Traffic Simulation

A multithreaded C simulation of a traffic intersection controlled by semaphores and traffic lights. This project demonstrates concurrent programming concepts using POSIX threads and semaphores.

## Overview

This application simulates a traffic intersection where cars arrive from two perpendicular roads (horizontal and vertical). The system uses semaphores to manage resource access and a traffic light system to control which direction of traffic can proceed at any given time.

## Project Structure

```
semaphore/
├── Dockerfile              # Docker configuration for containerized builds
├── compose.yaml            # Docker Compose configuration
├── Makefile               # Build configuration
├── main                   # Compiled executable
├── build/                 # Compiled object files
│   ├── main.o
│   ├── game.o
│   ├── car.o
│   ├── list.o
│   ├── input.o
│   ├── traffic_light.o
│   └── game_utils.o
├── lib/                   # Header files
│   ├── game.h            # Core game logic and threading
│   ├── car.h             # Car data structure and movement
│   ├── traffic_light.h   # Traffic light state management
│   ├── list.h            # Sequential list for car queues
│   ├── input.h           # User input handling
│   ├── game_utils.h      # Utility functions
│   └── ai.h              # AI commander implementations
└── src/                   # Source files
    ├── main.c            # Entry point
    ├── game.c            # Game initialization and threading logic
    ├── car.c             # Car movement implementation
    ├── traffic_light.c   # Traffic light toggling
    ├── list.c            # Sequential list implementation
    ├── input.c           # Input handling
    ├── game_utils.c      # Utility function implementations
    └── ai.c              # AI commander logic
```

## Key Features

- **Multi-threaded Simulation**: Uses 5 concurrent threads for different tasks:
  - `car_factory`: Spawns new cars at random intervals
  - `car_mover`: Moves cars through the intersection
  - `world_renderer`: Displays the current state of the simulation
  - `commander`: Handles user input to control traffic lights
  - `game_state_manager`: Manages overall game state and scoring

- **Semaphore Synchronization**: Uses semaphores to protect shared road resources
- **Traffic Light Control**: Interactive traffic light control (horizontal and vertical)
- **Collision Management**: Prevents cars from colliding through proper synchronization
- **Score Tracking**: Counts successfully passed cars through the intersection

## Building

### Prerequisites
- GCC compiler
- POSIX-compatible system (Linux, macOS, etc.)
- pthread library
- math library

### Build Commands

Build the executable:
```bash
make
```

Clean build artifacts:
```bash
make clean
```

### Docker Build

Build with Docker:
```bash
make docker
```

Run with Docker Compose:
```bash
make docker-run
```

## Running the Simulation

### Local Execution

```bash
make run
```

Or directly with command-line flags:
```bash
./main [OPTIONS]
```

### Command-Line Options

```
-h                           Show help message
--mode <mode>                Set execution mode:
  - default                     Interactive mode with visual rendering
  - performance_stats           Headless mode for performance analysis
--commander <type>           Set traffic light controller:
  - player                      Manual control with keyboard
  - fixed_toggle                AI controller with fixed toggle intervals
--number_of_games <n>        Number of games to simulate (default: 1)
--games <n>                  Alias for --number_of_games
-n <number>                  Short alias for --number_of_games
```

### Example Usage

Interactive mode with keyboard control:
```bash
./main --mode default --commander player
```

Automated simulation with AI controller:
```bash
./main --mode default --commander fixed_toggle
```

Headless performance testing:
```bash
./main --mode performance_statistics --commander fixed_toggle --games 5
```

With debug output:
```bash
./main --debug --mode default --commander player
```

### Interactive Controls (player commander mode)

While the simulation is running with the player commander:
- **H**: Toggle horizontal (left-right) traffic light
- **V**: Toggle vertical (up-down) traffic light

The traffic light states cycle between RED and GREEN.

## Technical Details

### Data Structures

- **game_t**: Main game state containing:
  - Two sequential list queues for each road direction
  - Semaphores for memory/resource management on each road
  - Traffic light state
  - Game status and score
  - Cycle counter
  - Options structure for configuration

- **options**: Configuration structure containing:
  - `mode`: Execution mode (DEFAULT or PERFORMANCE_STATS)
  - `commander`: Traffic light controller type (PLAYER or FIXED_TOGGLE)
  - `number_of_games`: Number of games to simulate
  - `debug`: Debug output flag

- **car_t**: Individual car with position coordinates (x, y)

- **traffic_light_t**: Traffic light with position and state for both directions
  - TL_RED (0): Blocks traffic
  - TL_GREEN (1): Allows traffic

### Compilation Flags

```
CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O3
LDFLAGS :=
LDLIBS  := -lm -lpthread
```

The project uses C11 standard with strict compiler warnings and optimization level 3.

## Execution Modes

### Default Mode

The `default` mode provides interactive visual rendering of the traffic simulation:
- Real-time display of car positions and traffic light states
- Game statistics (cycle count, score)
- Suitable for manual testing and observation
- Renderer thread runs concurrently with game logic

### Performance Statistics Mode

The `performance_statistics` mode runs the simulation without visual rendering:
- Headless execution for performance benchmarking
- Faster execution since no rendering overhead
- Useful for testing multiple game cycles rapidly
- Outputs final statistics after completion

## Commander Types

### Player Commander

Manual control of traffic lights via keyboard input:
- Responds to user input in real-time
- Allows testing manual traffic management strategies
- Requires interactive mode to observe results

### Fixed Toggle Commander

Automated AI controller implementing a simple traffic management strategy:

- **Behavior**: Toggles both horizontal and vertical traffic lights every 5 game cycles
- **Interval**: 0.5 seconds between toggle operations
- **Purpose**: Provides fair and alternating access to each road direction
- **Implementation**: Runs in the commander thread, checking `game->cycles_passed` to trigger toggles

This mode is useful for:
- Automated testing and demonstrations
- Baseline comparison for more sophisticated AI algorithms
- Understanding traffic light coordination without manual intervention

### Commander Factory Pattern

The `commander_factory()` function in `main.c` implements a factory pattern to select the appropriate commander function based on the options:

```c
void*(*commander_factory(enum commander commander_type))(void*);
```

This design allows for easy addition of new commander types without modifying the main thread creation logic.

## POSIX Semaphores Implementation

This project uses POSIX semaphores for thread synchronization and mutual exclusion. Here's how they are implemented:

### Semaphore Initialization

In `game_init()` (game.c), two semaphores are created for managing access to each road:

```c
sem_init(&game->road1_memmory, 0, 1);
sem_init(&game->road2_memmory, 0, 1);
```

- **First parameter**: Pointer to the semaphore variable
- **Second parameter**: `0` indicates this is a process-local semaphore (not shared between processes)
- **Third parameter**: Initial value of `1` creates a binary semaphore (mutex), allowing one thread to hold the resource at a time

### Resource Protection

The semaphores act as binary mutexes (mutex) with initial value of 1, ensuring only one car can access each road at a time:

- **`sem_wait()`**: Decrements the semaphore. If the value is 0, the thread blocks until the semaphore is incremented by another thread
- **`sem_post()`**: Increments the semaphore, waking up any blocked threads

### Usage Pattern

When multiple `car_mover` threads attempt to access a road:

1. Thread calls `sem_wait()` on road semaphore
2. If semaphore > 0, value decrements and thread proceeds (acquires resource)
3. If semaphore = 0, thread blocks until another thread calls `sem_post()`
4. After accessing the road, thread calls `sem_post()` to release the resource
5. Next blocked thread wakes up and proceeds

### Maximum Capacity

The `MAX_CARS_WAITING` constant (defined as 10 in game.h) limits cars waiting on each road queue. Combined with the binary semaphores, this ensures:

- No more than one car actively moves through each road segment
- Cars queue in linked lists when the road is occupied
- Fair scheduling of car movement across threads

### Thread Safety

This implementation guarantees:
- **Mutual exclusion**: Only one thread can modify a road's linked list at a time
- **No race conditions**: Car position updates are atomic with respect to list modifications
- **Prevention of deadlocks**: Simple binary semaphore with no circular waits
- **No busy-waiting**: Threads sleep when semaphore is 0, allowing CPU time for other threads

## Author

Caio Corrêa Chaves (15444406)
Gustavo Henrique Nogueira de Andrade Filho (16871388)

## License

This is an educational project created for learning concurrent programming concepts.
