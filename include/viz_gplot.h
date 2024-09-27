#ifndef VIZ_GPLOT_H
#define VIZ_GPLOT_H

#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void gplt_init(unsigned width, unsigned height);
void gplt_flush(void);
void gplt_write_rect(rect_t *rect);
void gplt_write_point(point_t *p);
void gplt_close(void);
void gplt_qtree_graph(quadtree_t* qtree);

#endif // VIZ_GPLOT_H
