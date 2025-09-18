# OS-Assignment2
## Virtual-Memory Page Replacement Simulator

### Overview
This project implements a simulator for evaluating different page replacement algorithms in a virtual memory system. It reads memory access traces and simulates how pages are loaded, replaced, and written back to disk under varying policies.

The simulator supports four algorithms:
- Random (rand) – evicts a randomly chosen frame.
- FIFO (fifo) – evicts the oldest page loaded (first-in-first-out).
- LRU (lru) – evicts the least-recently used page.
- CLOCK (clock) – uses a single-hand clock (second-chance) algorithm with reference bits.

Two run modes are available:
- Quiet (quiet) – prints only summary statistics at the end.
- Debug (debug) – prints detailed logs for each memory access, page fault, and eviction.

### Build Instructions
This project comes with a Makefile to automate building and testing.

To Build: <br>
`make build` <br>
This will compile memsim.c into bin/memsim.

### Usage
Run the simulator manually with: <br>
`
./bin/memsim <tracefile> <num_frames> <algorithm> <mode>
`

example: <br>
`./bin/memsim traces/trace1 4 lru quiet`

### Running Tests
The Makefile also provides a test target that runs the simulator on sample traces with different algorithms and frame sizes. <br><br>
`make test`<br>
This will:
- Read inputs from the traces/ directory.
- Run the simulator with different algorithms (esc, lru) and frame sizes.
- Save results into the outputs/ directory.

Expected outputs (for verification) are stored in expected_outputs/.

### Cleaning Up
To remove build artifacts and test outputs: <br>
`make clean`

### Input Format
Traces are plain text files containing one memory access per line:
```
0041f7a0 R
13f5e2c0 R
31348900 W
```

- Address is in hexadecimal.
- Access type is R (read) or W (write).
- Page size is fixed at 4KB (12-bit offset).

### Output
At the end of execution, the simulator prints:
- Total memory frames
- Events in trace
- Disk reads (page-ins)
- Disk writes (evicted dirty pages)
- Page fault rate

Example:
```
total memory frames:  6
events in trace:      1000000
total disk reads:     45678
total disk writes:    12345
page fault rate:      0.0457
```