# 1. Introduction

## 1.1 Quadtree

Quadtree is a data structure used to index spatial data. Each node always has 4
children. The branching strategy depends on the kind of quadtree. In this
project, a **point quadtree** is implemented.

## 1.2 Point quadtree

The idea is that each node represents a rectangel in 2D space. The rectangle can
store a limited number of points. If its capacity is reached, a node gets
divided into 4 NW, NE, SE, SW children (leaves), where its points get
re-distributed. Some nice theoretical resources are found in [1], [2], [3].

To get some intuition, the schematic below shows what happens to a node of
capacity of 2 when 3 points are sequentially inserted.
```
spatial representation:
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
tree representation:
     +--+                      +--+                      +--+
     |* |                      |**|                      |  |
     +--+                      +--+                      +--+
                                                          | 
                                                          |
                                               +------+---+---+------+
                                               |      |       |      |
                                              +--+   +--+   +--+   +--+
                                              |* |   |  |   |* |   |* |
                                           NW +--+ NE+--+ SE+--+ SW+--+
```

# 2. Usage

## 2.1 Project structure
```
.
├── examples                 <- directory where demos can be added
│   └── 01_particle_sim.c    <- demo
├── include
│   ├── quad.h               <- library 
│   ├── viz_gplot.h          <- gnuplot plotter
│   ├── viz.h                <- plotter interface
│   └── viz_ppm.h            <- ppm frame plotter
├── src
│   ├── quad.c               <- library
│   ├── viz_gplot.c          <- gnuplot plotter
│   └── viz_ppm.c            <- ppm frame plotter
└── test
    ├── nanotest.h           <- unit test framework
    └── tests.c              <- unit tests
```
The visualizer plots the quadtree in real time either via a pipe fed to GNUplot
or directly as .ppm frames. The quadtree library is independent from the
visualizer.

## 2.2. Dependencies and required tools

* `make` to compile the project.
* `gcc` is the default compiler, set in the Makefile.
*  either `gnuplot` or a media player like `mpv` to run the particle simulation
demo.

## 2.3. Compilation

You can compile the example(s) with either GNUplot or .ppm frame emitter as the
visualizer.

| gnuplot  | ppm frames       |
| -------- | ---------------- |
| `make`   | `make -DUSE_PPM` |

Then all demos under `examples` will be compiled against the library. All
executables will be generated in the root. A particle simulation to demonstrate
how the tree works has been written. To run it:

| gnuplot               | ppm frames                                              |
| --------------------- | ------------------------------------------------------- |
| `./01_particle_sim`   | `./01_particle_sim \| mpv --no-correct-pts --fps=30 -`  |

To compile and run the unit tests:
```
make test
```
To clean all executables and object files:
```
make clean
```

# 3. Particle simulation demo

![Particle
demo](https://raw.githubusercontent.com/leonmavr/quad-tree/refs/heads/master/assets/01_particle_sim.gif)
  
[High quality video
version](https://github.com/leonmavr/quad-tree/blob/master/assets/01_particle_sim.mp4)

# References

\[1\] [CMSC 420](https://www.cs.cmu.edu/~ckingsf/bioinfo-lectures/quadtrees.pdf)  
\[2\] [Dortmund
uni](https://ls11-www.cs.tu-dortmund.de/_media/buchin/teaching/akda_ws21/quadtrees.pdf)  
\[3\] [CS267 Berkley](https://people.eecs.berkeley.edu/~demmel/cs267/lecture26/lecture26.html)
