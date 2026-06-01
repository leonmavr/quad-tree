#include "nanotest.h"
#include "oct.h"

static bool are_points_equal(point3_t p1, point3_t p2) { return p1.id == p2.id; }

int main(int argc, char **argv) {
  cuboid_t boundary = {0, 0, 0, 15, 15, 15};
  point3_t points[5] = {{1, 1, 1}, {2, 2, 2}, {5, 5, 5}, {6, 1, 1}, {12, 12, 12}};
  for (int i = 0; i < 5; ++i) points[i].id = (size_t)i;

  octree_arena_init(128);
  octree_t octree;
  octree_new(&octree, boundary);
  for (int i = 0; i < 5; ++i)
    octree_insert(&octree, points[i]);

  NTEST_ASSERT(octree.root->nwf != NULL && !octree.root->nwf->is_leaf);
  NTEST_ASSERT(are_points_equal(octree.root->nwf->nwf->points[0], points[0]));
  NTEST_ASSERT(are_points_equal(octree.root->nwf->nwf->points[1], points[1]));
  NTEST_ASSERT(are_points_equal(octree.root->nwf->seb->points[0], points[2]));
  NTEST_ASSERT(are_points_equal(octree.root->nwf->nef->points[0], points[3]));
  NTEST_ASSERT(are_points_equal(octree.root->seb->points[0], points[4]));

  point3_t nearest;
  point3_t query = {6, 2, 1};
  double dist_nearest_squared = octree_nearest_neighbor(&octree, query, &nearest);
  NTEST_ASSERT(0.99 <= dist_nearest_squared && dist_nearest_squared <= 1.01);
  NTEST_ASSERT(nearest.id == points[3].id);

  int how_many = 0;
  cuboid_t search_area = {0, 0, 0, 7, 7, 7};
  octree_query(&octree, search_area, &how_many);
  NTEST_ASSERT(how_many == 4);

  point3_t pnew = {1, 12, 1, 99};
  octree_update_point(&octree, &points[4], &pnew);
  NTEST_ASSERT(octree.root->swf->count == 1);
  NTEST_ASSERT(octree.root->swf->points[0].id == pnew.id);

  NTEST_ASSERT(octree.root->nwf->seb->count == 1);
  octree_remove_point(&octree, &points[2]);
  NTEST_ASSERT(octree.root->nwf->seb->count == 0);
  NTEST_ASSERT(octree.root->nwf->nef->count == 1);
  octree_remove_point(&octree, &points[3]);
  NTEST_ASSERT(octree.root->nwf->nef->count == 0);

  octree_merge(&octree);
  NTEST_ASSERT(octree.root->nwf->is_leaf);
  NTEST_ASSERT(octree.root->nwf->count == 2);
  NTEST_ASSERT((are_points_equal(octree.root->nwf->points[0], points[0]) &&
                are_points_equal(octree.root->nwf->points[1], points[1])) ||
               (are_points_equal(octree.root->nwf->points[0], points[1]) &&
                are_points_equal(octree.root->nwf->points[1], points[0])));

  octree_del(&octree);
  return ntest_result;
}