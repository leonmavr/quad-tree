#ifndef QUAD_H
#define QUAD_H

#include <stdbool.h>
#include <stddef.h> // size_t

// Child index bits: bit 0 = east (x > mid_x), bit 1 = south (y > mid_y),
// with y growing southwards. So index = (x > mid_x) | (y > mid_y) << 1.
enum {
    IND_NW = 0,
    IND_NE = 1,
    IND_SW = 2,
    IND_SE = 3
};

// Maximum number of points per leaf node. It changes the size of node_t, so
// the library and everything including this header must use the same value
// (qtree_new() aborts with a message if they don't).
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
    int count;   // points stored in this node (leaves only)
    bool is_leaf;
    // Overflow chain: only used by leaves whose boundary is a single cell
    // (can't be split further) once they hold more than NODE_CAPACITY points.
    struct node_t *next;
    // A node is either a leaf or internal, never both, so they share storage.
    union {
        point_t points[NODE_CAPACITY]; // leaf
        struct node_t *children;       // internal: 4 contiguous nodes,
                                       // children[IND_NW] .. children[IND_SE]
    };
} node_t;

/**
 * Per-tree node arena (internal - don't touch the fields). Nodes live in a
 * chain of blocks that only grows, so node pointers stay valid; nodes freed
 * by qtree_merge() go on free lists and are reused by later inserts.
 */
typedef struct qtree_block_t qtree_block_t;
typedef struct qtree_arena_t {
    qtree_block_t *head;    // first (smallest) block
    qtree_block_t *current; // block new nodes are carved from
    node_t *free_nodes;     // recycled single nodes, linked through ->next
    node_t *free_quads;     // recycled groups of 4 children, linked through
                            // the first node's ->next
} qtree_arena_t;

typedef struct quadtree_t {
    node_t *root;
    qtree_arena_t arena;
} quadtree_t;

/**
 * Creates an empty tree that owns its own memory. Each tree is independent,
 * so several can exist at once. Pair every qtree_new() with a qtree_del().
 * (A macro so the library can check the caller's NODE_CAPACITY matches.)
 */
// Variadic so a compound literal boundary like (rect_t){0, 0, 9, 9} works.
#define qtree_new(qtree, ...) \
    qtree_new_checked((qtree), (__VA_ARGS__), sizeof(node_t))
void qtree_new_checked(quadtree_t *qtree, rect_t boundary, size_t node_size);
/**
 * Removes all points but keeps the tree's memory and boundary - cheap way to
 * rebuild the tree every frame.
 */
void qtree_clear(quadtree_t *qtree);
/** Returns false (and stores nothing) if the point lies outside the root. */
bool qtree_insert(quadtree_t *qtree, point_t point);
/** Returns the number of points inside search_area (edges inclusive). */
int qtree_query(const quadtree_t *qtree, rect_t search_area);
/**
 * Finds the point closest to query and returns the squared distance to it.
 * A point equal to query (e.g. the query itself, if it's in the tree) counts.
 * On an empty tree returns DBL_MAX and leaves *nearest unchanged.
 */
double qtree_nearest_neighbor(const quadtree_t *qtree, point_t query,
                              point_t *nearest);
/**
 * Removes the point with point.id. point.x/y must be the coordinates it was
 * inserted with, since they are used to find its leaf.
 * Returns false if no such point was found.
 */
bool qtree_remove_point(quadtree_t *qtree, point_t point);
/**
 * Moves old_point (located as in qtree_remove_point) to new_point.
 * Returns false and leaves the tree unchanged if old_point isn't found or
 * new_point lies outside the root.
 */
bool qtree_update_point(quadtree_t *qtree, point_t old_point, point_t new_point);
void qtree_merge(quadtree_t *qtree);
/** Frees all memory owned by the tree; qtree->root becomes NULL. */
void qtree_del(quadtree_t *qtree);

#endif // QUAD_H
