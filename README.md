# Cache Simulator 

## Overview
This program is a command-line cache and virtual memory simulator.  
It parses input parameters, computes required cache and memory values, and prints the results.  

## Build
g++ -o team11_VMCacheSim main.cpp

## Usage
./team11_VMCacheSim -s <KB> -b <block size> -a <assoc> -r <rr|rnd> -p <MB> -u <percent> -n <instructions> -f <trace file>

## Example
./team11_VMCacheSim -s 512 -b 16 -a 4 -r rr -p 1024 -u 75 -n 100 -f A-9_new_trunk.trc

## Parameters
- `-s` Cache size in KB (8–8192, power of 2)
- `-b` Block size (8, 16, 32, 64 bytes)
- `-a` Associativity (1, 2, 4, 8, 16)
- `-r` Replacement policy (`rr` or `rnd`)
- `-p` Physical memory in MB (128–4096, power of 2)
- `-u` Percent of memory used by OS (0–100)
- `-n` Instructions per time slice (`-1` for max)
- `-f` Trace file (1–3)


## Notes
- Page size is 4 KB
- Output formatting follows assignment specifications
- Trace files are accepted but not processed in this milestone

## Team
- Meegan, Sean M.
- Guerra, Sage
- Hipolito, Kristian
- Teschan, Addison K.
