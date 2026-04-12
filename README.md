# Cache Simulator 

## Overview
This program is a command-line cache and virtual memory simulator.  
It parses input parameters, computes cache and physical memory values (M1), and simulates virtual memory behavior using trace files (M2).  

The simulator processes instruction (`EIP`) and data (`dstM`, `srcM`) memory accesses, tracks virtual-to-physical page mappings, and reports statistics such as page table hits, pages allocated from free memory, and page faults.

---

## Build
g++ -std=c++11 -Wall -O2 -o team11_VMCacheSim_M2 team11_VMCacheSim_M2.cpp

---

## Usage
./team11_VMCacheSim_M2 -s <KB> -b <block size> -a <assoc> -r <rr|rnd> -p <MB> -u <percent> -n <instructions> -f <trace file> [-f <trace file> ...]

---

## Example
./team11_VMCacheSim_M2 -s 512 -b 16 -a 4 -r rr -p 1024 -u 75 -n 100 -f A-9_new_trunk1.trc -f A-9_new_trunk2.trc -f Corruption2.trc

---

## Parameters
- `-s` Cache size in KB (8–8192, power of 2)
- `-b` Block size (8, 16, 32, 64 bytes)
- `-a` Associativity (1, 2, 4, 8, 16)
- `-r` Replacement policy (`rr` or `rnd`)
- `-p` Physical memory in MB (128–4096, power of 2)
- `-u` Percent of memory used by OS (0–100)
- `-n` Instructions per time slice (`-1` for max)
- `-f` Trace file (supports 1–3 trace files, each representing a process)

---

## Virtual Memory Simulation (M2)
- Virtual addresses are divided by `PAGE_SIZE` (4 KB) to determine virtual page numbers  
- Each process maintains a page table with 512K entries  
- Page tracking is performed using a valid bit per entry  
- Free physical pages are managed using a vector  
- Page allocation uses a LIFO strategy (`pop_back`)  
- Pages are returned to the free pool when a process completes  

### Statistics Reported
- Virtual Pages Mapped  
- Page Table Hits  
- Pages Allocated from Free Memory  
- Total Page Faults  
- Per-process page table usage and wasted memory  

---

## Notes
- Page size is fixed at 4 KB  
- Page tables are large (512K entries per process), resulting in significant unused space for sparse workloads  
- Page replacement is **not implemented** in M2; page faults are counted but not resolved  
- Output formatting follows assignment specifications exactly  
- Trace file parsing supports `EIP`, `dstM`, and `srcM` entries, ignoring invalid accesses  

---

## Team
- Meegan, Sean M.
- Guerra, Sage
- Hipolito, Kristian
- Teschan, Addison K.
