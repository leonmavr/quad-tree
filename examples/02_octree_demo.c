#include "oct.h"
#include "oct_viz_gplot.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
  DEFAULT_POINTS = 3000,
  INITIAL_ARENA_CAPACITY = 4096,
  BOUNDARY_SIDE = 255,
  CLUSTER_SIDE = 64,
  MAX_RENDERED_POINTS = 20000,
};

static const char *TREE_PNG_PATH = "assets/02_octree_tree.png";

static void usage(const char *program) {
  fprintf(stderr, "usage: %s [point-count]\n", program);
}

static bool parse_size_arg(const char *text, size_t *value) {
  char *end = NULL;
  unsigned long long parsed;

  errno = 0;
  parsed = strtoull(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || parsed == 0)
    return false;

  *value = (size_t)parsed;
  return true;
}

static point3_t generate_point(size_t index) {
  static const int cluster_bases[4][3] = {
      {16, 16, 16},
      {156, 28, 76},
      {40, 152, 152},
      {168, 168, 168},
  };
  const int cluster = (int)(index % 4);
  // simple linear hashing (similar to Fibonacci) based on Vigna's generator:
  // https://prng.di.unimi.it/splitmix64.c
  enum { PERMUTE_MULTIPLIER = 0x9e3779b97f4a7c15ULL,
         MIXING_CONSTANT = 0xbf58476d1ce4e5b9ULL };
  const uint64_t seed = (uint64_t)index * PERMUTE_MULTIPLIER + MIXING_CONSTANT;
  point3_t point = {
      .x = cluster_bases[cluster][0] + (int)(seed % CLUSTER_SIDE),
      .y = cluster_bases[cluster][1] + (int)((seed >> 8) % CLUSTER_SIDE),
      .z = cluster_bases[cluster][2] + (int)((seed >> 16) % CLUSTER_SIDE),
      .id = index,
  };

  enum { OUTLIER_EVERY = 53 };
  if ((index % OUTLIER_EVERY) == 0) {
    // random coordinates from seed, within the boundary
    point.x = (int)(seed & BOUNDARY_SIDE);
    point.y = (int)((seed >> 8) & BOUNDARY_SIDE);
    point.z = (int)((seed >> 16) & BOUNDARY_SIDE);
  }
  return point;
}

int main(int argc, char **argv) {
  cuboid_t boundary = {0, 0, 0, BOUNDARY_SIDE, BOUNDARY_SIDE, BOUNDARY_SIDE};
  size_t count = DEFAULT_POINTS;
  octree_t octree;

  if (argc > 2) {
    usage(argv[0]);
    return 1;
  }
  if (argc == 2 && !parse_size_arg(argv[1], &count)) {
    usage(argv[0]);
    return 1;
  }

  octree_arena_init(INITIAL_ARENA_CAPACITY);
  octree_new(&octree, boundary);
  for (size_t i = 0; i < count; ++i)
    octree_insert(&octree, generate_point(i));

  if (!oct_gplt_write_png(&octree, boundary, TREE_PNG_PATH, MAX_RENDERED_POINTS)) {
    octree_del(&octree);
    return 1;
  }

  printf("wrote %s with %zu points\n", TREE_PNG_PATH, count);
  octree_del(&octree);
  return 0;
}