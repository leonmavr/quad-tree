#ifndef QUAD_H
#define QUAD_H

#include "stdbool.h"
#include <stdlib.h> // abs
#define IND_NW 0
#define IND_NE 1
#define IND_SE 2
#define IND_SW 3

static int g_capacity = 2;  // Maximum number of points per node

typedef struct point_t {
    int x;
    int y;
} point_t;

typedef struct rect_t {
    int x0;
    int y0;
    int x1;
    int y1;
} rect_t;

typedef struct node_t {
    rect_t boundary;
    point_t *points;
    int count;
    struct node_t *nw; // Northwest
    struct node_t *ne; // Northeast
    struct node_t *sw; // Southwest
    struct node_t *se; // Southeast
} node_t;

typedef struct quadtree_t {
    node_t* root;
} quadtree_t;



static bool point_in_rect(point_t* point, rect_t* boundary) {
    return point->x >= boundary->x0 && point->x <= boundary->x1 &&
           point->y >= boundary->y0 && point->y <= boundary->y1;
}

static int rect_area(rect_t* rect) {
    return abs(rect->x0 - rect->x1) * abs(rect->y0 - rect->y1);
}

node_t* node_new(rect_t* boundary);
void rect_divide(rect_t* src, rect_t* dest);
int point_get_quadrant(rect_t* rect, point_t* point);
bool node_is_leaf(node_t* node);
void node_insert(node_t* node, point_t* point);

bool rect_intersect(rect_t* r1, rect_t* r2);

#endif // QUAD_H
