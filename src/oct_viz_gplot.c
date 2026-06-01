#include "oct_viz_gplot.h"
#include <stdio.h>
#include <stdlib.h>

enum {
  OUTPUT_WIDTH = 1800,
  OUTPUT_HEIGHT = 1200,
};

static bool open_gnuplot(FILE **pipe) {
  *pipe = popen("gnuplot", "w");
  if (*pipe != NULL)
    return true;

  fprintf(stderr, "warning: gnuplot not available, skipping octree plot output\n");
  return false;
}

static size_t node_point_count(const oct_node_t *node) {
  size_t total = 0;

  if (node == NULL)
    return 0;
  if (node->is_leaf)
    return (size_t)node->count;

  for (int octant = 0; octant < 8; ++octant)
    total += node_point_count(node->children[octant]);
  return total;
}

static void emit_edge(FILE *plot, int x0, int y0, int z0, int x1, int y1, int z1) {
  fprintf(plot, "%d %d %d\n", x0, y0, z0);
  fprintf(plot, "%d %d %d\n\n", x1, y1, z1);
}

static void emit_cuboid_edges(FILE *plot, cuboid_t cuboid) {
  const int x0 = cuboid.x0;
  const int y0 = cuboid.y0;
  const int z0 = cuboid.z0;
  const int x1 = cuboid.x1;
  const int y1 = cuboid.y1;
  const int z1 = cuboid.z1;

  emit_edge(plot, x0, y0, z0, x1, y0, z0);
  emit_edge(plot, x1, y0, z0, x1, y1, z0);
  emit_edge(plot, x1, y1, z0, x0, y1, z0);
  emit_edge(plot, x0, y1, z0, x0, y0, z0);

  emit_edge(plot, x0, y0, z1, x1, y0, z1);
  emit_edge(plot, x1, y0, z1, x1, y1, z1);
  emit_edge(plot, x1, y1, z1, x0, y1, z1);
  emit_edge(plot, x0, y1, z1, x0, y0, z1);

  emit_edge(plot, x0, y0, z0, x0, y0, z1);
  emit_edge(plot, x1, y0, z0, x1, y0, z1);
  emit_edge(plot, x1, y1, z0, x1, y1, z1);
  emit_edge(plot, x0, y1, z0, x0, y1, z1);
}

static void emit_leaf_cuboids(FILE *plot, const oct_node_t *node) {
  if (node == NULL)
    return;
  if (node->is_leaf) {
    if (node->count > 0)
      emit_cuboid_edges(plot, node->boundary);
    return;
  }

  for (int octant = 0; octant < 8; ++octant)
    emit_leaf_cuboids(plot, node->children[octant]);
}

static void emit_sampled_points(FILE *plot, const oct_node_t *node, size_t stride) {
  if (node == NULL)
    return;
  if (node->is_leaf) {
    for (int i = 0; i < node->count; ++i) {
      if ((node->points[i].id % stride) == 0)
        fprintf(plot, "%d %d %d\n", node->points[i].x, node->points[i].y,
                node->points[i].z);
    }
    return;
  }

  for (int octant = 0; octant < 8; ++octant)
    emit_sampled_points(plot, node->children[octant], stride);
}

bool oct_gplt_write_png(const octree_t *octree, cuboid_t boundary,
                        const char *output_path, size_t max_rendered_points) {
  FILE *plot;
  size_t total_points;
  size_t point_stride;

  if (octree == NULL || octree->root == NULL || output_path == NULL)
    return false;
  if (!open_gnuplot(&plot))
    return false;

  total_points = node_point_count(octree->root);
  if (max_rendered_points == 0)
    max_rendered_points = total_points == 0 ? 1 : total_points;
  point_stride = total_points > max_rendered_points
                     ? (total_points + max_rendered_points - 1) / max_rendered_points
                     : 1;

  fprintf(plot, "set term pngcairo size %d,%d\n", OUTPUT_WIDTH, OUTPUT_HEIGHT);
  fprintf(plot, "set output '%s'\n", output_path);
  fprintf(plot, "set title 'Octree occupied leaf cuboids and sampled points'\n");
  fprintf(plot, "set xlabel 'x'\n");
  fprintf(plot, "set ylabel 'y'\n");
  fprintf(plot, "set zlabel 'z'\n");
  fprintf(plot, "set xrange [%d:%d]\n", boundary.x0, boundary.x1);
  fprintf(plot, "set yrange [%d:%d]\n", boundary.y0, boundary.y1);
  fprintf(plot, "set zrange [%d:%d]\n", boundary.z0, boundary.z1);
  fprintf(plot, "set view 64, 28, 1.1, 1.15\n");
  fprintf(plot, "set xyplane at %d\n", boundary.z0);
  fprintf(plot, "set ticslevel 0\n");
  fprintf(plot, "set border 4095 back\n");
  fprintf(plot, "set grid xtics ytics ztics\n");
  fprintf(plot, "set key off\n");
  fprintf(plot,
          "splot '-' with lines lw 1 lc rgb '#1f2937', '-' with points pt 7 ps 0.55 lc rgb '#d9480f'\n");

  emit_leaf_cuboids(plot, octree->root);
  fprintf(plot, "e\n");
  emit_sampled_points(plot, octree->root, point_stride);
  fprintf(plot, "e\n");

  fflush(plot);
  return pclose(plot) == 0;
}