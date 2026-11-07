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
  node->next     = NULL;
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
  // Compute in double: int differences/products overflow for large coords.
  const double dx = (double)p1.x - (double)p2.x;
  const double dy = (double)p1.y - (double)p2.y;
  return dx * dx + dy * dy;
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
  const double dx = MAX(MAX((double)rect.x0 - p.x, 0.0), (double)p.x - rect.x1);
  const double dy = MAX(MAX((double)rect.y0 - p.y, 0.0), (double)p.y - rect.y1);
  return dx * dx + dy * dy;
}

// Floor midpoint of [lo, hi] (lo <= hi). Unlike (lo + hi) / 2 it doesn't
// round toward zero for negative values
static int midpoint(int lo, int hi) {
  return lo + (int)(((long long)hi - lo) / 2);
}

// A rect can only be split if it is wider or taller than one cell.
static bool rect_can_split(const rect_t *r) {
  return r->x0 < r->x1 || r->y0 < r->y1;
}

static void rect_divide(rect_t *src, rect_t *dest) {
  const int mid_x = midpoint(src->x0, src->x1);
  const int mid_y = midpoint(src->y0, src->y1);
  dest[IND_NW] = (rect_t){src->x0,    src->y0,    mid_x,    mid_y};
  dest[IND_NE] = (rect_t){mid_x + 1,  src->y0,    src->x1,  mid_y};
  dest[IND_SE] = (rect_t){mid_x + 1,  mid_y + 1,  src->x1,  src->y1};
  dest[IND_SW] = (rect_t){src->x0,    mid_y + 1,  mid_x,    src->y1};
}

static int point_get_quadrant(rect_t rect, point_t point) {
  const int mid_x = midpoint(rect.x0, rect.x1);
  const int mid_y = midpoint(rect.y0, rect.y1);
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

// Appends to the first non-full node of a leaf's overflow chain, growing the
// chain if needed. For a leaf with room this is just a plain append.
static void leaf_append(node_t *leaf, point_t point) {
  while (leaf->count == NODE_CAPACITY) {
    if (!leaf->next)
      leaf->next = node_new(&leaf->boundary);
    leaf = leaf->next;
  }
  leaf->points[leaf->count++] = point;
}

// Total points held by a leaf, including its overflow chain.
static int leaf_count(const node_t *leaf) {
  int n = 0;
  for (; leaf; leaf = leaf->next)
    n += leaf->count;
  return n;
}

static void node_insert(node_t *node, point_t point) {
  if (!point_in_rect(point, node->boundary))
    return;

  if (node->is_leaf &&
      (node->count < NODE_CAPACITY || !rect_can_split(&node->boundary))) {
    leaf_append(node, point);
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
    for (const node_t *n = node; n; n = n->next)
      for (int i = 0; i < n->count; ++i)
        if (point_in_rect(n->points[i], search_area))
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
    for (const node_t *n = node; n; n = n->next) {
      for (int i = 0; i < n->count; ++i) {
        double dist = distance_sq(query, n->points[i]);
        if (dist < *best_dist_squared) {
          *best_dist_squared = dist;
          *nearest = n->points[i];
        }
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
    for (node_t *n = node; n; n = n->next) {
      for (int i = 0; i < n->count; ++i) {
        if (n->points[i].id == point->id) {
          n->points[i] = n->points[--n->count];
          return;
        }
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
      point_count += leaf_count(node->children[q]);

    if (point_count <= NODE_CAPACITY) {
      node->count = 0;
      for (int q = 0; q < 4; ++q) {
        for (node_t *n = node->children[q]; n; n = n->next)
          for (int j = 0; j < n->count; ++j)
            node->points[node->count++] = n->points[j];
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
