# CSE-240A-Cache-Simulator

1. Clone the Repo
2. Make Changes to the main.c File
3. Compile Using "make"

Cache Properties :
Cache Size can be modified by updating parameters like : CACHE_SIZE, BLOCK_SIZE, ASSOCIATIVITY -- all in powers of 2

Usage: bunzip2 -kc gcc.trace.bz2 | ./main
--verbose:0/1/2/3/4  (0:Default) Print different detailed debug message on stdout
--replacement:0/1    Use (0:default) for FIFO replacement policy, use (1) for LIFO replacement policy
