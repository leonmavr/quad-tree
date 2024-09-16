#ifndef QUAD_H
#define QUAD_H

#include "stdbool.h"
#include <stdlib.h> // abs
#include <stddef.h> // size_t
#define IND_NW 0
#define IND_NE 1
#define IND_SE 2
#define IND_SW 3

static int g_capacity = 2;  // Maximum number of points per node
static size_t g_point_id = 0; // ID of each point

typedef struct point_t {
    int x;
    int y;
    size_t id; // unique identifier
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

void qtree_new(quadtree_t* qtree, rect_t* boundary);
void qtree_del(quadtree_t* qtree);
void qtree_insert(quadtree_t* qtree, point_t* point);
void qtree_graph(quadtree_t* qtree);
void qtree_query(quadtree_t* qtree, rect_t* search_area, int* count);
double qtree_nearest_neighbor(quadtree_t* qtree, point_t* query, point_t* nearest);

node_t* node_new(rect_t* boundary);
void rect_divide(rect_t* src, rect_t* dest);
int point_get_quadrant(rect_t* rect, point_t* point);
bool node_is_leaf(node_t* node);
void node_insert(node_t* node, point_t* point);

bool rect_intersect(rect_t* r1, rect_t* r2);
void node_query(node_t* node, rect_t* search_area, int* count);

void node_nearest_neighbor(node_t* node, point_t* query, point_t* nearest, double* best_dist_squared);
double point_to_rect_distance(point_t* p, rect_t* rect);
void node_remove_point(node_t* node, point_t* point);
void node_merge(node_t* node);
void qtree_update_point(quadtree_t* qtree, point_t* old_point, point_t* new_point);
void node_del_all(node_t* node);
void node_graph(node_t* node);

#endif // QUAD_H
