#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

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
    return node->nw == NULL && node->ne == NULL && node->se == NULL && node->sw == NULL;
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
