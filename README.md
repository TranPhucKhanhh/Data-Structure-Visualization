# Data Structure Visualization

An interactive desktop application for visualizing data structures and algorithms through animated playback, highlighted states, and step-by-step execution.

## Team

This project was created for **CS163 - Data Structures and Algorithms** at **HCMUS** by:

- Trần Phúc Khánh - 25125020
- Lê Hồng Đăng - 25125040
- Phạm Gia Bảo - 25125080
- Lương Nhật Minh - 25125088

## Overview

The goal of this project is to make core data structures and graph algorithms easier to understand by showing how each operation changes the internal state over time.

The application is built with:

- **C++17**
- **SFML 3**
- **Dear ImGui**
- **ImGui-SFML**
- **CMake**

It provides a desktop UI where users can switch between modules, input data manually or from files, and watch operations unfold visually.

## Implemented Modules

### Trie

The trie visualizer supports:

- Initialize from a list of words
- Initialize from file
- Insert a word
- Search for a word
- Delete a word
- Update a word

### Heap

The heap visualizer supports:

- Initialize from a list
- Initialize from file
- Generate random input
- Insert a value
- Search for a value
- Delete the top node
- Update a value
- Switch between min-heap and max-heap

### Singly Linked List

The singly linked list visualizer supports:

- Initialize random data
- Initialize from a user-defined list
- Initialize from text file
- Add at index
- Delete at index
- Update at index
- Search by value

### Shortest Path

The shortest path visualizer supports:

- Graph input using weighted edges
- Dijkstra shortest path execution
- Step-by-step playback
- Visual highlighting of nodes, relaxed edges, and final path

## Main Features

- Interactive menu-based desktop interface
- Step-by-step and run-at-once playback modes
- Animated transitions between algorithm states
- Visual highlighting of active and affected nodes
- Zooming and panning on visualization canvases
- Operation, comment, and code panels
- File input support through a simple file dialog
- Custom font loading from the local `fonts/` folder

## Project Structure

```text
data-visualize/
|-- include/
|   |-- logic/      # Core algorithm and data structure declarations
|   |-- ui/         # UI declarations for each module
|   `-- utils/      # Utility headers
|-- src/
|   |-- logic/      # Logic implementations
|   |-- ui/         # Rendering and interaction implementations
|   `-- utils/      # Utility implementations
|-- fonts/          # Font assets used by the UI
|-- .github/
|   `-- workflows/  # CI pipeline
|-- CMakeLists.txt
`-- README.md
```

## Architecture

The project is split into two main layers:

- `logic` contains the underlying data structures and algorithm implementations.
- `ui` contains the ImGui panels and SFML drawing code used to animate and render each module.

Most modules expose operation timelines or instruction sequences so the UI can replay each algorithm step visually instead of only showing the final result.

## Build Instructions

### Requirements

- A C++17-compatible compiler
- [CMake 3.28+](https://cmake.org/download/)
- Git

### Build

From the project root, run:

```bash
cmake -B build
cmake --build build --config Release
```

### Run

After building, run the generated `DataVisualization` executable from the build output directory.

## Dependencies

Project dependencies are fetched automatically through CMake using `FetchContent`.

The main dependencies are:

- [SFML](https://github.com/SFML/SFML)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [ImGui-SFML](https://github.com/SFML/imgui-sfml)

## Continuous Integration

This repository includes a GitHub Actions workflow that builds the project on:

- Windows
- Linux
- macOS (File browser does not support macOS)

The CI pipeline also checks both shared and static configurations.

## License

See [LICENSE.md](LICENSE.md) for license details.
