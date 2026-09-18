#ifndef GRAPH_H
#define GRAPH_H

#define MAX_LOCATIONS 50
#define NAME_LENGTH 64
#define INF_DISTANCE 1000000000
/* Every simple path has at most MAX_LOCATIONS - 1 roads. */
#define MAX_ROAD_DISTANCE ((INF_DISTANCE - 1) / (MAX_LOCATIONS - 1))

typedef struct {
    int id;
    char name[NAME_LENGTH];
    char type[NAME_LENGTH];
} Location;

typedef struct {
    Location locations[MAX_LOCATIONS];
    int adjacency[MAX_LOCATIONS][MAX_LOCATIONS];
    int count;
} Graph;

typedef struct {
    int road_count;
    long long total_distance;
    int degrees[MAX_LOCATIONS];
    int max_degree;
    int isolated_count;
} MapSummary;

void graph_init(Graph *graph);
int graph_add_location(Graph *graph, int id, const char *name, const char *type);
int graph_add_road(Graph *graph, int from_id, int to_id, int distance);
int load_locations(Graph *graph, const char *filename);
int load_roads(Graph *graph, const char *filename);
int find_location_index_by_id(const Graph *graph, int id);
int find_location_matches(const Graph *graph, const char *keyword, int matches[]);
MapSummary calculate_map_summary(const Graph *graph);
void list_locations(const Graph *graph);
void list_roads(const Graph *graph);
void search_locations(const Graph *graph, const char *keyword);
void show_map_summary(const Graph *graph);

#endif
