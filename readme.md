# MISLIB — B+ Tree Book Repository

A high-performance, file-backed book catalog built on a hand-rolled B+ Tree index. The system persists records to a flat text file, serializes its tree index to binary for instant cold-start loading, and exposes both a CLI and a benchmark runner.

---

## Architecture Overview

```
MiserableTree/
├── benchmark/
│   └── benchmark.cpp          # Scale performance harness (10K / 100K / 1M)
├── data/
│   └── books_dataset.txt      # Flat CSV-style data file
├── src/
│   ├── entity/
│   │   └── BookEntity.h       # Book POD struct with ostream operator
│   ├── index/
│   │   ├── BPlusTree.cpp/.h   # Core B+ Tree (insert, search, remove, split, merge)
│   │   ├── IndexManager.cpp/.h# Load-or-build orchestrator for the primary index
│   │   ├── TreeSerializer.cpp/.h # Binary serialization / deserialization of the tree
│   ├── persistence/
│   │   ├── BookRepository.cpp/.h # CRUD layer — ties index to flat file
│   │   └── CrudRepository.h   # Abstract repository interface
│   ├── ui/
│   │   └── UserInterface.cpp/.h  # Interactive CLI menu
│   └── util/
│       ├── BookParser.h       # Zero-copy CSV line parser
│       └── BPlusTreeUtils.h   # Binary search helper + DJB2 hash
├── tests/                     # Correctness test suite
├── main.cpp                   # Entry point — boots UI
└── CMakeLists.txt
```

### Index layout

| Index file | Tree | Key | Value |
|---|---|---|---|
| `Index.dat` | Primary B+ Tree | `book.id` | byte offset in data file |
| `AuthorIndex.dat` | Secondary B+ Tree | `DJB2(author)` | `book.id` |
| `GenreIndex.dat` | Secondary B+ Tree | `DJB2(genre)` | `book.id` |

All three trees are serialized to binary on shutdown and loaded back on the next boot, bypassing the O(N log N) rebuild cost.

---

## How It Works

### Insert (`create`)
1. Auto-increments `lastId` and serializes the book as a CSV line appended to the data file.
2. Inserts `(id → offset)` into the primary B+ Tree.
3. Inserts hashed author and genre keys into the two secondary trees.

### Lookup (`get`)
1. Binary-searches the primary B+ Tree for the target ID in O(log N).
2. Seeks directly to the recorded byte offset in the data file.
3. Parses and returns the single line — no full scan required.

### Update
1. Soft-deletes the old record by writing a `DELETED_MARKER` byte at its offset.
2. Appends the new version at end-of-file to avoid length-mismatch overwrites.
3. Removes the old offset from the tree and inserts the new one.

### Delete (`remove`)
1. Writes a `DELETED_MARKER` at the record's file offset (O(1) patch).
2. Removes the key from the primary B+ Tree, triggering borrow-or-merge rebalancing.

### Secondary search (`getByAuthor` / `getByGenre`)
1. Hashes the query string with DJB2.
2. Calls `searchAll()` on the secondary tree — follows the leaf linked list to collect all matching IDs (handles hash collisions with an exact-string post-filter).
3. Resolves each ID through the primary tree.

---

## Build

### Requirements
- CMake ≥ 3.15
- A C++17-capable compiler (GCC 9+, Clang 10+, MSVC 2019+)

### Steps

```bash
cmake -S . -B build
cmake --build build
```

This produces three executables inside `build/`:

| Binary | Purpose |
|---|---|
| `program` | Interactive CLI |
| `run_tests` | Correctness test suite |
| `scale_benchmark` | Build-time and lookup latency benchmark |

The `data/` folder is automatically copied to the build directory by CMake.

### Windows note
The benchmark links `psapi` for physical RAM measurement. CMake handles this automatically on WIN32 builds — no manual linker flags needed.

---

## Running

### Interactive CLI

```bash
./build/program
```

```
--- MISLIB SYSTEM DASHBOARD (Last ID: 42) ---
1. Search Book by ID
2. Register New Book (Auto-ID)
3. Delete Book Record
4. List All Records (Sorted)
5. Search Books by Author
6. Search Books by Genre
0. Shutdown System
```

### Benchmark

```bash
./build/scale_benchmark
```

Output example:

```
====================================================
     MISLIB INDEX ACCESS BENCHMARK 
====================================================

[RUNNING] 1M — data/books_dataset.txt
  Build time : 121 ms
  Net memory : 53.4883 MB
  Avg lookup : 0.41 µs
  Hit rate   : 1000 / 1000
```
---

## Data File Format

Each record is stored as a single line:

```
<id>,<title>,<author>,<genre>,<year>
```

Example:

```
1,The Great Gatsby,F. Scott Fitzgerald,Fiction,1925
2,Dune,Frank Herbert,Science Fiction,1965
```

Soft-deleted records have their first byte replaced with a marker character and are skipped by the parser and index builder.

---

## Design Decisions

**Append-only writes with soft deletion** — in-place overwrites are unsafe because updated records can differ in byte length. Appending guarantees the byte offset stored in the index always points to a valid record, while the marker byte prevents re-indexing on cold start.

**Binary index serialization** — rebuilding a B+ Tree over a 1M-record file takes ~2 seconds. Serializing the tree to `Index.dat` on shutdown reduces the next boot to a direct `fread`, typically under 50 ms.

**DJB2 hash for secondary keys** — the secondary trees key on the hash of the author/genre string rather than the raw string, keeping the node type uniform (`size_t`). Hash collisions are resolved by an exact-string post-filter in `getByAuthor` / `getByGenre`.

**Leaf linked list** — leaf nodes are chained in a doubly-linked list during construction and reconstructed during deserialization. This enables O(N) ordered traversal (`listAll`) and efficient `searchAll` range scans without revisiting internal nodes.

---

## Complexity Summary

| Operation | Time complexity |
|---|---|
| Cold-start index build | O(N log N) |
| Warm start (load from binary) | O(N) — linear read |
| Primary lookup by ID | O(log N) |
| Secondary lookup by author / genre | O(log N + k) — k results |
| Insert | O(log N) amortized |
| Delete | O(log N) amortized |
| List all (sorted) | O(N) |

---

## Cleaning the Build

```bash
cmake --build build --target clean-all
```

This removes the entire `build/` directory.