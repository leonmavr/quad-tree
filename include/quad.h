#ifndef QUAD_H
#define QUAD_H

#include "stdbool.h"
#include <stddef.h> // size_t

enum {
    IND_NW = 0, // it must start from 0 for indexing
    IND_NE,
    IND_SE,
    IND_SW
};

// Maximum number of points per leaf node
#ifndef NODE_CAPACITY
#define NODE_CAPACITY 4
#endif

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
    point_t points[NODE_CAPACITY];
    int count;
    bool is_leaf; // cached flag avoids repeated 4-pointer check
    struct node_t *children[4]; // NW, NE, SE, SW children
} node_t;

/**
 * Arena allocator - call qtree_arena_init() once at startup, then
 * qtree_arena_reset() each frame instead of qtree_del() + qtree_new().
 * Call it once or on resize.
 */
void qtree_arena_init(int capacity);
void qtree_arena_reset(void);        // resets pool for reuse

typedef struct quadtree_t {
    node_t *root;
} quadtree_t;

void qtree_new(quadtree_t *qtree, rect_t boundary);
void qtree_insert(quadtree_t *qtree, point_t point);
void qtree_query(quadtree_t *qtree, rect_t search_area, int *count);
double qtree_nearest_neighbor(quadtree_t *qtree, point_t query, point_t *nearest);
void qtree_remove_point(quadtree_t *qtree, point_t *point);
void qtree_update_point(quadtree_t *qtree, point_t *old_point, point_t *new_point);
void qtree_merge(quadtree_t *qtree);
/**
 * Frees the entire arenaa - call once at shutdown.
 */
void qtree_del(quadtree_t *qtree);
extern void (*qtree_graph)(quadtree_t *);

#endif // QUAD_H
