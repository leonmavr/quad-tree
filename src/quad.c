#include "quad.h"
#include <assert.h>
#include <float.h> // DBL_MAX
#include <stdbool.h>
#include <stddef.h> // size_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memcpy
#include <unistd.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// ---------------------------------------------------------------------------
// Arena allocator
// ---------------------------------------------------------------------------
static node_t *_node_pool    = NULL;
static int     _pool_count   = 0;
static int     _pool_capacity = 0;

void qtree_arena_init(int capacity) {
  free(_node_pool);
  _node_pool     = malloc(sizeof(node_t) * capacity);
  _pool_capacity = capacity;
  _pool_count    = 0;
}

void qtree_arena_reset(void) { _pool_count = 0; }

static node_t *node_new(rect_t *boundary) {
  assert(_node_pool && "call qtree_arena_init() before use");
  assert(_pool_count < _pool_capacity && 
         "node pool exhausted; call qtree_arena_init() with a larger capacity");
  node_t *node = &_node_pool[_pool_count++];
  node->boundary = *boundary;
  node->count    = 0;
  node->is_leaf  = true;
  node->nw = NULL;
  node->ne = NULL;
  node->se = NULL;
  node->sw = NULL;
  return node;
}

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------
static double distance_sq(point_t p1, point_t p2) {
  return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

static bool point_in_rect(point_t point, rect_t boundary) {
  return point.x >= boundary.x0 && point.x <= boundary.x1 &&
         point.y >= boundary.y0 && point.y <= boundary.y1;
}

static double point_rect_distsq(point_t p, rect_t rect) {
  /*
   * Why the formula max(x0 - x, x - x1, 0)^2 +
   * max(y0 - y, y - y1, 0)^2 works for the distance between a
   * point and a rectangle.
   *
   * Case 1 (next to)
   * ==========================================================
   * (x0, y0)                            x0 - x < 0, x - x1 > 0
   *  +-----------------+                y0 - y < 0. y - y1 < 0
   *  |                 |                d = dx = x - x1
   *  |                 |     * (x, y)
   *  |                 |<------>
   *  |                 |  dx
   *  +-----------------+
   *                 (x1, y1)
   *
   * Case 2 (above/below)
   * ==========================================================
   *             * (x, y)
   *             ^
   *          dy |
   *(x0, y0)     v                       x0 - x < 0, x - x1 < 0
   * +-----------------+                 y0 - y > 0. y - y1 < 0
   * |                 |                 d = dy = y0 - y
   * |                 |
   * |                 |
   * |                 |
   * +-----------------+
   *
   * Case 3 (diagonally)
   * ==========================================================
   * (x0, y0)                            x0 - x < 0, x - x1 > 0
   *  +-----------------+                y0 - y < 0. y - y1 > 0
   *  |                 |                d^2 = dx^2 + dy^2
   *  |                 |                    = (x-x1)^2 + (y-y1)^2
   *  |                 |
   *  |                 |   dx
   *  +-----------------+<-------->
   *                 (x1, y1)     ^
   *                              | dy
   *                              v
   *                              *
   */
  int dx = MAX(MAX(rect.x0 - p.x, 0), p.x - rect.x1);
  int dy = MAX(MAX(rect.y0 - p.y, 0), p.y - rect.y1);
  return dx * dx + dy * dy;
}

static void rect_divide(rect_t *src, rect_t *dest) {
  const int mid_x = (src->x0 + src->x1) / 2;
  const int mid_y = (src->y0 + src->y1) / 2;
  dest[IND_NW] = (rect_t){src->x0,    src->y0,    mid_x,    mid_y};
  dest[IND_NE] = (rect_t){mid_x + 1,  src->y0,    src->x1,  mid_y};
  dest[IND_SE] = (rect_t){mid_x + 1,  mid_y + 1,  src->x1,  src->y1};
  dest[IND_SW] = (rect_t){src->x0,    mid_y + 1,  mid_x,    src->y1};
}

static int point_get_quadrant(rect_t rect, point_t point) {
  const int mid_x = (rect.x0 + rect.x1) / 2;
  const int mid_y = (rect.y0 + rect.y1) / 2;
  if (point.x <= mid_x && point.y <= mid_y) return IND_NW;
  if (point.x >  mid_x && point.y <= mid_y) return IND_NE;
  if (point.x >  mid_x && point.y >  mid_y) return IND_SE;
  return IND_SW;
}

static bool rect_intersect(rect_t *r1, rect_t *r2) {
  int left   = r1->x0 > r2->x0 ? r1->x0 : r2->x0;
  int right  = r1->x1 < r2->x1 ? r1->x1 : r2->x1;
  int top    = r1->y0 > r2->y0 ? r1->y0 : r2->y0;
  int bottom = r1->y1 < r2->y1 ? r1->y1 : r2->y1;
  return left <= right && top <= bottom;
}

// ---------------------------------------------------------------------------
// Tree operations
// ---------------------------------------------------------------------------
void qtree_new(quadtree_t *qtree, rect_t boundary) {
  qtree->root = node_new(&boundary);
}

static void node_insert(node_t *node, point_t point) {
  if (!point_in_rect(point, node->boundary))
    return;

  if (node->count < NODE_CAPACITY && node->is_leaf) {
    node->points[node->count++] = point;
    return;
  }

  // Subdivide if still a leaf
  if (node->is_leaf) {
    rect_t subrects[4];
    rect_divide(&node->boundary, subrects);
    for (int q = 0; q < 4; ++q)
      node->children[q] = node_new(&subrects[q]);
    node->is_leaf = false;

    // Redistribute existing points into children
    for (int i = 0; i < node->count; ++i) {
      int q = point_get_quadrant(node->boundary, node->points[i]);
      node_insert(node->children[q], node->points[i]);
    }
    node->count = 0;
  }

  // Descend into the correct child
  int q = point_get_quadrant(node->boundary, point);
  node_insert(node->children[q], point);
}

void qtree_insert(quadtree_t *qtree, point_t point) {
  node_insert(qtree->root, point);
}

static void node_query(node_t *node, rect_t search_area, int *count) {
  if (!rect_intersect(&node->boundary, &search_area))
    return;
  if (node->is_leaf) {
    for (int i = 0; i < node->count; ++i)
      if (point_in_rect(node->points[i], search_area))
        (*count)++;
  } else {
    for (int q = 0; q < 4; ++q)
      node_query(node->children[q], search_area, count);
  }
}

void qtree_query(quadtree_t *qtree, rect_t search_area, int *count) {
  node_query(qtree->root, search_area, count);
}

static void node_nearest_neighbor(node_t *node, point_t query, point_t *nearest,
                                  double *best_dist_squared) {
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
    int q = point_get_quadrant(node->boundary, query);
    // Search the home quadrant first to get a good bound early
    node_nearest_neighbor(node->children[q], query, nearest, best_dist_squared);
    // Then prune and search the rest
    for (int i = 0; i < 4; ++i) {
      if (i == q) continue;
      double dist_to_region = point_rect_distsq(query, node->children[i]->boundary);
      if (dist_to_region < *best_dist_squared)
        node_nearest_neighbor(node->children[i], query, nearest, best_dist_squared);
    }
  }
}

double qtree_nearest_neighbor(quadtree_t *qtree, point_t query, point_t *nearest) {
  double best_dist_squared = DBL_MAX;
  node_nearest_neighbor(qtree->root, query, nearest, &best_dist_squared);
  return best_dist_squared;
}

static void node_remove_point(node_t *node, point_t *point) {
  if (node->is_leaf) {
    for (int i = 0; i < node->count; ++i) {
      if (node->points[i].id == point->id) {
        node->points[i] = node->points[--node->count];
        return;
      }
    }
  } else {
    int q = point_get_quadrant(node->boundary, *point);
    node_remove_point(node->children[q], point);
  }
}

void qtree_remove_point(quadtree_t *qtree, point_t *point) {
  node_remove_point(qtree->root, point);
}

void qtree_update_point(quadtree_t *qtree, point_t *old_point, point_t *new_point) {
  node_remove_point(qtree->root, old_point);
  old_point->x  = new_point->x;
  old_point->y  = new_point->y;
  old_point->id = new_point->id;
  node_insert(qtree->root, *old_point);
}

static void node_merge(node_t *node) {
  if (node->is_leaf)
    return;

  bool all_leaves = true;
  for (int q = 0; q < 4; ++q) {
    if (!node->children[q]->is_leaf) {
      node_merge(node->children[q]);
      all_leaves = false;
    }
  }

  if (all_leaves) {
    size_t point_count = 0;
    for (int q = 0; q < 4; ++q)
      point_count += node->children[q]->count;

    if (point_count <= NODE_CAPACITY) {
      node->count = 0;
      for (int q = 0; q < 4; ++q) {
        node_t *child = node->children[q];
        for (int j = 0; j < child->count; ++j)
          node->points[node->count++] = child->points[j];
        // It was arena allocated so no need to call free()
        node->children[q] = NULL;
      }
      node->is_leaf = true;
    }
  }
}

void qtree_merge(quadtree_t *qtree) { node_merge(qtree->root); }

// qtree_del frees the pool. Call once at shutdown.
// For per-frame reuse, call qtree_arena_reset() instead.
void qtree_del(quadtree_t *qtree) {
  free(_node_pool);
  _node_pool     = NULL;
  _pool_count    = 0;
  _pool_capacity = 0;
}
