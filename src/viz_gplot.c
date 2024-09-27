#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

#define NOBJECTS 3000

static size_t _nobjects_to_clear;
static FILE* _plot_pipe;
static size_t _nrects;
static rect_t _viz_rects[NOBJECTS];
static point_t _viz_points[NOBJECTS];
static size_t _irect;
static size_t _ipoint;

void gplt_init(unsigned width, unsigned height) {
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


void gplt_flush() {
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


void gplt_write_rect(rect_t* rect) {
    _viz_rects[_irect++] = *rect;
}

void gplt_write_point(point_t* point) {
    _viz_points[_ipoint++] = *point;
}

void gplt_close() {
    pclose(_plot_pipe);
}

static bool _node_is_leaf(node_t* node) {
    return (node != NULL) ? (node->nw == NULL && node->ne == NULL &&
                            node->se == NULL && node->sw == NULL) : false;
}

static void _node_graph(node_t* node) {
    if (node == NULL) return;
    if (_node_is_leaf(node)) {
        gplt_write_rect(&node->boundary);
        for (int i = 0; i < node->count; ++i) {
            gplt_write_point(&node->points[i]);
        }
    } else {
        _node_graph(node->nw);
        _node_graph(node->ne);
        _node_graph(node->sw);
        _node_graph(node->se);
    }
}

void gplt_qtree_graph(quadtree_t* qtree) {
    _node_graph(qtree->root);
}
