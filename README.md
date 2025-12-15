# Semaphore Traffic Simulation

A multithreaded C simulation of a traffic intersection controlled by semaphores and traffic lights. This project demonstrates concurrent programming concepts using POSIX threads and semaphores.

## Overview

This application simulates a traffic intersection where cars arrive from two perpendicular roads (horizontal and vertical).
Also there is a evolutionary system model that learns optimal ways to control the traffic lights.

## Project Structure

// @todo

## Building

// @todo

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

## Running the Simulation

### Local Execution

```bash
make run
```

Or directly with command-line flags:
```bash
./semaphore [OPTIONS]
```

### Command-Line Options

```
-h                           Show help message
--config <filepath.cfg>      Specifies a configuration file
```

## Author

Caio Corrêa Chaves (15444406)

## License

This is an educational project created for learning concurrent programming concepts.