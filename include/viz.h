#ifndef VIZ_H
#define VIZ_H

#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

#define NOBJECTS 3000
extern size_t nobjects_clear;

static FILE* viz_plot_pipe;
static size_t viz_nrects;
static rect_t viz_rects[NOBJECTS];
static point_t viz_points[NOBJECTS];
static size_t irect;
static size_t ipoint;

void viz_init(unsigned width, unsigned height);
void viz_flush();
void viz_write_rect(rect_t* rect);
void viz_write_point(point_t* point);
void viz_close();

#endif // VIZ_H

