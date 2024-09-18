#include "viz.h"
#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

static size_t _nobjects_to_clear;
static FILE* _plot_pipe;
static size_t _nrects;
static rect_t _viz_rects[NOBJECTS];
static point_t _viz_points[NOBJECTS];
static size_t _irect;
static size_t _ipoint;

void viz_init(unsigned width, unsigned height) {
    // don't close the window upon exit
    _plot_pipe = popen("gnuplot -persistent", "w");
    if (_plot_pipe == NULL) {
        fprintf(stderr, "ERROR: gnuplot not found! Please try installing it!\n");
        exit(EXIT_FAILURE);
    }
    fprintf(_plot_pipe, "set title 'Quadtree points and boundaries'\n");
    fprintf(_plot_pipe, "set xrange [0:%u]\n", width);
    fprintf(_plot_pipe, "set yrange [0:%u]\n", height);
    fprintf(_plot_pipe, "set size square\n");
    _nrects = 0;
    _nobjects_to_clear = 0;
    _irect = 0;
    _ipoint = 0;
}


void viz_flush() {
    _nobjects_to_clear = (_irect > _nobjects_to_clear) ? _irect : _nobjects_to_clear;
    // remove all previously drawn rectangles
    for (size_t i = 0; i < _nobjects_to_clear; ++i)
        fprintf(_plot_pipe, "unset object %ld\n", i + 1);
    for (int i = 0; i < _irect; ++i) {
        fprintf(_plot_pipe, "set object %d rect from %d,%d to %d,%d\n", 
            i + 1, _viz_rects[i].x0, _viz_rects[i].y0, _viz_rects[i].x1, _viz_rects[i].y1);
    }
    if (_ipoint > 0) {
        fprintf(_plot_pipe, "plot '-' with points pt 7 ps 1\n");
        for (int i = 0; i < _ipoint; ++i) {
            fprintf(_plot_pipe, "%d %d\n", _viz_points[i].x, _viz_points[i].y); 
        }
        fprintf(_plot_pipe, "e\n");  // End the input
    } else {
        fprintf(stderr, "Warning: No points to plot.\n");
    }
    fprintf(_plot_pipe, "replot\n");
    _irect = _ipoint = 0;
    fflush(_plot_pipe);
    usleep(16000);
}


void viz_write_rect(rect_t* rect) {
    _viz_rects[_irect++] = *rect;
}

void viz_write_point(point_t* point) {
    _viz_points[_ipoint++] = *point;
}

void viz_close() {
    pclose(_plot_pipe);
}
