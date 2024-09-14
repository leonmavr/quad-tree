#include "viz.h"
#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

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
    irect = 0;
    ipoint = 0;
}

void viz_flush() {
    fprintf(viz_plot_pipe, "clear\n");
    for (int i = 0; i < irect; ++i) {
        fprintf(viz_plot_pipe, "set object %zu rect from %d,%d to %d,%d\n", 
            i + 1, viz_rects[i].x0, viz_rects[i].y0, viz_rects[i].x1, viz_rects[i].y1);
    }
    if (ipoint > 0) {
        fprintf(viz_plot_pipe, "plot '-' with points pt 7 ps 1 title 'Points'\n");
        for (int i = 0; i < ipoint; ++i)
            fprintf(viz_plot_pipe, "%d %d\n", viz_points[i].x, viz_points[i].y); 
        fprintf(viz_plot_pipe, "e\n"); // end the input
    } else {
        fprintf(stderr, "Warning: No points to plot.\n");
    }
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
