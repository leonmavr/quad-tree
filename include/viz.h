#ifndef VIZ_H
#define VIZ_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // usleep

static FILE* viz_plot_pipe;
static size_t viz_nrects;

void viz_init(unsigned width, unsigned height) {
    viz_plot_pipe = popen("gnuplot -persistent", "w");
    if (viz_plot_pipe) {
        fprintf(viz_plot_pipe, "set title 'Quadtree points and boundaries'\n");
        fprintf(viz_plot_pipe, "set xrange [0:%u]\n", width);
        fprintf(viz_plot_pipe, "set yrange [0:%u]\n", height);
    } else {
        fprintf(stderr, "ERROR: Could not open pipe to gnuplot!\n");
    }
    viz_nrects = 0;
}

void viz_write_rect(int x0, int y0, int x1, int y1) {
    fprintf(viz_plot_pipe, "set object %zu rect from %d,%d to %d,%d\n",
        ++viz_nrects, x0, y0, x1, y1);
}

void viz_write_point(int x, int y) {
    fprintf(viz_plot_pipe, "plot '-' with points pointtype 7 pointsize 2\n");
    fprintf(viz_plot_pipe, "%d %d\n", x, y);
    // to end the point input
    fprintf(viz_plot_pipe, "e\n");
}

void viz_flush() {
    fflush(viz_plot_pipe);
    usleep(16000);
}

void viz_close() {
    pclose(viz_plot_pipe);
}

#endif // VIZ_H

