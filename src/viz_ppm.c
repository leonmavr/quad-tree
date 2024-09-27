#include "viz.h"
#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ppm_width;
static int ppm_height;
static unsigned char *ppm_buffer;

void ppm_init(unsigned width, unsigned height)
{
    #if _WIN32
    _setmode(1, 0x8000);
    #endif
    ppm_width  = width  + 1;
    ppm_height = height + 1;
    ppm_buffer = malloc(3 * ppm_width * ppm_height);
    memset(ppm_buffer, 0xff, 3*ppm_width*ppm_height);
}

static void ppm_drawpoint(int x, int y, int color)
{
    if (x>=0 && x<ppm_width && y>=0 && y<ppm_height) {
        unsigned char *d = ppm_buffer + y*3*ppm_width + x*3;
        d[0] = color >> 16;
        d[1] = color >>  8;
        d[2] = color >>  0;
    }
}

void ppm_flush(void)
{
    printf("P6\n%d %d\n255\n", ppm_width, ppm_height);
    fwrite(ppm_buffer, ppm_width*3, ppm_height, stdout);
    memset(ppm_buffer, 0xff, 3*ppm_width*ppm_height);
    for (int x = 0; x < ppm_width; x++) {
        ppm_drawpoint(x, ppm_height-1, 0x000000);
    }
    for (int y = 0; y < ppm_height; y++) {
        ppm_drawpoint(ppm_width-1, y, 0x000000);
    }
    fflush(stdout);
}

void ppm_write_rect(rect_t *rect)
{
    rect_t r = *rect;
    for (int x = r.x0; x <= r.x1; x++) {
        ppm_drawpoint(x, r.y0, 0x000000);
    }
    for (int y = r.y0; y <= r.y1; y++) {
        ppm_drawpoint(r.x0, y, 0x000000);
    }
}

void ppm_write_point(point_t *p)
{
    int d[] = {-1, 0, +1, 0, 0, +1, 0, -1, 0, 0};
    for (int i = 0; i < 5; i++) {
        ppm_drawpoint(p->x+d[2*i+0], p->y+d[2*i+1], 0xff00ff);
    }
}

void ppm_close(void)
{
    free(ppm_buffer);
}

static bool _node_is_leaf(node_t* node) {
    return (node != NULL) ? (node->nw == NULL && node->ne == NULL &&
                            node->se == NULL && node->sw == NULL) : false;
}

static void _node_graph(node_t* node) {
    if (node == NULL) return;
    if (_node_is_leaf(node)) {
        ppm_write_rect(&node->boundary);
        for (int i = 0; i < node->count; ++i) {
            ppm_write_point(&node->points[i]);
        }
    } else {
        _node_graph(node->nw);
        _node_graph(node->ne);
        _node_graph(node->sw);
        _node_graph(node->se);
    }
}

void ppm_qtree_graph(quadtree_t* qtree) {
    _node_graph(qtree->root);
}
