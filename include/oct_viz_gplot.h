#ifndef OCT_VIZ_GPLOT_H
#define OCT_VIZ_GPLOT_H

#include "oct.h"

bool oct_gplt_write_png(const octree_t *octree, cuboid_t boundary,
                        const char *output_path, size_t max_rendered_points);

#endif // OCT_VIZ_GPLOT_H