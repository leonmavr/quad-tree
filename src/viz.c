#include "viz.h"
#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

static size_t nobjects_clear;
static FILE* viz_plot_pipe;
static size_t viz_nrects;
static rect_t viz_rects[NOBJECTS];
static point_t viz_points[NOBJECTS];
static size_t irect;
static size_t ipoint;

void viz_init(unsigned width, unsigned height) {
    // don't close the window upon exit
    viz_plot_pipe = popen("gnuplot -persistent", "w");
    if (viz_plot_pipe == NULL) {
        fprintf(stderr, "ERROR: gnuplot not found! Please try installing it!\n");
        exit(EXIT_FAILURE);
    }
    fprintf(viz_plot_pipe, "set title 'Quadtree points and boundaries'\n");
    fprintf(viz_plot_pipe, "set xrange [0:%u]\n", width);
    fprintf(viz_plot_pipe, "set yrange [0:%u]\n", height);
    fprintf(viz_plot_pipe, "set size square\n");
    viz_nrects = 0;
    nobjects_clear = 0;
    irect = 0;
    ipoint = 0;
}


void viz_flush() {
    nobjects_clear = (irect > nobjects_clear) ? irect : nobjects_clear;
    // remove all previously drawn rectangles
    for (size_t i = 0; i < nobjects_clear; ++i)
        fprintf(viz_plot_pipe, "unset object %d\n", i + 1);
    for (int i = 0; i < irect; ++i) {
        fprintf(viz_plot_pipe, "set object %d rect from %d,%d to %d,%d\n", 
            i + 1, viz_rects[i].x0, viz_rects[i].y0, viz_rects[i].x1, viz_rects[i].y1);
    }
    if (ipoint > 0) {
        fprintf(viz_plot_pipe, "plot '-' with points pt 7 ps 1\n");
        for (int i = 0; i < ipoint; ++i) {
            fprintf(viz_plot_pipe, "%d %d\n", viz_points[i].x, viz_points[i].y); 
        }
        fprintf(viz_plot_pipe, "e\n");  // End the input
    } else {
        fprintf(stderr, "Warning: No points to plot.\n");
    }
    fprintf(viz_plot_pipe, "replot\n");
    irect = ipoint = 0;
    fflush(viz_plot_pipe);
    usleep(16000);
}


void viz_write_rect(rect_t* rect) {
    viz_rects[irect++] = *rect;
}

void viz_write_point(point_t* point) {
    viz_points[ipoint++] = *point;
}

void viz_close() {
    pclose(viz_plot_pipe);
}
