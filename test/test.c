#include "quad.h"
#include "nanotest.h"
#include <assert.h>
#include <limits.h>

static bool are_points_equal(point_t* p1, point_t* p2) {
    return p1->x == p2->x && p1->y == p2 ->y;
}   

int main(int argc, char** argv) {
    // in case it changes in the future
    g_capacity = 2;
    rect_t boundary = {0, 0, 400, 200};
    point_t points[10] = {{51, 25}, {20, 49}, {199, 62}, {160, 70}, {160, 80},
        {110, 90}, {108, 120}, {180, 143}, {0, 149}, {330, 190}};
    quadtree_t qtree;
    qtree.root = node_new(&boundary);   
    for (int i = 0; i < sizeof(points)/sizeof(points[0]); ++i)
        node_insert(qtree.root, &points[i]);
    //-------------------------------------------------------------------------
    // Inspect insertion
    //-------------------------------------------------------------------------
    NTEST_ASSERT(are_points_equal(&qtree.root->nw->se->ne->points[0], &points[2]));
    NTEST_ASSERT(are_points_equal(&qtree.root->nw->se->ne->points[1], &points[3]));
    NTEST_ASSERT(are_points_equal(&qtree.root->nw->se->se->points[0], &points[4]));
    NTEST_ASSERT(are_points_equal(&qtree.root->nw->se->sw->points[0], &points[5]));
    NTEST_ASSERT(are_points_equal(&qtree.root->nw->nw->points[0], &points[0]));
    NTEST_ASSERT(are_points_equal(&qtree.root->nw->nw->points[1], &points[1]));
    NTEST_ASSERT(are_points_equal(&qtree.root->sw->nw->points[0], &points[8]));
    NTEST_ASSERT(are_points_equal(&qtree.root->sw->ne->points[0], &points[6]));
    NTEST_ASSERT(are_points_equal(&qtree.root->sw->ne->points[1], &points[7]));
    NTEST_ASSERT(are_points_equal(&qtree.root->se->points[0], &points[9]));
    //-------------------------------------------------------------------------
    // Nearest point 
    //-------------------------------------------------------------------------
    point_t nearest; 
    point_t query = {160, 78};
    double dist_nearest_squared = INT_MAX;
    quadtree_nearest_neighbor(qtree.root, &query, &nearest, &dist_nearest_squared);
    NTEST_ASSERT(3.99 <= dist_nearest_squared && dist_nearest_squared <= 4.01);
    NTEST_ASSERT(nearest.x == 160 && nearest.y == 80);
    //-------------------------------------------------------------------------
    // Update point 
    //-------------------------------------------------------------------------
    point_t pnew = {90, 90};
    qtree_update_point(&qtree, &points[6], &pnew);
    NTEST_ASSERT(qtree.root->nw->sw->points[0].x == pnew.x &&
        qtree.root->nw->sw->points[0].y == pnew.y);
    NTEST_ASSERT(qtree.root->sw->ne->count == 1 && qtree.root->nw->sw->count == 1);
    //-------------------------------------------------------------------------
    // Merge
    //-------------------------------------------------------------------------
    // now sw->nw should have one point and so should sw->ne, hence mergeable
    //node_merge(qtree.root);
    //NTEST_ASSERT(qtree.root->sw->count == 2);
    // assert that 90, 90 has moved to nw->sw
    return 0;
}