#include "pathfinder.h"
#include <stdio.h>

PathResult dijkstra_shortest_path(const Graph *graph, int start_index, int end_index)
{
    PathResult result = {0};
    int distance[MAX_LOCATIONS], visited[MAX_LOCATIONS] = {0}, previous[MAX_LOCATIONS];
    int step, i, current, neighbor, weight, candidate, reverse[MAX_LOCATIONS], length = 0;
    result.distance = INF_DISTANCE;
    if (start_index < 0 || end_index < 0 || start_index >= graph->count || end_index >= graph->count) return result;
    for (i = 0; i < graph->count; ++i) { distance[i] = INF_DISTANCE; previous[i] = -1; }
    distance[start_index] = 0;
    for (step = 0; step < graph->count; ++step) {
        current = -1;
        for (i = 0; i < graph->count; ++i)
            if (!visited[i] && distance[i] < INF_DISTANCE &&
                (current == -1 || distance[i] < distance[current])) current = i;
        if (current == -1) break;
        visited[current] = 1;
        if (current == end_index) break;
        for (neighbor = 0; neighbor < graph->count; ++neighbor) {
            weight = graph->adjacency[current][neighbor];
            if (visited[neighbor] || weight < 0 || weight >= INF_DISTANCE) continue;
            /* Check before addition, so even a malformed graph cannot overflow. */
            if (distance[current] > INF_DISTANCE - weight) continue;
            candidate = distance[current] + weight;
            if (candidate < distance[neighbor]) {
                distance[neighbor] = candidate;
                previous[neighbor] = current;
            }
        }
    }
    if (distance[end_index] == INF_DISTANCE) return result;
    current = end_index;
    while (current != -1 && length < MAX_LOCATIONS) {
        reverse[length++] = current;
        if (current == start_index) break;
        current = previous[current];
    }
    if (reverse[length - 1] != start_index) return result;
    result.found = 1;
    result.distance = distance[end_index];
    result.path_length = length;
    for (i = 0; i < length; ++i) result.path[i] = reverse[length - 1 - i];
    return result;
}

void print_path(const Graph *graph, const PathResult *result)
{
    int i;
    if (!result->found) { puts("No route available between these locations."); return; }
    puts("Shortest route:");
    for (i = 0; i < result->path_length; ++i)
        printf("%s%s", i ? " -> " : "", graph->locations[result->path[i]].name);
    printf("\n\nTotal distance: %d m\n", result->distance);
}
