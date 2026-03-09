# DeltaQ IDE

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Qt 6](https://img.shields.io/badge/Qt-6-green.svg)](https://www.qt.io/)
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)]()

**A Linux-first modular IDE for C/C++ workflows that combines code editing, graph composition, and SDL2 UI design.**

> **[Русская версия / Russian version](README_RU.md)**

---

## What DeltaQ Is

DeltaQ IDE is built around one transparent workflow:

`module -> graph -> generated C code -> build -> run`

The current project goal is deliberately narrower than a generic "all-in-one IDE" claim. DeltaQ focuses on **modular C/C++ application development on Linux**, where code editing, graph composition, generated code, UI layout, and build/debug all stay inside one inspectable toolchain.

Three product principles define the repository today:

- one module model across code, graphs, UI contracts, and imported packs;
- generated C stays visible and traceable back to the source-of-truth;
- templates and examples are checked-in source trees, not IDE-side code synthesis.

## Current Scope

- **Primary platform:** Linux. The repository already ships Linux CI, Linux release bundles, Linux tarball packaging, and a verified Linux AppImage flow.
- **Strongest built-in scenarios:** console flow, desktop/UI flow, reusable composition, and imported-pack integration.
- **Current boundary:** Windows and macOS are roadmap items, not current release claims.

---

## Features

- **Code Editor** — full-featured C/C++ editor with syntax highlighting, LSP integration (clangd), auto-completion, find & replace, go to definition, and bracket matching
- **Visual Block Editor** — node-based graph editor where you connect modules to build programs visually; graphs compile down to pure C code via topological sorting and IR generation
- **UI Designer** — drag & drop interface builder targeting SDL2; design windows visually, bind events to graph handlers, and generate compilable C code
- **Library Processor** — import existing C/C++ libraries through libclang AST parsing; automatically decompose functions and classes into reusable modules
- **Module System** — everything is a module (`.dqmod`). Modules nest recursively (matryoshka principle): a graph is a module, a module can contain a graph. The repository currently ships 43 checked-in core modules across 7 categories
- **Project Templates** — 6 file-based starter templates are loaded from `resources/templates/` and copied into new projects as ready source trees
- **Built-in Debugger** — GDB/MI integration with breakpoints, stepping, variable inspection, call stack, and visual debugging on the graph canvas
- **Build System** — CMake-based build pipeline with compiler output parsing, error navigation, and one-click build & run

---

## Architecture

DeltaQ IDE is built on a **4-layer architecture** with a central CommandBus:

```
┌─────────────────────────────────────────────────┐
│              Presentation Layer                  │
│  ┌───────────┬──────────────┬──────────────────┐ │
│  │   Code    │    Block     │   UI Designer    │ │
│  │  Editor   │   Editor    │                  │ │
│  └───────────┴──────────────┴──────────────────┘ │
├─────────────────────────────────────────────────┤
│              Application Layer                   │
│    CommandBus · UndoManager · ActionManager      │
│    SessionManager · ModuleRegistry               │
├─────────────────────────────────────────────────┤
│              Core Services Layer                 │
│    GraphCompiler · BuildManager · LSPClient      │
│    SDL2CodeGenerator · LibclangParser            │
├─────────────────────────────────────────────────┤
│              Data Layer                          │
│    ProjectManager · GraphStore · UILayoutStore   │
│    ModuleStore · FileSystem                      │
└─────────────────────────────────────────────────┘
```

All operations go through the **CommandBus**, enabling full undo/redo support across every editor.

---

## Quick Start

The build and release flow below is **Linux-first**. Building the repository on other platforms may be possible, but Windows/macOS are not yet supported as finished delivery targets.

### Dependencies

```bash
# Ubuntu / Debian
sudo apt install \
    cmake g++ \
    qt6-base-dev \
    libqscintilla2-qt6-dev \
    libclang-dev \
    libsdl2-dev \
    libsdl2-ttf-dev \
    gdb clangd

# Fedora
sudo dnf install \
    cmake gcc-c++ \
    qt6-qtbase-devel \
    qscintilla-qt6-devel \
    clang-devel \
    SDL2-devel \
    SDL2_ttf-devel \
    gdb clang-tools-extra
```

**Optional dependencies:**
- `libqscintilla2-qt6-dev` — advanced code editor (falls back to QPlainTextEdit if not found)
- `libclang-dev` — library import/parsing (library processor is disabled without it)
- `libsdl2-dev` and `libsdl2-ttf-dev` — required only for building generated UI projects
- `clangd` — LSP server for code intelligence
- `gdb` — debugger backend

### Build

```bash
git clone https://github.com/Vldimirus/IDE_DeltaQ.git
cd IDE_DeltaQ

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Run

```bash
./build/src/deltaq
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `DQ_BUILD_TESTS` | `ON` | Build unit tests |
| `DQ_USE_LSP` | `ON` | Enable LSP client (requires clangd) |
| `DQ_USE_LIBCLANG` | `ON` | Enable libclang integration |

```bash
# Example: build without tests
cmake .. -DCMAKE_BUILD_TYPE=Release -DDQ_BUILD_TESTS=OFF
```

---

## Usage

A typical workflow in DeltaQ IDE:

1. **Create a project** — File → New Project, choose one of the 6 file-based templates: Console Hello World, Console Counter Until Q, Desktop Empty Window, Desktop UI Graph Example, Desktop Text Editor, or Desktop Multi Window Workspace
2. **Write modules** — create C functions with `@dqmodule` annotations, or use the Module Manager to write and test modules with instant preview
3. **Build a graph** — open the Block Editor, drag modules from the palette, and connect their ports to define program flow
4. **Design UI** *(Desktop projects)* — open the UI Designer, place widgets (buttons, text fields, sliders...), set properties, and bind events to graph handlers
5. **Build & Run** — hit Build (Ctrl+B) to compile the graph into C code, generate CMakeLists.txt, and produce an executable; then Run (Ctrl+R)
6. **Debug** — set breakpoints (F9) and start debugging (F5); the debugger highlights the active node on the graph and shows variable values on ports

## First Run

If you want the shortest reproducible DeltaQ walkthrough, start with:

- `resources/examples/minimal_console_flow/minimal_console_flow.dqproj`

Open `graphs/main.dqgraph`, build the project, run it, type `DeltaQ`, and then inspect the generated `src/main.c`.

That example is intentionally tiny and demonstrates the core path:

`module -> graph -> generated C code -> build -> run`

The detailed walkthrough is in:

- `docs/onboarding/first_run.md`

## Project Templates And Examples

DeltaQ now loads project templates directly from files in `resources/templates/` rather than generating starter source code inside the IDE.

- **Console Hello World** — minimal console app that prints `Hello, world!` and exits
- **Console Counter Until Q** — console loop that prints an incrementing counter until the user presses `q`
- **Desktop Empty Window** — SDL2 desktop app that opens a blank window
- **Desktop UI Graph Example** — desktop project with a ready graph, UI layout, and generated SDL2 runtime files
- **Desktop Text Editor** — simple SDL2 text editor with a top menu bar
- **Desktop Multi Window Workspace** — desktop workspace with child windows inside the main frame

The repository currently includes 4 checked-in example projects in `resources/examples/`:

- `minimal_console_flow` — shortest onboarding path for `module -> graph -> generated C code -> build -> run`
- `desktop_ui_flow` — desktop/UI showcase with generated SDL2 runtime and live event handlers
- `reusable_composition_console` — composite submodules and repeated reuse in one root graph
- `imported_pack_sensor_console` — curated imported pack flow from external library to working graph runtime

---

## Module System

DeltaQ uses a **module-centric architecture**. Every function is a module (`.dqmod`) with typed input/output ports.

- **Standard Library** — 43 checked-in core modules across `control`, `conversion`, `desktop`, `io`, `logic`, `math`, and `string`; installed to `~/.deltaq/modules/`
- **Local Modules** — project-specific modules in `dqmods/`
- **Submodules (Matryoshka)** — select nodes on a graph → "Create Submodule" → the selection becomes a reusable composite module with its own internal graph. Nesting is unlimited
- **UI Modules** — UI widgets (Button, Label, Slider...) appear as modules with property inputs and event outputs
- **Library Import** — import C/C++ headers via libclang → functions/classes are automatically decomposed into modules
- **Library Docs Hub** — `docs/library/README.md` is the entry point for core curation, verification story, and imported pack guides

---

## Tech Stack

| Component | Technology |
|-----------|-----------|
| Language | C++20 |
| GUI Framework | Qt 6 |
| Code Editor | QScintilla (fallback: QPlainTextEdit) |
| Build System | CMake 3.20+ |
| Generated UI | SDL2 |
| C/C++ Parsing | libclang |
| LSP Server | clangd |
| Debugger | GDB (MI protocol) |
| Graph Compiler | Custom IR → C code generation |

---

## Tests

The repository currently includes 48 checked-in test source files covering `core`, `editor`, `uiDesigner`, `blockEditor`, `libProcessor`, `codegen`, `lsp`, and `debug`.

```bash
cd build
ctest --output-on-failure
```

Or run a specific test:

```bash
./build/tests/test_CommandBus
```

The repository also includes a Linux CI baseline in `.github/workflows/ci.yml`: GitHub Actions performs full `configure -> build -> ctest` on Ubuntu and verifies install/package smoke for the self-contained bundle layout, including translation payload and bundled templates/examples.

---

## Roadmap

- [x] Linux CI baseline (GitHub Actions full build, 48 tests, install/package smoke)
- [ ] Cross-platform support (Windows, macOS)
- [x] Linux AppImage packaging
- [ ] Windows installer / macOS DMG packaging
- [ ] Python and Rust language backends (IR → Python/Rust code generation)
- [ ] Plugin system for third-party extensions
- [ ] Performance optimizations for large graphs (100+ nodes)
- [ ] User documentation and tutorials
- [x] Example projects

---

## Contributing

Contributions are welcome! Here's how you can help:

1. **Fork** the repository
2. **Create a branch** for your feature (`git checkout -b feature/my-feature`)
3. **Commit** your changes
4. **Push** to the branch (`git push origin feature/my-feature`)
5. **Open a Pull Request**

Please make sure:
- Code compiles without warnings
- All existing tests pass (`ctest --output-on-failure`)
- New functionality includes tests where appropriate

---

## License

This project is licensed under the **GNU General Public License v3.0** — see the [LICENSE](LICENSE) file for details.

```
Copyright (c) 2024-2026 Vladimir Kononenko
```

---

## Acknowledgments

- [Qt Project](https://www.qt.io/) — GUI framework
- [QScintilla](https://riverbankcomputing.com/software/qscintilla/) — code editor component
- [LLVM/Clang](https://clang.llvm.org/) — C/C++ parsing and LSP
- [SDL2](https://www.libsdl.org/) — target platform for generated UI
- [CMake](https://cmake.org/) — build system
