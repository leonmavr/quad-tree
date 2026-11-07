#ifndef QUAD_H
#define QUAD_H

#include <stdbool.h>
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
    // Overflow chain: only used by leaves whose boundary is a single cell
    // (can't be split further) once they hold more than NODE_CAPACITY points.
    struct node_t *next;
    union {
        struct {
            struct node_t *nw;
            struct node_t *ne;
            struct node_t *se;
            struct node_t *sw;
        };
        struct node_t *children[4]; // NW, NE, SE, SW children
    };
} node_t;

/**
 * Per-tree node arena (internal - don't touch the fields). Nodes live in a
 * chain of blocks that only grows, so node pointers stay valid; nodes freed
 * by qtree_merge() go on a free list and are reused by later inserts.
 */
typedef struct qtree_block_t qtree_block_t;
typedef struct qtree_arena_t {
    qtree_block_t *head;    // first (smallest) block
    qtree_block_t *current; // block new nodes are carved from
    node_t *free_list;      // recycled nodes, linked through ->next
} qtree_arena_t;

typedef struct quadtree_t {
    node_t *root;
    qtree_arena_t arena;
} quadtree_t;

/**
 * Creates an empty tree that owns its own memory. Each tree is independent,
 * so several can exist at once. Pair every qtree_new() with a qtree_del().
 */
void qtree_new(quadtree_t *qtree, rect_t boundary);
/**
 * Removes all points but keeps the tree's memory and boundary - cheap way to
 * rebuild the tree every frame.
 */
void qtree_clear(quadtree_t *qtree);
/** Returns false (and stores nothing) if the point lies outside the root. */
bool qtree_insert(quadtree_t *qtree, point_t point);
void qtree_query(quadtree_t *qtree, rect_t search_area, int *count);
double qtree_nearest_neighbor(quadtree_t *qtree, point_t query, point_t *nearest);
/**
 * Removes the point with point->id. point->x/y must be the coordinates it was
 * inserted with, since they are used to find its leaf.
 * Returns false if no such point was found.
 */
bool qtree_remove_point(quadtree_t *qtree, const point_t *point);
/**
 * Moves old_point (located as in qtree_remove_point) to new_point.
 * Returns false and leaves the tree unchanged if old_point isn't found or
 * new_point lies outside the root. Neither argument is modified.
 */
bool qtree_update_point(quadtree_t *qtree, const point_t *old_point,
                        const point_t *new_point);
void qtree_merge(quadtree_t *qtree);
/** Frees all memory owned by the tree; qtree->root becomes NULL. */
void qtree_del(quadtree_t *qtree);
extern void (*qtree_graph)(quadtree_t *);

#endif // QUAD_H
