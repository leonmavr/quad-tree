# 1. Introduction

## 1.1 Quadtree

Quadtree is a data structure used to index spatial data. Each node always has 4
children. The branching strategy depends on the kind of quadtree. In this
project, a **point quadtree** is implemented.

## 1.2 Point quadtree

The idea is that each node represents a rectangel in 2D space. The rectangle can
store a limited number of points. If its capacity is reached, a node gets
divided into 4 NW, NE, SE, SW children (leaves), where its points get
re-distributed. Some nice theoretical resources are found in [1], [2].

To get some intuition, the schematic below shows what happens to a node of
capacity of 2 when 3 points are sequentially inserted.
```
capacity=2                                       NW                 NE
+--------------------+  +--------------------+  +--------------------+
|                    |  |                    |  |          |         |
|     *              |  |     *              |  |     *    |         |
|                    |  |                    |  |          |         |
|                    |  |                    |  |          |         |
|                    |  |                    |  |----------+---------|
|                    |  |                    |  |          |         |
|                    |  |                *   |  |       *  |     *   |
|                    |  |                    |  |          |         |
|                    |  |                    |  |          |         | 
+--------------------+  +--------------------+  +--------------------+
                                                SW                  SE
```

# 2. Usage

## 2.1 Project structure
```
.
├── examples
│   ├── 01_particle_sim.c    <- demo
├── include
│   ├── quad.h               <- library
│   └── viz.h                <- GNUplot visualizer
├── src
│   ├── quad.c               <- library
│   └── viz.c                <- GNUplot visualizer
└── test
    ├── nanotest.h           <- testing framework
    └── test.c               <- unit tests

```

## 2.2. Dependencies and required tools

* `make` to compile the project.
* `gnuplot` to run the particle simulation demo.
* `gcc` is the default compiler, set in the Makefile.

## 2.3. Compilation

To compile the library with any demo(s):
```
make
```
Then all demos under `examples` will be compiled against the library. All
executables will be generated in the root. A particle simulation to demonstrate
how the tree works has been written. To run it:
```
./01_particle_sim
```
To compile and run the unit tests:
```
make test
```
To clean all executables and object files:
```
make clean
```

# 3. Particle simulation demo

TODO

# References

\[1\] [CMSC 420](https://www.cs.cmu.edu/~ckingsf/bioinfo-lectures/quadtrees.pdf)
\[2\] [Dortmund
uni](https://ls11-www.cs.tu-dortmund.de/_media/buchin/teaching/akda_ws21/quadtrees.pdf)
