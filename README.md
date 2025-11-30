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
│   └── game_utils.h      # Utility functions
└── src/                   # Source files
    ├── main.c            # Entry point
    ├── game.c            # Game initialization and threading logic
    ├── car.c             # Car movement implementation
    ├── traffic_light.c   # Traffic light toggling
    ├── list.c            # Sequential list implementation
    ├── input.c           # Input handling
    └── game_utils.c      # Utility function implementations
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

Or directly:
```bash
./main
```

### Interactive Controls

While the simulation is running:
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

## Author

Caio Corrêa Chaves (15444406)

## License

This is an educational project created for learning concurrent programming concepts.
