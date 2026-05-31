#ifndef OCT_H
#define OCT_H

#include "stdbool.h"
#include <stddef.h>

enum {
    OCT_IND_NWF = 0,
    OCT_IND_NEF,
    OCT_IND_SEF,
    OCT_IND_SWF,
    OCT_IND_NWB,
    OCT_IND_NEB,
    OCT_IND_SEB,
    OCT_IND_SWB
};

#ifndef NODE_CAPACITY
#define NODE_CAPACITY 4
#endif

typedef struct point3_t {
    int x;
    int y;
    int z;
    size_t id;
} point3_t;

typedef struct cuboid_t {
    int x0, y0, z0;
    int x1, y1, z1;
} cuboid_t;

typedef struct oct_node_t {
    cuboid_t boundary;
    point3_t points[NODE_CAPACITY];
    int count;
    bool is_leaf;
    union {
        struct {
            struct oct_node_t *nwf;
            struct oct_node_t *nef;
            struct oct_node_t *sef;
            struct oct_node_t *swf;
            struct oct_node_t *nwb;
            struct oct_node_t *neb;
            struct oct_node_t *seb;
            struct oct_node_t *swb;
        };
        struct oct_node_t *children[8];
    };
} oct_node_t;

typedef struct octree_t {
    oct_node_t *root;
} octree_t;

void octree_arena_init(int capacity);
void octree_arena_reset(void);

void octree_new(octree_t *octree, cuboid_t boundary);
void octree_insert(octree_t *octree, point3_t point);
void octree_query(octree_t *octree, cuboid_t search_area, int *count);
double octree_nearest_neighbor(octree_t *octree, point3_t query, point3_t *nearest);
void octree_remove_point(octree_t *octree, point3_t *point);
void octree_update_point(octree_t *octree, point3_t *old_point, point3_t *new_point);
void octree_merge(octree_t *octree);
void octree_del(octree_t *octree);

#endif // OCT_H