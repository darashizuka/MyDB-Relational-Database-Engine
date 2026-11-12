# MyDB: Relational Database Engine

A relational database engine built from scratch in C++, featuring buffer management, B+ tree indexing, relational operators, SQL parsing, and a cost-based query optimizer. Tested against TPC-H benchmark queries on multi-table joins with aggregation.

## Architecture

```
SQL Query
   │
   ▼
┌──────────────────┐
│  Lexer + Parser  │  Flex/Bison SQL frontend
│  (Lexer.l,       │
│   Parser.y)      │
└────────┬─────────┘
         │  AST (ExprTree)
         ▼
┌──────────────────┐
│  Query Optimizer  │  Cost-based plan enumeration
│  (SFWQuery,      │  using table statistics
│   MyDB_Stats)    │
└────────┬─────────┘
         │  LogicalOp tree
         ▼
┌──────────────────┐
│ Execution Engine  │  Logical-to-physical mapping
│  (LogicalOps →   │
│   RelOps)        │
└────────┬─────────┘
         │
    ┌────┴─────┐
    ▼          ▼
┌────────┐ ┌────────────┐
│ RelOps │ │ B+ Tree    │
│        │ │ Index Scan │
└───┬────┘ └─────┬──────┘
    └──────┬─────┘
           ▼
┌──────────────────┐
│  Buffer Manager   │  LRU page cache with
│  (MyDB_Page,     │  configurable pool size
│   PageHandle)    │
└──────────────────┘
```

## Components

### Storage Layer
- **Buffer Manager** -- LRU page replacement policy, manages a configurable pool of fixed-size pages backed by disk files
- **Page Reader/Writer** -- Reads and writes variable-length records within pages, handles page overflow

### Indexing
- **B+ Tree** -- Supports insert, lookup, and range scans over sorted keys for indexed selections

### Record System
- **Record / AttVal** -- Supports `int`, `double`, `string`, and `bool` attribute types with a compiled expression evaluator for predicates and projections

### Relational Operators
- **RegularSelection** -- Full table scan with predicate filtering and projection
- **BPlusSelection** -- Index-based selection using B+ tree range scans
- **ScanJoin** -- Hash-based join with in-memory hash table on the smaller input
- **SortMergeJoin** -- External sort-merge join using sorted runs for large inputs
- **Aggregate** -- GROUP BY with SUM, AVG, and COUNT aggregation functions

### SQL Frontend
- **Lexer** (Flex) -- Tokenizes SQL input into keywords, identifiers, literals, and operators
- **Parser** (Bison) -- Parses SELECT-FROM-WHERE-GROUP BY queries into an AST

### Query Optimizer
- **Cost-Based Optimization** -- Enumerates all possible join orderings via binary partitioning, estimates output cardinalities using table statistics, and selects the plan with the lowest total estimated cost
- **Statistics** -- Tracks distinct value counts per attribute; estimates selectivity for equality, inequality, and range predicates

## Query Capabilities

Supports SQL queries with:
- Multi-table joins (tested up to 4-way joins)
- Selection predicates with AND/OR, comparison operators, arithmetic, and string operations
- Aggregation: `SUM()`, `AVG()`, `COUNT`
- `GROUP BY` with multiple grouping attributes
- Expression computation in SELECT (e.g., `l_extendedprice * (1.0 - l_discount)`)

## Sample Results (TPC-H)

The engine was validated against 12 TPC-H-derived queries on the `lineitem`, `orders`, `customer`, `nation`, `supplier`, `part`, and `partsupp` tables. Examples:

**Q5 -- Revenue by return flag with aggregation:**
```
|return flag was N |sum: 7826475 |sum: 11731187444.12 |avg: 38203.87 |
Found 1 records in all.
```

**Q8 -- Multi-table join (nation × orders × lineitem) with date filtering:**
```
|JORDAN    |1998-07-30 |291.942900    |
|ROMANIA   |1998-03-28 |18835.132800  |
|KENYA     |1998-08-02 |6909.096800   |
...
Found 2347 records in all.
```

**Q11 -- Customer × Nation join with GROUP BY and COUNT:**
```
|Customer#000000034|MOROCCO  |8  |
|Customer#000000100|SAUDI ARABIA |10 |
...
Found 72172 records in all.
```

## Build

Requires SCons, Flex, Bison, and a C++14 compiler.

```bash
cd Build
scons
```

## Project Structure

```
├── Build/                  # SCons build configuration
├── Main/
│   ├── BufferMgr/          # Buffer manager and page handling
│   ├── Catalog/            # Schema, table, and attribute type definitions
│   ├── DatabaseTable/      # Page/table reader-writers, B+ tree, sorting
│   ├── Execution/          # Query optimizer, logical operators, statistics
│   ├── Record/             # Record and attribute value implementation
│   ├── RelOps/             # Relational operators (select, join, aggregate)
│   ├── SQL/                # Flex lexer and Bison parser
│   ├── *Test/              # Unit tests for each component
│   └── Qunit/              # Test framework
└── results.txt             # Output from TPC-H benchmark queries
```
