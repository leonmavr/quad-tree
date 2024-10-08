#ifndef QUAD_H
#define QUAD_H

#include "stdbool.h"
#include <stddef.h> // size_t
#define IND_NW 0
#define IND_NE 1
#define IND_SE 2
#define IND_SW 3

extern int node_capacity;

typedef struct point_t {
    int x;
    int y;
    size_t id; // unique identifier
} point_t;

typedef struct rect_t {
    int x0, y0;
    int x1, y1;
} rect_t;

typedef struct node_t {
    rect_t boundary;
    point_t *points;
    int count;
    struct node_t *nw; // Northwest child
    struct node_t *ne; // Northeast child
    struct node_t *sw; // Southwest child
    struct node_t *se; // Southeast child
} node_t;

typedef struct quadtree_t {
    node_t* root;
} quadtree_t;


void qtree_new(quadtree_t* qtree, rect_t* boundary);
void qtree_insert(quadtree_t* qtree, point_t* point);
void qtree_query(quadtree_t* qtree, rect_t* search_area, int* count);
double qtree_nearest_neighbor(quadtree_t* qtree, point_t* query, point_t* nearest);
void qtree_remove_point(quadtree_t* qtree, point_t* point);
void qtree_update_point(quadtree_t* qtree, point_t* old_point, point_t* new_point);
void qtree_merge(quadtree_t* qtree);
void qtree_del(quadtree_t* qtree);
extern void (*qtree_graph)(quadtree_t*);

#endif // QUAD_H
