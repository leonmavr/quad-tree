#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

#define NOBJECTS 3000

static FILE* _plot_pipe;
static rect_t _viz_rects[NOBJECTS];
static point_t _viz_points[NOBJECTS];
static size_t _irect;
static size_t _ipoint;

void gplt_init(unsigned width, unsigned height) {
    const char *term = getenv("QTREE_GNUPLOT_TERM");
    if (term == NULL || *term == '\0') {
        term = "x11";
    }

    // don't close the window upon exit
    _plot_pipe = popen("gnuplot -persistent", "w");
    if (_plot_pipe == NULL) {
        fprintf(stderr, "ERROR: gnuplot not found! Please try installing it!\n");
        exit(EXIT_FAILURE);
    }
    fprintf(_plot_pipe, "set term %s\n", term);
    fprintf(_plot_pipe, "set title 'Quadtree points and boundaries'\n");
    fprintf(_plot_pipe, "set xrange [0:%u]\n", width);
    fprintf(_plot_pipe, "set yrange [0:%u]\n", height);
    fprintf(_plot_pipe, "set size square\n");
    fprintf(_plot_pipe, "set key off\n");
    _irect = 0;
    _ipoint = 0;
}


void gplt_flush() {
    fprintf(_plot_pipe,
            "plot '-' with lines lc rgb '#111111', '-' with points pt 7 ps 1 lc rgb '#ff00ff'\n");
    for (int i = 0; i < _irect; ++i) {
        rect_t rect = _viz_rects[i];
        fprintf(_plot_pipe, "%d %d\n", rect.x0, rect.y0);
        fprintf(_plot_pipe, "%d %d\n", rect.x1, rect.y0);
        fprintf(_plot_pipe, "%d %d\n", rect.x1, rect.y1);
        fprintf(_plot_pipe, "%d %d\n", rect.x0, rect.y1);
        fprintf(_plot_pipe, "%d %d\n\n", rect.x0, rect.y0);
    }
    fprintf(_plot_pipe, "e\n");
    for (int i = 0; i < _ipoint; ++i) {
        fprintf(_plot_pipe, "%d %d\n", _viz_points[i].x, _viz_points[i].y);
    }
    fprintf(_plot_pipe, "e\n");
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
    return (node != NULL) ? node->is_leaf : false;
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
        _node_graph(node->se);
        _node_graph(node->sw);
    }
}

void gplt_qtree_graph(quadtree_t* qtree) {
    _node_graph(qtree->root);
}
