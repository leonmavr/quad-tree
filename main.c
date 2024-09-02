#include "quad.h"
#include <stdio.h>

int main() {
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
    printf("%d, %d, %d", i1, i2, i3);
    return 0;

}
