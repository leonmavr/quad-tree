#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h> // memcpy
#include <unistd.h>
#include <float.h> // DBL_MAX
#include <stddef.h> // size_t

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Maximum number of points per node
int node_capacity = 2;
// unique ID to assign to each point
static size_t _node_ids = 0;

static node_t* node_new(rect_t* boundary);
static bool node_is_leaf(node_t* node);
static void node_insert(node_t* node, point_t* point);
static void node_query(node_t* node, rect_t* search_area, int* count);
static void node_nearest_neighbor(node_t* node, point_t* query, point_t* nearest, double* best_dist_squared);
static void node_remove_point(node_t* node, point_t* point);
static void node_merge(node_t* node);
static void node_del_all(node_t* node);

static double distance_sq(point_t* p1, point_t* p2) {
    return (p1->x - p2->x) * (p1->x - p2->x) + (p1->y - p2->y) * (p1->y - p2->y);
}

static bool point_in_rect(point_t* point, rect_t* boundary) {
    return point->x >= boundary->x0 && point->x <= boundary->x1 &&
           point->y >= boundary->y0 && point->y <= boundary->y1;
}

static double point_rect_distsq(point_t* p, rect_t* rect) {
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
    *  |                 |<---->         
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
    *  +-----------------+<------>
    *                 (x1, y1)   ^
    *                            | dy
    *                            v
    */
    int dx = MAX(MAX(rect->x0 - p->x, 0), p->x - rect->x1);
    int dy = MAX(MAX(rect->y0 - p->y, 0), p->y - rect->y1);
    return dx * dx + dy * dy;
}

static void rect_divide(rect_t* src, rect_t* dest) {
    const int mid_x = (src->x0 + src->x1) / 2;
    const int mid_y = (src->y0 + src->y1) / 2;
    dest[IND_NW] = (rect_t) {src->x0 , src->y0 , mid_x   , mid_y  };
    dest[IND_NE] = (rect_t) {mid_x+1 , src->y0 , src->x1 , mid_y };
    dest[IND_SE] = (rect_t) {mid_x+1 , mid_y+1 , src->x1 , src->y1};
    dest[IND_SW] = (rect_t) {src->x0 , mid_y+1 , mid_x , src->y1};
}

static int point_get_quadrant(rect_t* rect, point_t* point) {
    const int mid_x = (rect->x0 + rect->x1) / 2;
    const int mid_y = (rect->y0 + rect->y1) / 2;
    if (point->x <= mid_x && point->y <= mid_y) return IND_NW;
    else if (point->x > mid_x && point->y <= mid_y) return IND_NE;
    else if (point->x > mid_x && point->y > mid_y) return IND_SE;
    else return IND_SW;
}

static bool rect_intersect(rect_t* r1, rect_t* r2) {
    // max of the left edges and min of the right edges
    int left   = r1->x0 > r2->x0 ? r1->x0 : r2->x0;
    int right  = r1->x1 < r2->x1 ? r1->x1 : r2->x1;
    // max of the top edges and min of the bottom edges
    int top    = r1->y0 > r2->y0 ? r1->y0 : r2->y0;
    int bottom = r1->y1 < r2->y1 ? r1->y1 : r2->y1;
    return left <= right && top <= bottom;
}

node_t* node_new(rect_t* boundary) {
    node_t *node = malloc(sizeof(node_t));
    node->boundary = *boundary;
    node->count = 0;
    node->points = malloc(node_capacity * sizeof(point_t));
    for (int i = 0; i < node_capacity; ++i)
        node->points[i].id = _node_ids++;
    node->nw = NULL;
    node->ne = NULL;
    node->sw = NULL;
    node->se = NULL;
    return node;
}

void qtree_new(quadtree_t* qtree, rect_t* boundary) {
    qtree->root = node_new(boundary);
}
static bool node_is_leaf(node_t* node) {
    return (node != NULL) ? (node->nw == NULL && node->ne == NULL &&
                            node->se == NULL && node->sw == NULL) : false;
}

static void node_insert(node_t* node, point_t* point) {
    if (!point_in_rect(point, &node->boundary))
        return;
    if (node->count < node_capacity && node_is_leaf(node)) {
        node->points[node->count++] = *point;
    } else {
        // Divide the current node into four sub-regions if it's a leaf and not yet divided
        if (node_is_leaf(node)) {
            rect_t subrects[4];
            rect_divide(&node->boundary, subrects);
            node->nw = node_new(&subrects[IND_NW]);
            node->ne = node_new(&subrects[IND_NE]);
            node->se = node_new(&subrects[IND_SE]);
            node->sw = node_new(&subrects[IND_SW]);
            // Leaves were created so re-distributes points into the leaves (children) 
            for (int i = 0; i < node->count; ++i) {
                point_t existing_point = node->points[i];
                int quadrant = point_get_quadrant(&node->boundary, &existing_point);
                if (quadrant == IND_NW) node_insert(node->nw, &existing_point);
                else if (quadrant == IND_NE) node_insert(node->ne, &existing_point);
                else if (quadrant == IND_SE) node_insert(node->se, &existing_point);
                else if (quadrant == IND_SW) node_insert(node->sw, &existing_point);
            }
            // Parent node has moved its data to the children 
            node->count = 0;
        }
        // keep searching top-down
        int quadrant = point_get_quadrant(&node->boundary, point);
        if (quadrant == IND_NW) node_insert(node->nw, point);
        else if (quadrant == IND_NE) node_insert(node->ne, point);
        else if (quadrant == IND_SE) node_insert(node->se, point);
        else if (quadrant == IND_SW) node_insert(node->sw, point);
    }
}

void qtree_insert(quadtree_t* qtree, point_t* point) {
    node_insert(qtree->root, point);
}

static void node_query(node_t* node, rect_t* search_area, int* count) {
    if (!rect_intersect(&node->boundary, search_area))
        return;
    // If the node is a leaf and the boundary overlaps with the search area, count the points
    if (node_is_leaf(node)) {
        for (int i = 0; i < node->count; ++i) {
            if (point_in_rect(&node->points[i], search_area))
                (*count)++;
        }
    } else {
        node_query(node->nw, search_area, count);
        node_query(node->ne, search_area, count);
        node_query(node->sw, search_area, count);
        node_query(node->se, search_area, count);
    }
}

void qtree_query(quadtree_t* qtree, rect_t* search_area, int* count) {
    node_query(qtree->root, search_area, count);
}

static void node_nearest_neighbor(node_t* node, point_t* query, point_t* nearest, double* best_dist_squared) {
    if (!node) return;

    if (node_is_leaf(node)) {
        for (int i = 0; i < node->count; ++i) {
            double dist = distance_sq(query, &node->points[i]);
            if (dist < *best_dist_squared) {
                *best_dist_squared = dist;
                *nearest = node->points[i];
            }
        }
    } else {
        int quadrant = point_get_quadrant(&node->boundary, query);
        node_t* children[4] = {node->nw, node->ne, node->sw, node->se};
        // 1. Narrow down to the quadrant containing the query point first
        //    to prune as fast as possible
        node_nearest_neighbor(children[quadrant], query, nearest, best_dist_squared);
        // 2. Search neighboring nodes from top to bottom
        for (int i = 0; i < 4; ++i) {
            if (i == quadrant) continue;
            double dist_to_region = point_rect_distsq(query, &children[i]->boundary);
            // 3. If a node is too far away, prune (skip) it and all its children
            if (dist_to_region < *best_dist_squared)
                node_nearest_neighbor(children[i], query, nearest, best_dist_squared);
        }
    }
}

double qtree_nearest_neighbor(quadtree_t* qtree, point_t* query, point_t* nearest) {
    double best_dist_squared = DBL_MAX;
    node_nearest_neighbor(qtree->root, query, nearest, &best_dist_squared);
    return best_dist_squared;
}

static void node_remove_point(node_t* node, point_t* point) {
    if (node_is_leaf(node)) {
        for (int i = 0; i < node->count; ++i) {
            if (node->points[i].id == point->id && node->count > 1) {
#if 1
                // shift last point into deleted point
                memcpy(&node->points[i], &node->points[--node->count], sizeof(node->points[0]));
#else
                memcpy(&node->points[i], &node->points[node->count], sizeof(node->points[0]));
#endif
                return;
            } else if (node->points[i].id == point->id && node->count == 1) {
                node->count--;
                return;
            }

        }
    } else {
        // search top-down
        int quadrant = point_get_quadrant(&node->boundary, point);
        if (quadrant == IND_NW) node_remove_point(node->nw, point);
        else if (quadrant == IND_NE) node_remove_point(node->ne, point);
        else if (quadrant == IND_SE) node_remove_point(node->se, point);
        else if (quadrant == IND_SW) node_remove_point(node->sw, point);
    }
}

void qtree_remove_point(quadtree_t* qtree, point_t* point) {
    node_remove_point(qtree->root, point);
}

void qtree_update_point(quadtree_t* qtree, point_t* old_point, point_t* new_point) {
    node_remove_point(qtree->root, old_point);
    old_point->x = new_point->x;
    old_point->y = new_point->y;
    old_point->id = new_point->id;
    node_insert(qtree->root, old_point);
}

static void node_merge(node_t* node) {
    if (node_is_leaf(node))
        return;
    node_t* children[4] = { node->nw, node->ne, node->se, node->sw };
    bool all_leaves = true;
    for (int i = 0; i < 4; ++i) {
        if (!node_is_leaf(children[i])) {
            node_merge(children[i]);
            all_leaves = false;
        }
    }
    // for each child, collect its points and copy them to the parent and then
    // delete all children
    if (all_leaves) {
        size_t point_count = 0;
        for (int i = 0; i < 4; ++i)
            point_count += children[i]->count;
        if (point_count <= node_capacity) {
            node->count = 0;
            int ipoint = 0;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < children[i]->count; ++j) {
                    node->points[ipoint].x = children[i]->points[j].x;
                    node->points[ipoint].y = children[i]->points[j].y;
                    node->points[ipoint].id = children[i]->points[j].id;
                    ipoint++;
                    (node->count)++;
                }
            }
            for (int i = 0; i < 4; ++i) {
                if (children[i] != NULL) {
                    free(children[i]->points);
                    free(children[i]);
                }
            }
        }
    }
}

void qtree_merge(quadtree_t* qtree) {
    node_merge(qtree->root);
}

void node_del_all(node_t* node) {
    if (node == NULL)
        return;
    node_del_all(node->nw);
    node_del_all(node->ne);
    node_del_all(node->sw);
    node_del_all(node->se);
    if (node != NULL) {
        free(node->points);
        free(node);
    }
}

void qtree_del(quadtree_t* qtree) {
    node_del_all(qtree->root);
}
