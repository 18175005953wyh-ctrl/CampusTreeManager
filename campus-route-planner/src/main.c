#include "graph.h"
#include "input.h"
#include "pathfinder.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_CAPACITY 1024

static int join_path(char *output, const char *directory, const char *file)
{
    int written = snprintf(output, PATH_CAPACITY, "%s/%s", directory, file);
    return written >= 0 && written < PATH_CAPACITY;
}

static int choose_data_directory(int argc, char *argv[], char *directory)
{
    FILE *probe;
    const char *slash, *backslash;
    size_t prefix;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: campus_route_planner [--data DIRECTORY]\n"
             "Default: data in current directory, then data beside the executable.\n"
             "Use --data for an explicit map directory.");
        return 0;
    }
    if (argc != 1) {
        if (argc != 3 || strcmp(argv[1], "--data") != 0 || argv[2][0] == '\0' || strlen(argv[2]) >= PATH_CAPACITY) {
            fputs("Invalid arguments. Usage: campus_route_planner [--data DIRECTORY]\n", stderr);
            return -1;
        }
        memcpy(directory, argv[2], strlen(argv[2]) + 1);
        return 1;
    }
    memcpy(directory, "data", 5);
    probe = fopen("data/locations.csv", "r");
    if (probe != NULL) { fclose(probe); return 1; }
    slash = strrchr(argv[0], '/');
    backslash = strrchr(argv[0], '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) slash = backslash;
    if (slash != NULL) {
        prefix = (size_t)(slash - argv[0]) + 1;
        if (prefix + 5 > PATH_CAPACITY) return -1;
        memcpy(directory, argv[0], prefix);
        memcpy(directory + prefix, "data", 5);
    }
    return 1;
}

static int read_location(const Graph *graph, const char *prompt, int *index)
{
    int id;
    for (;;) {
        if (!read_integer(prompt, 1, INT_MAX, &id)) return 0;
        *index = find_location_index_by_id(graph, id);
        if (*index >= 0) return 1;
        puts("Unknown location ID. Please try again.");
    }
}

static void find_route(const Graph *graph)
{
    int start, end;
    PathResult result;
    if (graph->count == 0) { puts("No locations loaded."); return; }
    if (!read_location(graph, "Start location ID: ", &start) ||
        !read_location(graph, "Destination location ID: ", &end)) return;
    if (start == end) puts("Start and destination are the same location.");
    result = dijkstra_shortest_path(graph, start, end);
    print_path(graph, &result);
}

int main(int argc, char *argv[])
{
    Graph graph;
    char directory[PATH_CAPACITY], locations[PATH_CAPACITY], roads[PATH_CAPACITY];
    char keyword[NAME_LENGTH];
    int option, status = choose_data_directory(argc, argv, directory);
    if (status == 0) return EXIT_SUCCESS;
    if (status < 0 || !join_path(locations, directory, "locations.csv") || !join_path(roads, directory, "roads.csv")) {
        fputs("Data directory arguments are invalid or too long.\n", stderr);
        return EXIT_FAILURE;
    }
    graph_init(&graph);
    if (!load_locations(&graph, locations) || !load_roads(&graph, roads)) {
        fputs("Map load failed. Check files or use --data DIRECTORY.\n", stderr);
        return EXIT_FAILURE;
    }
    printf("Loaded %d locations.\n", graph.count);
    for (;;) {
        puts("\n===== Campus Route Planner =====\n1. List all locations\n2. Show direct roads\n"
             "3. Find shortest route\n4. Search location\n5. Show map summary\n0. Exit");
        if (!read_integer("Select an option: ", 0, 5, &option) || option == 0) break;
        switch (option) {
        case 1: list_locations(&graph); break;
        case 2: list_roads(&graph); break;
        case 3: find_route(&graph); break;
        case 4:
            if (read_keyword("Name keyword: ", keyword, (int)sizeof keyword)) search_locations(&graph, keyword);
            break;
        case 5: show_map_summary(&graph); break;
        default: break;
        }
    }
    puts("Goodbye.");
    return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
}
