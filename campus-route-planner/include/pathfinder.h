#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "graph.h"

typedef struct {
    int found;
    int distance;
    int path[MAX_LOCATIONS];
    int path_length;
} PathResult;

PathResult dijkstra_shortest_path(const Graph *graph, int start_index, int end_index);
void print_path(const Graph *graph, const PathResult *result);

#endif
