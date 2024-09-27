#ifndef VIZ_PPM_H
#define VIZ_PPM_H

#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ppm_init(unsigned width, unsigned height);
void ppm_flush(void);
void ppm_write_rect(rect_t *rect);
void ppm_write_point(point_t *p);
void ppm_close(void);
void ppm_qtree_graph(quadtree_t* qtree);

#endif // VIZ_PPM_H
