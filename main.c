#include "quad.h"
#include <stdio.h>
#include <limits.h>

int main() {
#if 0
    quadtree_t qtree;    
    rect_t region = (rect_t) {0, 0, 200, 200};
    qtree.root = node_new(&region);
    point_t p1 = {10, 20}, p2 = {15, 190}, p3 = {30, 120}, p4 = {12, 20}, p5 = {48, 10};
    node_insert(qtree.root, &p1);
    node_insert(qtree.root, &p2);
    node_insert(qtree.root, &p3);
    node_insert(qtree.root, &p4);
    node_insert(qtree.root, &p5);
    printf("nw->nw->ne: (%d, %d)\n", qtree.root->nw->nw->ne->points[0].x, qtree.root->nw->nw->ne->points[0].y);

    rect_t r1 = {10, 10, 20, 20}, r2 = {21, 21, 24, 24}, r3 = {2, 2, 11, 11}, r4 = {19, 19, 23, 23};
    bool i1 = rect_intersect(&r1, &r2), i2 = rect_intersect(&r1, &r3), i3 = rect_intersect(&r1, &r4);
    printf("%d, %d, %d\n", i1, i2, i3);
    int count = 0;
    rect_t search = {11, 19, 200, 200};
    quadtree_query(qtree.root, &search, &count);
    printf("search: %d\n", count);
    return 0;
#else
    quadtree_t qtree;    
    rect_t region = {0, 0, 200, 200};
    qtree.root = node_new(&region);
    point_t p1 = {10, 20}, p2 = {15, 190}, p3 = {30, 120}, p4 = {12, 20}, p5 = {48, 10};
    node_insert(qtree.root, &p1);
    node_insert(qtree.root, &p2);
    node_insert(qtree.root, &p3);
    node_insert(qtree.root, &p4);
    node_insert(qtree.root, &p5);

    point_t query_point = {25, 30};
    point_t nearest_point;
    double best_dist = INT_MAX;
    quadtree_nearest_neighbor(qtree.root, &query_point, &nearest_point, &best_dist);

    printf("Nearest Point to (%d, %d): (%d, %d) with Distance: %.2f\n",
           query_point.x, query_point.y, nearest_point.x, nearest_point.y, best_dist);

    return 0;
#endif
}
