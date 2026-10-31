# router

> **High-Throughput Geometric Substrate, Hypervector Lifecycle Router & Hybrid Code Search Engine**  
> Syntra Labs systems core in pure C++20 with zero third-party dependencies.

---

## 1. Overview

`router` is a unified systems engine providing two complementary capabilities for developer environments, language agents, and code reasoning systems:

1. **`manifold` — Geometric Substrate & Lifecycle Router**:
   - Zero-dependency, parser-free byte-entropy and 8192-bit binary hypervector engine.
   - **SpiralCore $\text{Cl}(0,6)$ Clifford Algebra & $E_8$ Root Invariants**: 64-state geometric algebra, 15 bivectors, and $16 \times 16 = 256$ BFS transport.
   - **Order-2 Dirichlet Byte-Markov & Cellular Automata (CA 184/30) Demarcation**: Natural stream chunking without relying on programming language syntax trees.
   - **Online Shannon Surprisal Sketching & 90% Semantic Mass Sparsification**: Retains highest-salience semantic nodes per block, raising SNR by $\sqrt{48/16} \approx 1.73\times$ while reducing accumulator work.
   - **Dynamic Lifecycle Routing & Counterfactual Placement**: Classifies code across 6 universal computational lifecycle archetypes (`#io`, `#state`, `#space`, `#op`, `#route`, `#gate`) and routes implementation tasks with automated counterfactual multi-concern splitting.
   - **Scientific Experiment Ledger & Binary Slab Serialization**: Non-destructive state accumulation, exact algebraic undo, and Merkle root verification.

2. **`router-search` (`search_engine`) — Hybrid Repository Search**:
   - **Smith-Waterman / FZF Affine-Gap Fuzzy Path Matcher**: Word boundary, snake_case/camelCase hump detection, and basename multipliers.
   - **Okapi BM25 Content Scoring**: Corpus inverse document frequency (IDF) weighting, 5-line sliding window multi-term proximity, and typo tolerance.
   - **Structural Symbol Extraction**: Detects definitions and signatures across C/C++, Rust, Lua, Python, Go, TypeScript/JavaScript, and Lean.
   - **C ABI (`search_engine.h`)**: Ready for direct FFI embedding into Neovim (via LuaJIT `ffi`), Python (`ctypes`), or Rust (`bindgen`).

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

## 5. `router-search` CLI & C API Usage

### Command Line
```bash
# Search for files or symbols
./bin/router-search "manifold" . 10
```

### Neovim LuaJIT Integration
```lua
local ffi = require("ffi")
ffi.cdef[[
  const char* synvim_search_repo(const char* root_dir, const char* query_raw, const char* locations_json, int max_results);
  const char* synvim_search_version(void);
]]

local search_lib = ffi.load("librouter_search.so")
local json_results = ffi.string(search_lib.synvim_search_repo(".", "ManifoldEngine", nil, 20))
```

---

## 6. License & Attribution

Part of the **Syntra Labs** systems architecture. Designed for pairing with `synvim`, `r4-prover`, and sovereign developer environments.
