# Cache Simulator

### The goal of this project is to measure the effectiveness of cache subsystem organizations using traces of memory instructions obtained from the realistic programs. Each trace contains about 1 million memory instructions with two values provided for each instruction: a flag indicating whether this is a load or a store (L stands for a load, S stands for a store), and the byte address targeted by this instruction. 
### The goal is to write a program in C or C++ that would use these traces to measure the cache hit rate of various data cache organizations and prefetching techniques (note: we are not estimating the instruction cache performance in this project, only the data cache). Specifically, the following cache designs have to be implemented.

#### The following cache designs have been implemented: 
1. Direct-Mapped Cache
2. Set-Associative Cache
3. Fully-Associative Cache with LRU Policy
4. Fully-Associative Cache with Hot-Cold LRU Approximation Policy
5. Set-Associative Cache with No Allocation on a Write Miss
6. Set-Associative Cache with Next-Line Prefetching
7. Set-Associative Cache with Next-Line Prefetching on a Miss
