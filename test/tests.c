#include "nanotest.h"
#include "quad.h"
#include <assert.h>
#include <limits.h>
#include <math.h>

// The test target compiles the library with -DNODE_CAPACITY=2.

static bool are_points_equal(point_t p1, point_t p2) { return p1.id == p2.id; }

// Convenience aliases so test assertions stay readable
#define NW children[IND_NW]
#define NE children[IND_NE]
#define SE children[IND_SE]
#define SW children[IND_SW]

int main(int argc, char **argv) {
  rect_t boundary = {0, 0, 400, 200};
  point_t points[10] = {{51, 25},  {20, 49},  {199, 62},  {160, 70},
                        {160, 80}, {110, 90}, {108, 120}, {180, 143},
                        {0, 149},  {330, 190}};
  // Assign unique ids so are_points_equal works correctly
  for (int i = 0; i < 10; ++i) points[i].id = i;

  qtree_arena_init(256);
  quadtree_t qtree;
  qtree_new(&qtree, boundary);
  for (int i = 0; i < sizeof(points) / sizeof(points[0]); ++i)
    qtree_insert(&qtree, points[i]);

  //-------------------------------------------------------------------------
  // Inspect insertion
  //-------------------------------------------------------------------------
  NTEST_ASSERT(are_points_equal(qtree.root->NW->SE->NE->points[0], points[2]));
  NTEST_ASSERT(are_points_equal(qtree.root->NW->SE->NE->points[1], points[3]));
  NTEST_ASSERT(are_points_equal(qtree.root->NW->SE->SE->points[0], points[4]));
  NTEST_ASSERT(are_points_equal(qtree.root->NW->SE->SW->points[0], points[5]));
  NTEST_ASSERT(are_points_equal(qtree.root->NW->NW->points[0], points[0]));
  NTEST_ASSERT(are_points_equal(qtree.root->NW->NW->points[1], points[1]));
  NTEST_ASSERT(are_points_equal(qtree.root->SW->NW->points[0], points[8]));
  NTEST_ASSERT(are_points_equal(qtree.root->SW->NE->points[0], points[6]));
  NTEST_ASSERT(are_points_equal(qtree.root->SW->NE->points[1], points[7]));
  NTEST_ASSERT(are_points_equal(qtree.root->SE->points[0], points[9]));

  //-------------------------------------------------------------------------
  // Nearest point
  //-------------------------------------------------------------------------
  point_t nearest;
  point_t query = {160, 78};
  double dist_nearest_squared = qtree_nearest_neighbor(&qtree, query, &nearest);
  NTEST_ASSERT(3.99 <= dist_nearest_squared && dist_nearest_squared <= 4.01);
  NTEST_ASSERT(nearest.id == points[4].id);

  //-------------------------------------------------------------------------
  // Rectangular query
  //-------------------------------------------------------------------------
  int how_many = 0;
  rect_t search_area = {100, 50, 200, 100};
  qtree_query(&qtree, search_area, &how_many);
  NTEST_ASSERT(how_many == 4);

  //-------------------------------------------------------------------------
  // Update point
  //-------------------------------------------------------------------------
  point_t pnew = {90, 90, 99};
  // {108, 120} -> {90, 90}
  qtree_update_point(&qtree, &points[6], &pnew);
  NTEST_ASSERT(qtree.root->NW->SW->points[0].id == pnew.id);
  NTEST_ASSERT(qtree.root->SW->NE->count == 1 &&
               qtree.root->NW->SW->count == 1);

  //-------------------------------------------------------------------------
  // Merge
  //-------------------------------------------------------------------------
  // sw->nw has one point, sw->ne has one point -> mergeable into sw
  qtree_merge(&qtree);
  // p7, p8 should have propagated up into sw
  NTEST_ASSERT(qtree.root->SW->count == 2);
  NTEST_ASSERT((are_points_equal(qtree.root->SW->points[1], points[7]) &&
                are_points_equal(qtree.root->SW->points[0], points[8])) ||
               (are_points_equal(qtree.root->SW->points[0], points[8]) &&
                are_points_equal(qtree.root->SW->points[1], points[7])));

  //-------------------------------------------------------------------------
  // Point deletion
  //-------------------------------------------------------------------------
  NTEST_ASSERT(qtree.root->NW->SE->SW->count == 1);
  qtree_remove_point(&qtree, &points[5]);
  NTEST_ASSERT(qtree.root->NW->SE->SW->count == 0);

  qtree_del(&qtree);
  return ntest_result;
}
