#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static double distance_sq(point_t* p1, point_t* p2) {
    return (p1->x - p2->x) * (p1->x - p2->x) + (p1->y - p2->y) * (p1->y - p2->y);
}


node_t* node_new(rect_t* boundary) {
    node_t *node = malloc(sizeof(node_t));
    node->boundary = *boundary;
    node->count = 0;
    node->points = malloc(g_capacity * sizeof(point_t));
    node->nw = NULL;
    node->ne = NULL;
    node->sw = NULL;
    node->se = NULL;
    return node;
}

void rect_divide(rect_t* src, rect_t* dest) {
    const int w = abs(src->x1 - src->x0), h = abs(src->y1 - src->y0);
    const int x0_nw = src->x0, y0_nw = src->y0;
    const int x1_nw = src->x0 + w/2, y1_nw = src->y0 + h/2;
    dest[IND_NW] = (rect_t) {x0_nw    , y0_nw    , x1_nw      , y1_nw};
    dest[IND_NE] = (rect_t) {x1_nw + 1, y0_nw    , x1_nw + w/2, y1_nw};
    dest[IND_SE] = (rect_t) {x1_nw + 1, y1_nw + 1, x1_nw + w/2, y1_nw + h/2};
    dest[IND_SW] = (rect_t) {x0_nw    , y1_nw + 1, x1_nw      , y1_nw + h/2};
}

int point_get_quadrant(rect_t* rect, point_t* point) {
    const int x = point->x, y = point->y;
    const int w = abs(rect->x0 - rect->x1), h = abs(rect->y0 - rect->y1);
    if ((rect->x0 <= x) && (x < rect->x0 + w/2) && (rect->y0 <= y) && (y < rect->y0 + h/2))
        return IND_NW;
    else if ((rect->x0 + w/2 <= x) && (x < rect->x1) && (rect->y0 <= y) && (y < rect->y0 + h/2))
        return IND_NE;
    else if ((rect->x0 + w/2 <= x) && (x < rect->x1) && (rect->y0 + h/2 <= y) && (y < rect->y1))
        return IND_SE;
    else if ((rect->x0 <= x) && (x < rect->x0 + w/2) && (rect->y0 + h/2 <= y) && (y < rect->y1))
        return IND_SW;
    return -1; // error - not in rectangle
}

bool node_is_leaf(node_t* node) {
    return (node != NULL) ? (node->nw == NULL && node->ne == NULL && node->se == NULL && node->sw == NULL) : false;
}

void node_insert(node_t* node, point_t* point) {
    if (node->count < g_capacity && node_is_leaf(node)) {
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

        // Insert the new point into the appropriate child node
        int quadrant = point_get_quadrant(&node->boundary, point);
        if (quadrant == IND_NW) node_insert(node->nw, point);
        else if (quadrant == IND_NE) node_insert(node->ne, point);
        else if (quadrant == IND_SE) node_insert(node->se, point);
        else if (quadrant == IND_SW) node_insert(node->sw, point);
    }
}

bool rect_intersect(rect_t* r1, rect_t* r2) {
    // the max of the left edges and the min of the right edges
    int left   = r1->x0 > r2->x0 ? r1->x0 : r2->x0;
    int right  = r1->x1 < r2->x1 ? r1->x1 : r2->x1;
    // the max of the top edges and the min of the bottom edges
    int top    = r1->y0 > r2->y0 ? r1->y0 : r2->y0;
    int bottom = r1->y1 < r2->y1 ? r1->y1 : r2->y1;
    return left <= right && top <= bottom;
}

void quadtree_query(node_t* node, rect_t* search_area, int* count) {
    if (!rect_intersect(&node->boundary, search_area))
        return;
    // If the node is a leaf and the boundary overlaps with the search area, count the points
    if (node_is_leaf(node)) {
        for (int i = 0; i < node->count; ++i) {
            if (point_in_rect(&node->points[i], search_area))
                (*count)++;
        }
    } else {
        quadtree_query(node->nw, search_area, count);
        quadtree_query(node->ne, search_area, count);
        quadtree_query(node->sw, search_area, count);
        quadtree_query(node->se, search_area, count);
    }
}

double point_rect_distsq(point_t* p, rect_t* rect) {
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

void quadtree_nearest_neighbor(node_t* node, point_t* query, point_t* nearest, double* best_dist_squared) {
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
        quadtree_nearest_neighbor(children[quadrant], query, nearest, best_dist_squared);
        // 2. Search neighboring nodes from top to bottom
        for (int i = 0; i < 4; ++i) {
            if (i == quadrant) continue;
            double dist_to_region = point_rect_distsq(query, &children[i]->boundary);
            // 3. If a node is too far away, prune (skip) it and all its children
            if (dist_to_region < *best_dist_squared)
                quadtree_nearest_neighbor(children[i], query, nearest, best_dist_squared);
        }
    }
}

void node_remove_point(node_t* node, point_t* point) {
    if (node_is_leaf(node)) {
        for (int i = 0; i < node->count; ++i) {
            if (node->points[i].x == point->x && node->points[i].y == point->y) {
                // shift last point into deleted point
                node->points[i] = node->points[--node->count];
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


void node_merge(node_t* node) {
    if (node_is_leaf(node))
        return;
    // if a child is a leaf, collect its points, else keep searching
    node_t* children[4] = {node->nw, node->ne, node->se, node->sw};
    int point_count = 0;
    for (int i = 0; i < 4; ++i)
        point_count += children[i]->count;
    // if the total points fit within the capacity, merge them into parent
    if (point_count <= g_capacity) {
        int ipoint = 0;
        for (int i = 0; i < 4; ++i) {
            if (!node_is_leaf(children[i]))
                node_merge(children[i]);
            for (int j = 0; j < children[i]->count; ++j)
                memcpy(&node->points[ipoint++], &children[i]->points[j], sizeof(node->points[0]));
        }
        node->count = point_count;
        // cleanup
        for (int i = 0; i < node->count; ++i) {
            if (children[i] != NULL) {
                free(children[i]->points);
                free(children[i]);
                // to make parent a leaf
                children[i] = NULL;
            }
        }
    }
}

void qtree_update_point(quadtree_t* qtree, point_t* old_point, point_t* new_point) {
    node_t* root = qtree->root;
    node_remove_point(root, old_point);
    old_point->x = new_point->x;
    old_point->y = new_point->y;
    node_insert(root, old_point);
    // TODO: merge only needs to be called periodically if a lot of points got removed
    //node_merge(root);
}

void qtree_delete(node_t* node) {
    if (node == NULL)
        return;
    qtree_delete(node->nw);
    qtree_delete(node->ne);
    qtree_delete(node->sw);
    qtree_delete(node->se);
    if (node != NULL) {
        free(node->points);
        free(node);
    }
}
