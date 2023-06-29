
# Declarative Concurrent Data Structures (DCDS)


## Considerations

- Contention on data structure attributes. For example, in a simple FIFO queue, if the data structure maintains size as a class attribute, and is read/updated across the frequency operations, then it becomes the contention point, and hence, serializes all concurrent operations which is undesirable. Either think of alternatives, like implementing a empty() operation which checks for head/tail existence only, or use size in a read-only separate operation (later, it can be approximate or optimistic reads also), or in the future, use scans for getting size.

## Dependencies

- Currently, is a C++20 project and effectively depends on (Pelago)[https://gitlab.epfl.ch/DIAS/PROJECTS/caldera/pelago] to use a modern clang on DIAS servers.

