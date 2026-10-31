# router

> **Fast C++20 Code Search Engine & Data Classification Core**  
> Pure C++20 with zero third-party dependencies.

---

## 1. Overview

`router` is a fast C++20 systems library providing two main capabilities:

1. **Hybrid Code Search (`router-search`)**:
   - **Fuzzy File Matching**: Fast path matching with word boundary and case detection (similar to FZF).
   - **Full-Text BM25 Ranking**: Keyword content search with proximity scoring and typo tolerance.
   - **Code Symbol Detection**: Extracts function definitions, classes, and types across C/C++, Rust, Python, Go, TypeScript, Lua, and Lean.
   - **C API (`search_engine.h`)**: Ready for direct embedding into Neovim (via LuaJIT `ffi`), Python (`ctypes`), or Rust (`bindgen`).

2. **Data Routing & Classification (`manifold`)**:
   - Zero-dependency byte stream chunking and high-dimensional vector routing.
   - Classifies code and tasks into lifecycle categories (`#io`, `#state`, `#space`, `#op`, `#route`, `#gate`).
   - Compact binary serialization and state history.

---

## 2. Directory Layout

```
router/
├── include/
│   └── router/
│       └── search_engine.h    # C ABI declarations for hybrid search
├── src/
│   ├── manifold.cpp           # Complete standalone Manifold C++20 engine & CLI
│   ├── search_engine.cpp      # BM25 + FZF hybrid repository search implementation
│   ├── search_engine.h        # Internal header
│   └── search_cli.cpp         # CLI driver for hybrid search
├── tests/
│   └── test_search.cpp        # Search engine smoke & verification tests
├── .github/
│   └── workflows/ci.yml       # Automated CI test workflow
├── CMakeLists.txt             # Standard CMake build configuration
├── Makefile                   # Fast standalone Makefile
└── README.md
```

---

## 3. Quick Start & Build

### Prerequisites
- C++20 capable compiler (`g++` 11+, `clang++` 14+, or MSVC 2022)
- `make` or `cmake` (3.20+)

### Fast Build via Makefile
```bash
# Build binaries and shared library
make all

# Run full test suite (Manifold Phase 4 verification + Search unit tests)
make test

# Install binaries to ~/.local/bin and libraries to ~/.local/lib
make install
```

### Build via CMake
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

---

## 4. `manifold` CLI Usage

### Self-Test & Benchmarks
```bash
# Run the complete 7-suite Phase 4 empirical verification suite
./bin/manifold selftest

# Benchmark ingestion speed on a codebase
./bin/manifold benchmark /path/to/repo
```

### Initializing & Teaching Archetypes
```bash
# Initialize a new manifold database
./bin/manifold init index.mfd

# Teach archetype exemplars into the attractor field
./bin/manifold teach index.mfd #op "template <typename T> void matrix_multiply(const T* a, const T* b, T* c)"
./bin/manifold teach index.mfd #gate "assert(bounds_check(idx) && \"Index out of range\");"
```

### Multi-Scope Ingestion
```bash
# Ingest local repository files
./bin/manifold ingest index.mfd --scope repo src/

# Ingest wget / HTML reference documentation
./bin/manifold ingest index.mfd --scope web /path/to/wget_mirror/
```

### Scoped Geometric Query Routing
```bash
# Query across scopes and archetypes
./bin/manifold query index.mfd "@repo #op Fourier transform"
./bin/manifold query index.mfd "@web #space Riemann zeta prime Hopf"
```

### Task Placement & Falsification Hazards
```bash
# Place an implementation task specification onto the optimal file / code block
./bin/manifold place index.mfd "Implement 4th-order Taylor curvature expansion for holonomic parallel transport"
```

---

## 5. `router-search` CLI & Neovim `:Find` Plugin

### Command Line Tool
`router-search` is a fast CLI tool for code, symbol, and file searching with colorized output, file-only mode, and JSON export:

```bash
# Search current directory for a symbol or keyword
router-search "handle_connection"

# Search a specific repository path
router-search -d /path/to/repo "StateEngine"

# Print only matching file paths (useful for xargs, fzf, or scripts)
router-search -l "matrix_multiply"

# Output raw JSON for programmatic tools
router-search --json "solve_puzzle"

# Limit maximum results (default 30)
router-search -n 10 "tokenizer"
```

### Neovim Plugin Integration (`:Find`)

`router` is also a full Neovim plugin providing interactive hybrid search (`FZF + BM25 + AST`):

#### Installation
Using `lazy.nvim`:
```lua
{
    "Syntra-Labs/router",
    opts = {
        keymap = "<leader>f", -- or false to disable default mapping
    },
}
```

Or using `vim-plug`:
```vim
Plug 'Syntra-Labs/router'
```

#### Commands & Usage
- `:Find` opens the interactive floating search bar.
- Press `<leader>f` to trigger `:Find`.
- Type search queries with live preview:
  - Scopes: `@algo <query>`, `@docs <query>`, `@deps <query>`, `@code <query>`
  - Scoped directories: `:Find parse_json in src/ parser/`
- Navigation:
  - `<CR>`: Jump to match and close.
  - `<Tab>` or `p`: Live preview match in the editor split.
  - `<Down>` / `<Up>` / `<C-n>` / `<C-p>`: Select next/previous result.
  - `<Esc>` or `q`: Close search window.

---

## 6. License & Attribution

Part of the **Syntra Labs** systems architecture. Designed for pairing with `synvim`, `taskflow`, `scribe`, and modern terminal workflows.
