# router

> **Fast C++20 Code Search Engine & Reference Placement Core**  
> Pure C++20 with zero third-party dependencies.

---

## 1. Overview

`router` is a fast C++20 systems library providing two main capabilities:

1. **Hybrid Code Search (`router-search`)**:
   - **Fuzzy File Matching**: Fast path matching with word boundary and case detection (similar to FZF).
   - **Full-Text BM25 Ranking**: Keyword content search with proximity scoring and typo tolerance.
   - **Code Symbol Detection**: Extracts function definitions, classes, and types across C/C++, Rust, Python, Go, TypeScript, Lua, and Lean.
   - **C API (`search_engine.h`)**: Ready for direct embedding into Neovim (via LuaJIT `ffi`), Python (`ctypes`), or Rust (`bindgen`).

2. **Reference Placement & Gap Verification Engine (`manifold`)**:
   - Pure C++20 zero-dependency reference placement, discovery, and code-gap filling engine.
   - Multilingual gap detection (C++, Python, Rust, Lua, JavaScript) targeting stubs, empty functions, and `TODO` comments.
   - Verified gap lifecycle state machine (`CANDIDATE` &rarr; `PROPOSED` &rarr; `ADAPTED` &rarr; `VERIFIED` / `REJECTED`) with transition proof gating.
   - 8192-bit binary hypervector indexing, byte-entropy analysis, and compact binary checkpoints.

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
# Run the complete test and verification suite
./bin/manifold selftest

# Benchmark ingestion speed on a codebase
./bin/manifold benchmark [dir_path]
```

### Reference Discovery & Indexing
```bash
# List built-in reference catalog sources
./bin/manifold catalog --list

# Scan a repository to discover recommended reference sources
./bin/manifold discover /path/to/repo

# Index reference sources into the repository manifold database
./bin/manifold index <repo_or_db> --sources <id1,id2,...>
```

### Code Gap Detection & Placement
```bash
# Scan repository for code gaps (stubs, empty bodies, TODOs across C++, Python, Rust, Lua, JS)
./bin/manifold gaps <repo_or_db>

# Find candidate reference placements to fill a specific gap
./bin/manifold fill <repo_or_db> --gap <id> [--space <paths>]

# Place a query or code onto local targets (FILL, MOVE, EXTRACT)
./bin/manifold place "<query>" [--space <paths>] [--kinds <fill,move,extract>] [--json]

# Search indexed reference space
./bin/manifold search "<query>" [--space <paths>]
```

### Gap State Machine & Verification Transitions
```bash
# Transition lifecycle: CANDIDATE -> PROPOSED -> ADAPTED -> VERIFIED / REJECTED
./bin/manifold select <repo_or_db> --gap <id> --reference <idx_or_cell_id>
./bin/manifold apply <repo_or_db> --gap <id> --code "<adapted_code>"
./bin/manifold verify <repo_or_db> --gap <id> --result "<verification_result>"
./bin/manifold reject <repo_or_db> --gap <id> --reason "<reason>"

# Inspect current gap lifecycle status
./bin/manifold status <repo_or_db> [--gap <id>]
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
