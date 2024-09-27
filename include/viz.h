#ifndef VIZ_H
#define VIZ_H

#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

#define NOBJECTS 3000

/**
 * Interface for the quadtree visualizer.
 * The implementation of the member functions can be done in your preferred 
 * way, e.g. ppm frames, gnuplot, etc.
 */
typedef struct viz_t {
    void (*viz_init)(unsigned width, unsigned height);
    void (*viz_flush)(void);
    void (*viz_write_rect)(rect_t* rect);
    void (*viz_write_point)(point_t* point);
    void (*viz_close)(void);
    void (*viz_qtree_graph)(quadtree_t* qtree);
} viz_t;

#endif // VIZ_H
