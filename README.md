# Cache Simulator

## Overview
This program is a command-line cache and virtual memory simulator for a 32-bit system. It models:

- Virtual Memory: virtual-to-physical page mapping using trace files
- Cache System: configurable L1 cache with hit/miss tracking and CPI calculation

The simulator processes instruction (`EIP`) and data (`dstM`, `srcM`) accesses and reports performance metrics including memory behavior, cache efficiency, and CPI.

---

## Build
`g++ -std=c++11 -Wall -O2 -o team11_VMCacheSim team11_VMCacheSim_M3.cpp`

---

## Usage
`./team11_VMCacheSim -s <cache size KB> -b <block size> -a <associativity> -r <rr|rnd> -p <physical memory MB> -u <percent used by OS> -n <instructions per time slice> -f <trace1> [-f <trace2>] [-f <trace3>]`

---

## Example
`./team11_VMCacheSim -s 512 -b 16 -a 4 -r rr -p 1024 -u 75 -n 100 -f A-9_new_trunk1.trc -f A-9_new_trunk2.trc -f Corruption2.trc`

---

## Parameters
- `-s` Cache size in KB (8–8192, power of 2)
- `-b` Block size (8, 16, 32, 64 bytes)
- `-a` Associativity (1, 2, 4, 8, 16)
- `-r` Replacement policy (`rr` or `rnd`)
- `-p` Physical memory in MB (128–4096, power of 2)
- `-u` Percent of memory used by OS (0–100)
- `-n` Instructions per time slice (`-1` for max)
- `-f` Trace file(s) (1–3 supported, each representing a process)

---

## Virtual Memory Simulation 
- Page size: 4 KB
- 32-bit virtual address space (4 GB)
- 512K page table entries per process
- Each entry uses a valid bit

### Behavior
- Free physical pages are managed using a vector
- Pages are allocated using LIFO strategy (pop_back)
- When a process completes, its pages are returned to the free pool

### Page Replacement 
- When no free physical pages are available:
    - A victim page is selected
    - The old virtual-to-physical mapping is removed
    - The physical frame is reassigned
    - Any cache entries corresponding to that physical page are invalidated

### Statistics Reported
- Virtual Pages Mapped
- Page Table Hits
- Pages Allocated from Free Memory
- Total Page Faults
- Per-process page table usage and wasted memory

---

## Cache Simulation 
- Configurable cache size, block size, associativity
- Replacement policy: round-robin or random

### Tracks
- Total cache accesses
- Cache hits and misses
- Compulsory and conflict misses

### Performance Metrics
- Hit Rate / Miss Rate
- CPI (Cycles Per Instruction)
- Unused cache space (KB and %)
- Cost efficiency

---

## CPI Model
- Cache hit: 1 cycle
- Memory access: 4 cycles per 4-byte read
- Instruction execution: +2 cycles
- Address calculation (data): +1 cycle
- Page fault penalty: +100 cycles

---

## Trace File Handling
- Processes:
    - EIP → instruction fetch (variable length)
    - dstM, srcM → 4-byte data accesses
- Ignores invalid accesses ("--------")
- Each address handled independently
- Supports up to 3 concurrent processes

---

## Notes
- Cache is always smaller than physical memory
- Large page tables (512K entries) lead to unused space for small workloads
- Cache invalidation occurs on page replacement
- Output formatting follows assignment specifications

---

## Team
- Meegan, Sean M.
- Guerra, Sage
- Hipolito, Kristian
- Teschan, Addison K.