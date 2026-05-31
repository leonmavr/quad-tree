#include "oct.h"
#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct oct_node_block_t {
  oct_node_t *nodes;
  int capacity;
  int used;
  struct oct_node_block_t *next;
} oct_node_block_t;

static oct_node_block_t *_oct_node_blocks = NULL;
static oct_node_block_t *_oct_current_block = NULL;

static oct_node_block_t *arena_block_new(int capacity) {
  oct_node_block_t *block = malloc(sizeof(*block));
  if (block == NULL) {
    fprintf(stderr, "failed to allocate octree arena metadata\n");
    exit(EXIT_FAILURE);
  }

  block->nodes = malloc(sizeof(oct_node_t) * (size_t)capacity);
  if (block->nodes == NULL) {
    fprintf(stderr, "failed to allocate octree arena block for %d nodes\n", capacity);
    free(block);
    exit(EXIT_FAILURE);
  }

  block->capacity = capacity;
  block->used = 0;
  block->next = NULL;
  return block;
}

static void arena_blocks_free(void) {
  oct_node_block_t *block = _oct_node_blocks;
  while (block != NULL) {
    oct_node_block_t *next = block->next;
    free(block->nodes);
    free(block);
    block = next;
  }

  _oct_node_blocks = NULL;
  _oct_current_block = NULL;
}

void octree_arena_init(int capacity) {
  const int initial_capacity = MAX(capacity, 1);
  arena_blocks_free();
  _oct_node_blocks = arena_block_new(initial_capacity);
  _oct_current_block = _oct_node_blocks;
}

void octree_arena_reset(void) {
  for (oct_node_block_t *block = _oct_node_blocks; block != NULL; block = block->next)
    block->used = 0;
  _oct_current_block = _oct_node_blocks;
}

static oct_node_t *node_new(cuboid_t *boundary) {
  assert(_oct_current_block && "call octree_arena_init() before use");
  if (_oct_current_block->used == _oct_current_block->capacity) {
    if (_oct_current_block->next == NULL) {
      int next_capacity = MAX(_oct_current_block->capacity * 2, 1);
      _oct_current_block->next = arena_block_new(next_capacity);
    }
    _oct_current_block = _oct_current_block->next;
  }

  oct_node_t *node = &_oct_current_block->nodes[_oct_current_block->used++];
  node->boundary = *boundary;
  node->count = 0;
  node->is_leaf = true;
  for (int i = 0; i < 8; ++i)
    node->children[i] = NULL;
  return node;
}

static double distance_sq(point3_t p1, point3_t p2) {
  double dx = (double)p1.x - (double)p2.x;
  double dy = (double)p1.y - (double)p2.y;
  double dz = (double)p1.z - (double)p2.z;
  return dx * dx + dy * dy + dz * dz;
}

static bool point_in_cuboid(point3_t point, cuboid_t boundary) {
  return point.x >= boundary.x0 && point.x <= boundary.x1 &&
         point.y >= boundary.y0 && point.y <= boundary.y1 &&
         point.z >= boundary.z0 && point.z <= boundary.z1;
}

static double point_cuboid_distsq(point3_t p, cuboid_t cuboid) {
  int dx = MAX(MAX(cuboid.x0 - p.x, 0), p.x - cuboid.x1);
  int dy = MAX(MAX(cuboid.y0 - p.y, 0), p.y - cuboid.y1);
  int dz = MAX(MAX(cuboid.z0 - p.z, 0), p.z - cuboid.z1);
  return (double)dx * dx + (double)dy * dy + (double)dz * dz;
}

static void cuboid_divide(cuboid_t *src, cuboid_t *dest) {
  const int mid_x = (src->x0 + src->x1) / 2;
  const int mid_y = (src->y0 + src->y1) / 2;
  const int mid_z = (src->z0 + src->z1) / 2;

  dest[OCT_IND_NWF] = (cuboid_t){src->x0,   src->y0,   src->z0,
                                 mid_x,     mid_y,     mid_z};
  dest[OCT_IND_NEF] = (cuboid_t){mid_x + 1, src->y0,   src->z0,
                                 src->x1,   mid_y,     mid_z};
  dest[OCT_IND_SEF] = (cuboid_t){mid_x + 1, mid_y + 1, src->z0,
                                 src->x1,   src->y1,   mid_z};
  dest[OCT_IND_SWF] = (cuboid_t){src->x0,   mid_y + 1, src->z0,
                                 mid_x,     src->y1,   mid_z};
  dest[OCT_IND_NWB] = (cuboid_t){src->x0,   src->y0,   mid_z + 1,
                                 mid_x,     mid_y,     src->z1};
  dest[OCT_IND_NEB] = (cuboid_t){mid_x + 1, src->y0,   mid_z + 1,
                                 src->x1,   mid_y,     src->z1};
  dest[OCT_IND_SEB] = (cuboid_t){mid_x + 1, mid_y + 1, mid_z + 1,
                                 src->x1,   src->y1,   src->z1};
  dest[OCT_IND_SWB] = (cuboid_t){src->x0,   mid_y + 1, mid_z + 1,
                                 mid_x,     src->y1,   src->z1};
}

static int point_get_octant(cuboid_t cuboid, point3_t point) {
  const int mid_x = (cuboid.x0 + cuboid.x1) / 2;
  const int mid_y = (cuboid.y0 + cuboid.y1) / 2;
  const int mid_z = (cuboid.z0 + cuboid.z1) / 2;

  if (point.z <= mid_z) {
    if (point.x <= mid_x && point.y <= mid_y) return OCT_IND_NWF;
    if (point.x > mid_x && point.y <= mid_y) return OCT_IND_NEF;
    if (point.x > mid_x && point.y > mid_y) return OCT_IND_SEF;
    return OCT_IND_SWF;
  }

  if (point.x <= mid_x && point.y <= mid_y) return OCT_IND_NWB;
  if (point.x > mid_x && point.y <= mid_y) return OCT_IND_NEB;
  if (point.x > mid_x && point.y > mid_y) return OCT_IND_SEB;
  return OCT_IND_SWB;
}

static bool cuboid_intersect(cuboid_t *c1, cuboid_t *c2) {
  int left = c1->x0 > c2->x0 ? c1->x0 : c2->x0;
  int right = c1->x1 < c2->x1 ? c1->x1 : c2->x1;
  int top = c1->y0 > c2->y0 ? c1->y0 : c2->y0;
  int bottom = c1->y1 < c2->y1 ? c1->y1 : c2->y1;
  int front = c1->z0 > c2->z0 ? c1->z0 : c2->z0;
  int back = c1->z1 < c2->z1 ? c1->z1 : c2->z1;
  return left <= right && top <= bottom && front <= back;
}

void octree_new(octree_t *octree, cuboid_t boundary) {
  octree->root = node_new(&boundary);
}

static void node_insert(oct_node_t *node, point3_t point) {
  if (!point_in_cuboid(point, node->boundary))
    return;

  if (node->count < NODE_CAPACITY && node->is_leaf) {
    node->points[node->count++] = point;
    return;
  }

  if (node->is_leaf) {
    cuboid_t subcuboids[8];
    cuboid_divide(&node->boundary, subcuboids);
    for (int octant = 0; octant < 8; ++octant)
      node->children[octant] = node_new(&subcuboids[octant]);
    node->is_leaf = false;

    for (int i = 0; i < node->count; ++i) {
      int octant = point_get_octant(node->boundary, node->points[i]);
      node_insert(node->children[octant], node->points[i]);
    }
    node->count = 0;
  }

  int octant = point_get_octant(node->boundary, point);
  node_insert(node->children[octant], point);
}

void octree_insert(octree_t *octree, point3_t point) {
  node_insert(octree->root, point);
}

static void node_query(oct_node_t *node, cuboid_t search_area, int *count) {
  if (!cuboid_intersect(&node->boundary, &search_area))
    return;

  if (node->is_leaf) {
    for (int i = 0; i < node->count; ++i)
      if (point_in_cuboid(node->points[i], search_area))
        (*count)++;
  } else {
    for (int octant = 0; octant < 8; ++octant)
      node_query(node->children[octant], search_area, count);
  }
}

void octree_query(octree_t *octree, cuboid_t search_area, int *count) {
  node_query(octree->root, search_area, count);
}

static void node_nearest_neighbor(oct_node_t *node, point3_t query,
                                  point3_t *nearest, double *best_dist_squared) {
  if (!node)
    return;

  if (node->is_leaf) {
    for (int i = 0; i < node->count; ++i) {
      double dist = distance_sq(query, node->points[i]);
      if (dist < *best_dist_squared) {
        *best_dist_squared = dist;
        *nearest = node->points[i];
      }
    }
  } else {
    int octant = point_get_octant(node->boundary, query);
    node_nearest_neighbor(node->children[octant], query, nearest, best_dist_squared);
    for (int i = 0; i < 8; ++i) {
      if (i == octant)
        continue;

      double dist_to_region = point_cuboid_distsq(query, node->children[i]->boundary);
      if (dist_to_region < *best_dist_squared)
        node_nearest_neighbor(node->children[i], query, nearest, best_dist_squared);
    }
  }
}

double octree_nearest_neighbor(octree_t *octree, point3_t query, point3_t *nearest) {
  double best_dist_squared = DBL_MAX;
  node_nearest_neighbor(octree->root, query, nearest, &best_dist_squared);
  return best_dist_squared;
}

static void node_remove_point(oct_node_t *node, point3_t *point) {
  if (node->is_leaf) {
    for (int i = 0; i < node->count; ++i) {
      if (node->points[i].id == point->id) {
        node->points[i] = node->points[--node->count];
        return;
      }
    }
  } else {
    int octant = point_get_octant(node->boundary, *point);
    node_remove_point(node->children[octant], point);
  }
}

void octree_remove_point(octree_t *octree, point3_t *point) {
  node_remove_point(octree->root, point);
}

void octree_update_point(octree_t *octree, point3_t *old_point, point3_t *new_point) {
  node_remove_point(octree->root, old_point);
  old_point->x = new_point->x;
  old_point->y = new_point->y;
  old_point->z = new_point->z;
  old_point->id = new_point->id;
  node_insert(octree->root, *old_point);
}

static void node_merge(oct_node_t *node) {
  if (node->is_leaf)
    return;

  bool all_leaves = true;
  for (int octant = 0; octant < 8; ++octant) {
    if (!node->children[octant]->is_leaf) {
      node_merge(node->children[octant]);
      all_leaves = false;
    }
  }

  if (all_leaves) {
    size_t point_count = 0;
    for (int octant = 0; octant < 8; ++octant)
      point_count += node->children[octant]->count;

    if (point_count <= NODE_CAPACITY) {
      node->count = 0;
      for (int octant = 0; octant < 8; ++octant) {
        oct_node_t *child = node->children[octant];
        for (int i = 0; i < child->count; ++i)
          node->points[node->count++] = child->points[i];
        node->children[octant] = NULL;
      }
      node->is_leaf = true;
    }
  }
}

void octree_merge(octree_t *octree) {
  node_merge(octree->root);
}

void octree_del(octree_t *octree) {
  arena_blocks_free();
  if (octree != NULL)
    octree->root = NULL;
}