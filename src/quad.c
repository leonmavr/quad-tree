#include "quad.h"
#include <float.h> // DBL_MAX
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// ---------------------------------------------------------------------------
// Arena allocator
// ---------------------------------------------------------------------------
#define ARENA_FIRST_BLOCK 64 // nodes; each new block doubles the previous

struct qtree_block_t {
  qtree_block_t *next;
  int capacity;
  int used;
  node_t nodes[];
};

static qtree_block_t *block_new(int capacity) {
  qtree_block_t *block = malloc(sizeof(*block) + sizeof(node_t) * (size_t)capacity);
  if (!block) {
    fprintf(stderr, "quadtree: out of memory allocating %d nodes\n", capacity);
    abort();
  }
  block->next     = NULL;
  block->capacity = capacity;
  block->used     = 0;
  return block;
}

static void arena_init(qtree_arena_t *arena) {
  arena->head       = block_new(ARENA_FIRST_BLOCK);
  arena->current    = arena->head;
  arena->free_nodes = NULL;
  arena->free_quads = NULL;
}

// Forget all nodes but keep the blocks for reuse.
static void arena_reset(qtree_arena_t *arena) {
  for (qtree_block_t *b = arena->head; b; b = b->next)
    b->used = 0;
  arena->current    = arena->head;
  arena->free_nodes = NULL;
  arena->free_quads = NULL;
}

static void arena_free(qtree_arena_t *arena) {
  qtree_block_t *b = arena->head;
  while (b) {
    qtree_block_t *next = b->next;
    free(b);
    b = next;
  }
  arena->head = arena->current = NULL;
  arena->free_nodes = arena->free_quads = NULL;
}

// Takes n contiguous nodes from the current block, moving on to the next
// block if there isn't room (wasting at most n - 1 nodes at a block's end).
static node_t *arena_carve(qtree_arena_t *arena, int n) {
  qtree_block_t *b = arena->current;
  while (b->capacity - b->used < n) {
    if (!b->next) // blocks kept from a qtree_clear() are reused first
      b->next = block_new(b->capacity * 2);
    b = arena->current = b->next;
  }
  node_t *nodes = &b->nodes[b->used];
  b->used += n;
  return nodes;
}

static node_t *arena_alloc_node(qtree_arena_t *arena) {
  node_t *node = arena->free_nodes;
  if (!node)
    return arena_carve(arena, 1);
  arena->free_nodes = node->next;
  return node;
}

// 4 contiguous nodes, so a parent needs one pointer to reach all children.
static node_t *arena_alloc_quad(qtree_arena_t *arena) {
  node_t *quad = arena->free_quads;
  if (!quad)
    return arena_carve(arena, 4);
  arena->free_quads = quad->next;
  return quad;
}

// Returns a chain of single nodes (linked through ->next) to the free list.
static void arena_release_chain(qtree_arena_t *arena, node_t *node) {
  while (node) {
    node_t *next = node->next;
    node->next = arena->free_nodes;
    arena->free_nodes = node;
    node = next;
  }
}

// Returns 4 leaf children, and any overflow chains they own, to the arena.
static void arena_release_quad(qtree_arena_t *arena, node_t *quad) {
  for (int q = 0; q < 4; ++q)
    arena_release_chain(arena, quad[q].next);
  quad->next = arena->free_quads;
  arena->free_quads = quad;
}

static void node_init(node_t *node, rect_t boundary) {
  node->boundary = boundary;
  node->count    = 0;
  node->is_leaf  = true;
  node->next     = NULL;
}

static node_t *node_new(qtree_arena_t *arena, rect_t boundary) {
  node_t *node = arena_alloc_node(arena);
  node_init(node, boundary);
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
static bool rect_can_split(rect_t r) {
  return r.x0 < r.x1 || r.y0 < r.y1;
}

// Child q covers the east half if bit 0 of q is set, the south half if bit 1
// is set (see the IND_* enum).
static void rect_divide(rect_t src, rect_t dest[4]) {
  const int mid_x = midpoint(src.x0, src.x1);
  const int mid_y = midpoint(src.y0, src.y1);
  for (int q = 0; q < 4; ++q) {
    const bool east = q & 1, south = q & 2;
    dest[q] = (rect_t){east  ? mid_x + 1 : src.x0, south ? mid_y + 1 : src.y0,
                       east  ? src.x1    : mid_x,  south ? src.y1    : mid_y};
  }
}

static int point_get_quadrant(rect_t rect, point_t point) {
  const int mid_x = midpoint(rect.x0, rect.x1);
  const int mid_y = midpoint(rect.y0, rect.y1);
  // YX bits -> Y: is below y-middle, X: is below x-middle
  return (point.x > mid_x) | ((point.y > mid_y) << 1);
}

static bool rect_intersect(rect_t r1, rect_t r2) {
  return MAX(r1.x0, r2.x0) <= MIN(r1.x1, r2.x1) &&
         MAX(r1.y0, r2.y0) <= MIN(r1.y1, r2.y1);
}

// ---------------------------------------------------------------------------
// Tree operations
// ---------------------------------------------------------------------------
void qtree_new_checked(quadtree_t *qtree, rect_t boundary, size_t node_size) {
  if (node_size != sizeof(node_t)) {
    fprintf(stderr, "quadtree: node_t size mismatch (caller %zu, library %zu) - "
                    "compile everything with the same NODE_CAPACITY\n",
            node_size, sizeof(node_t));
    abort();
  }
  arena_init(&qtree->arena);
  qtree->root = node_new(&qtree->arena, boundary);
}

void qtree_clear(quadtree_t *qtree) {
  const rect_t boundary = qtree->root->boundary;
  arena_reset(&qtree->arena);
  qtree->root = node_new(&qtree->arena, boundary);
}

// Appends to the first non-full node of a leaf's overflow chain, growing the
// chain if needed. For a leaf with room this is just a plain append.
static void leaf_append(qtree_arena_t *arena, node_t *leaf, point_t point) {
  while (leaf->count == NODE_CAPACITY) {
    if (!leaf->next)
      leaf->next = node_new(arena, leaf->boundary);
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

static bool node_insert(qtree_arena_t *arena, node_t *node, point_t point) {
  if (!point_in_rect(point, node->boundary))
    return false;

  if (node->is_leaf &&
      (node->count < NODE_CAPACITY || !rect_can_split(node->boundary))) {
    leaf_append(arena, node, point);
    return true;
  }

  // Subdivide if still a leaf
  if (node->is_leaf) {
    // points and children share storage: save the points before linking
    point_t points[NODE_CAPACITY];
    const int count = node->count;
    for (int i = 0; i < count; ++i)
      points[i] = node->points[i];

    rect_t subrects[4];
    rect_divide(node->boundary, subrects);
    node_t *children = arena_alloc_quad(arena);
    for (int q = 0; q < 4; ++q)
      node_init(&children[q], subrects[q]);
    node->children = children;
    node->is_leaf  = false;
    node->count    = 0;

    // Redistribute existing points into children
    for (int i = 0; i < count; ++i)
      node_insert(arena, &children[point_get_quadrant(node->boundary, points[i])],
                  points[i]);
  }

  // Descend into the correct child
  int q = point_get_quadrant(node->boundary, point);
  return node_insert(arena, &node->children[q], point);
}

bool qtree_insert(quadtree_t *qtree, point_t point) {
  return node_insert(&qtree->arena, qtree->root, point);
}

static int node_query(const node_t *node, rect_t search_area) {
  if (!rect_intersect(node->boundary, search_area))
    return 0;
  int count = 0;
  if (node->is_leaf) {
    for (const node_t *n = node; n; n = n->next)
      for (int i = 0; i < n->count; ++i)
        count += point_in_rect(n->points[i], search_area);
  } else {
    for (int q = 0; q < 4; ++q)
      count += node_query(&node->children[q], search_area);
  }
  return count;
}

int qtree_query(const quadtree_t *qtree, rect_t search_area) {
  return node_query(qtree->root, search_area);
}

static void node_nearest_neighbor(const node_t *node, point_t query,
                                  point_t *nearest, double *best_dist_squared) {
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
    node_nearest_neighbor(&node->children[q], query, nearest, best_dist_squared);
    // Then prune and search the rest
    for (int i = 0; i < 4; ++i) {
      if (i == q) continue;
      double dist_to_region = point_rect_distsq(query, node->children[i].boundary);
      if (dist_to_region < *best_dist_squared)
        node_nearest_neighbor(&node->children[i], query, nearest, best_dist_squared);
    }
  }
}

double qtree_nearest_neighbor(const quadtree_t *qtree, point_t query, point_t *nearest) {
  double best_dist_squared = DBL_MAX;
  node_nearest_neighbor(qtree->root, query, nearest, &best_dist_squared);
  return best_dist_squared;
}

// Descends by the point's coordinates, then matches by id within the leaf.
static bool node_remove_point(node_t *node, point_t point) {
  while (!node->is_leaf)
    node = &node->children[point_get_quadrant(node->boundary, point)];

  for (node_t *n = node; n; n = n->next) {
    for (int i = 0; i < n->count; ++i) {
      if (n->points[i].id == point.id) {
        n->points[i] = n->points[--n->count];
        return true;
      }
    }
  }
  return false;
}

bool qtree_remove_point(quadtree_t *qtree, point_t point) {
  if (!point_in_rect(point, qtree->root->boundary))
    return false;
  return node_remove_point(qtree->root, point);
}

bool qtree_update_point(quadtree_t *qtree, point_t old_point, point_t new_point) {
  // Check the destination first so a failed update leaves the tree unchanged.
  if (!point_in_rect(new_point, qtree->root->boundary))
    return false;
  if (!qtree_remove_point(qtree, old_point))
    return false;
  return node_insert(&qtree->arena, qtree->root, new_point); // always true: bounds checked
}

static void node_merge(qtree_arena_t *arena, node_t *node) {
  if (node->is_leaf)
    return;

  // Merge bottom-up: collapse the children first, so a whole sparse subtree
  // folds into this node in a single pass.
  node_t *children = node->children;
  bool all_leaves = true;
  for (int q = 0; q < 4; ++q) {
    node_merge(arena, &children[q]);
    all_leaves &= children[q].is_leaf;
  }
  if (!all_leaves)
    return;

  size_t point_count = 0;
  for (int q = 0; q < 4; ++q)
    point_count += leaf_count(&children[q]);
  if (point_count > NODE_CAPACITY)
    return;

  // Writing points overwrites node->children (shared storage); we hold
  // the children in a local.
  node->is_leaf = true;
  node->count   = 0;
  for (int q = 0; q < 4; ++q)
    for (const node_t *n = &children[q]; n; n = n->next)
      for (int j = 0; j < n->count; ++j)
        node->points[node->count++] = n->points[j];
  arena_release_quad(arena, children);
}

void qtree_merge(quadtree_t *qtree) { node_merge(&qtree->arena, qtree->root); }

void qtree_del(quadtree_t *qtree) {
  arena_free(&qtree->arena);
  qtree->root = NULL;
}
