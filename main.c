#include "quad.h"
#include <stdio.h>

int main() {
    quadtree_t qtree;    
    rect_t region = (rect_t) {0, 0, 200, 200};
    qtree.root = node_new(&region);
    point_t p1 = {10, 20}, p2 = {15, 190}, p3 = {30, 120}, p4 = {12, 20}, p5 = {48, 10};
    quadtree_insert(qtree.root, &p1);
    quadtree_insert(qtree.root, &p2);
    quadtree_insert(qtree.root, &p3);
    quadtree_insert(qtree.root, &p4);
    quadtree_insert(qtree.root, &p5);
    printf("nw->nw->ne: (%d, %d), ", qtree.root->nw->nw->ne->points[0].x, qtree.root->nw->nw->ne->points[0].y);
    return 0;
}
