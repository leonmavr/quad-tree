#include "quad.h"
#include "viz.h"
#include <limits.h>
#include <stdlib.h>
#include <time.h>
// TODO: this should be less than what the visualiser can handle
#define NPARTICLES 100


static size_t id_particle = 0;

typedef struct particle_t {
    float accelerationx, accelerationy;
    float velx, vely;
    point_t point;
} particle_t;

void reflect(particle_t* particle, rect_t* boundary, float dt) {
    const int w = boundary->x1;
    const int h = boundary->y1;
    if (particle->point.x + particle->velx * dt > w) {
        particle->point.x -= w - particle->velx * dt;
        particle->velx *= -1;
    } else if (particle->point.x + particle->velx * dt < boundary->x0) {
        particle->point.x = -particle->velx * dt - particle->point.x;
        particle->velx *= -1;
    }
    if (particle->point.y + particle->vely * dt > h) {
        particle->point.y -= h - particle->vely * dt;
        particle->vely *= -1;
    } else if (particle->point.y + particle->vely * dt < boundary->y0) {
        particle->point.y = -particle->vely * dt - particle->point.y;
        particle->vely *= -1;
    }
}

static particle_t particles[NPARTICLES];

static size_t ids = 0;

int main() {
    srand(time(NULL));
    rect_t boundary = {.x0=0, .y0=0, .x1=400, .y1=400};
    point_t points[NPARTICLES];
    particle_t particles[NPARTICLES];
    for (int i = 0; i < NPARTICLES; ++i) {
        points[i].x = random() % boundary.x1;
        points[i].y = random() % boundary.y1;
        points[i].id = ids++;
    }
    quadtree_t qtree;
    viz_init(boundary.x1, boundary.y1);
    qtree.root = node_new(&boundary);
    for (int i = 0; i < NPARTICLES; ++i) {
        node_insert(qtree.root, &points[i]);
    }
    qtree_graph(qtree.root);
    viz_flush();
    sleep(3);
    viz_close();
}
