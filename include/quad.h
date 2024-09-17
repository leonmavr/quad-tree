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


void qtree_new(quadtree_t* qtree, rect_t* boundary);
void qtree_del(quadtree_t* qtree);
void qtree_insert(quadtree_t* qtree, point_t* point);
void qtree_graph(quadtree_t* qtree);
void qtree_query(quadtree_t* qtree, rect_t* search_area, int* count);
double qtree_nearest_neighbor(quadtree_t* qtree, point_t* query, point_t* nearest);
void qtree_update_point(quadtree_t* qtree, point_t* old_point, point_t* new_point);
void qtree_merge(quadtree_t* qtree);
void qtree_remove_point(quadtree_t* qtree, point_t* point);

static node_t* node_new(rect_t* boundary);
static bool node_is_leaf(node_t* node);
static void node_insert(node_t* node, point_t* point);
static void node_query(node_t* node, rect_t* search_area, int* count);
static void node_nearest_neighbor(node_t* node, point_t* query, point_t* nearest, double* best_dist_squared);
static double point_to_rect_distance(point_t* p, rect_t* rect);
static void node_remove_point(node_t* node, point_t* point);
static void node_merge(node_t* node);
static void node_del_all(node_t* node);
static void node_graph(node_t* node);

#endif // QUAD_H
