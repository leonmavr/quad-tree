#include "viz.h"
#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // usleep

static int ppm_width;
static int ppm_height;
static unsigned char *ppm;

void viz_init(unsigned width, unsigned height)
{
    #if _WIN32
    _setmode(1, 0x8000);
    #endif
    ppm_width  = width  + 1;
    ppm_height = height + 1;
    ppm = malloc(3 * ppm_width * ppm_height);
    memset(ppm, 0xff, 3*ppm_width*ppm_height);
}

static void drawpoint(int x, int y, int color)
{
    if (x>=0 && x<ppm_width && y>=0 && y<ppm_height) {
        unsigned char *d = ppm + y*3*ppm_width + x*3;
        d[0] = color >> 16;
        d[1] = color >>  8;
        d[2] = color >>  0;
    }
}

void viz_flush(void)
{
    printf("P6\n%d %d\n255\n", ppm_width, ppm_height);
    fwrite(ppm, ppm_width*3, ppm_height, stdout);
    memset(ppm, 0xff, 3*ppm_width*ppm_height);
    for (int x = 0; x < ppm_width; x++) {
        drawpoint(x, ppm_height-1, 0x000000);
    }
    for (int y = 0; y < ppm_height; y++) {
        drawpoint(ppm_width-1, y, 0x000000);
    }
    fflush(stdout);
}

void viz_write_rect(rect_t *rect)
{
    rect_t r = *rect;
    for (int x = r.x0; x <= r.x1; x++) {
        drawpoint(x, r.y0, 0x000000);
    }
    for (int y = r.y0; y <= r.y1; y++) {
        drawpoint(r.x0, y, 0x000000);
    }
}

void viz_write_point(point_t *p)
{
    int d[] = {-1, 0, +1, 0, 0, +1, 0, -1, 0, 0};
    for (int i = 0; i < 5; i++) {
        drawpoint(p->x+d[2*i+0], p->y+d[2*i+1], 0xff00ff);
    }
}

void viz_close(void) {}

static bool _node_is_leaf(node_t* node) {
    return (node != NULL) ? (node->nw == NULL && node->ne == NULL &&
                            node->se == NULL && node->sw == NULL) : false;
}

static void _node_graph(node_t* node) {
    if (node == NULL) return;
    if (_node_is_leaf(node)) {
        viz_write_rect(&node->boundary);
        for (int i = 0; i < node->count; ++i) {
            viz_write_point(&node->points[i]);
        }
    } else {
        _node_graph(node->nw);
        _node_graph(node->ne);
        _node_graph(node->sw);
        _node_graph(node->se);
    }
}

void viz_qtree_graph(quadtree_t* qtree) {
    _node_graph(qtree->root);
}

