#include "graph.h"
#include "input.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void graph_init(Graph *graph)
{
    int i, j;
    memset(graph, 0, sizeof *graph);
    for (i = 0; i < MAX_LOCATIONS; ++i)
        for (j = 0; j < MAX_LOCATIONS; ++j)
            graph->adjacency[i][j] = i == j ? 0 : INF_DISTANCE;
}

int find_location_index_by_id(const Graph *graph, int id)
{
    int i;
    for (i = 0; i < graph->count; ++i)
        if (graph->locations[i].id == id) return i;
    return -1;
}

static int valid_label(const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    int has_text = 0;
    if (strlen(text) >= NAME_LENGTH) return 0;
    for (; *p; ++p) {
        if (*p == ',' || iscntrl(*p)) return 0;
        if (!isspace(*p)) has_text = 1;
    }
    return has_text;
}

int graph_add_location(Graph *graph, int id, const char *name, const char *type)
{
    Location *location;
    if (graph->count >= MAX_LOCATIONS || id <= 0 ||
        find_location_index_by_id(graph, id) >= 0 || !valid_label(name) || !valid_label(type)) return 0;
    location = &graph->locations[graph->count++];
    location->id = id;
    memcpy(location->name, name, strlen(name) + 1);
    memcpy(location->type, type, strlen(type) + 1);
    return 1;
}

int graph_add_road(Graph *graph, int from_id, int to_id, int distance)
{
    int from = find_location_index_by_id(graph, from_id);
    int to = find_location_index_by_id(graph, to_id);
    if (from < 0 || to < 0 || from == to || distance < 1 || distance > MAX_ROAD_DISTANCE) return 0;
    /* Duplicate undirected roads represent one edge; keep the shorter distance. */
    if (distance < graph->adjacency[from][to]) {
        graph->adjacency[from][to] = distance;
        graph->adjacency[to][from] = distance;
    }
    return 1;
}

static int split_fields(char *line, char *fields[3])
{
    int i;
    char *comma;
    fields[0] = line;
    for (i = 0; i < 2; ++i) {
        comma = strchr(fields[i], ',');
        if (comma == NULL) return 0;
        *comma = '\0';
        fields[i + 1] = comma + 1;
    }
    if (strchr(fields[2], ',') != NULL) return 0;
    for (i = 0; i < 3; ++i) {
        fields[i] = trim_text(fields[i]);
        if (*fields[i] == '\0') return 0;
    }
    return 1;
}

static void warn_row(const char *filename, unsigned long line, const char *reason)
{
    fprintf(stderr, "Warning: %s line %lu: %s\n", filename, line, reason);
}

static int load_csv(Graph *graph, const char *filename, int roads)
{
    FILE *file = fopen(filename, "r");
    char buffer[512];
    char *line, *fields[3];
    int status, first, second, distance, from, to, failed;
    unsigned long row = 0;
    if (file == NULL) {
        fprintf(stderr, "Cannot open map file: %s\n", filename);
        return 0;
    }
    while ((status = read_line(file, buffer, (int)sizeof buffer)) != 0) {
        ++row;
        if (status < 0) { warn_row(filename, row, "line too long; skipped."); continue; }
        line = buffer;
        if (row == 1 && strncmp(line, "\xEF\xBB\xBF", 3) == 0) line += 3;
        line = trim_text(line);
        if (*line == '\0') continue;
        if (!split_fields(line, fields) || !parse_integer(fields[0], 1, INT_MAX, &first)) {
            warn_row(filename, row, "invalid CSV fields or ID; skipped.");
            continue;
        }
        if (!roads) {
            if (graph->count == MAX_LOCATIONS) warn_row(filename, row, "location limit (50); skipped.");
            else if (find_location_index_by_id(graph, first) >= 0) warn_row(filename, row, "duplicate location ID; skipped.");
            else if (!graph_add_location(graph, first, fields[1], fields[2])) warn_row(filename, row, "invalid name/type (1-63 bytes); skipped.");
        } else {
            if (!parse_integer(fields[1], 1, INT_MAX, &second) ||
                !parse_integer(fields[2], 1, MAX_ROAD_DISTANCE, &distance)) {
                warn_row(filename, row, "invalid endpoint/distance; skipped.");
                continue;
            }
            from = find_location_index_by_id(graph, first);
            to = find_location_index_by_id(graph, second);
            if (from < 0 || to < 0 || from == to) {
                warn_row(filename, row, "unknown endpoint or self-road; skipped.");
                continue;
            }
            if (graph->adjacency[from][to] != INF_DISTANCE)
                warn_row(filename, row, "duplicate road; keeping shorter distance.");
            (void)graph_add_road(graph, first, second, distance);
        }
    }
    failed = ferror(file);
    if (fclose(file) != 0) failed = 1;
    if (failed) { fprintf(stderr, "Error reading map file: %s\n", filename); return 0; }
    return 1;
}

int load_locations(Graph *graph, const char *filename) { return load_csv(graph, filename, 0); }
int load_roads(Graph *graph, const char *filename) { return load_csv(graph, filename, 1); }

static void print_header(void)
{
    printf("%-10s | %-63s | %-63s\n", "ID", "Name", "Type");
}

static void print_location(const Location *location)
{
    printf("%-10d | %-63s | %-63s\n", location->id, location->name, location->type);
}

void list_locations(const Graph *graph)
{
    int i;
    if (graph->count == 0) { puts("No locations loaded."); return; }
    print_header();
    for (i = 0; i < graph->count; ++i) print_location(&graph->locations[i]);
}

void list_roads(const Graph *graph)
{
    int i, j, count = 0;
    for (i = 0; i < graph->count; ++i) {
        for (j = i + 1; j < graph->count; ++j) {
            if (graph->adjacency[i][j] != INF_DISTANCE) {
                printf("%s <-> %s : %d m\n", graph->locations[i].name,
                       graph->locations[j].name, graph->adjacency[i][j]);
                ++count;
            }
        }
    }
    if (count == 0) puts("No direct roads loaded.");
}

static int contains_ignore_case(const char *text, const char *keyword)
{
    size_t i, j;
    for (i = 0; text[i]; ++i) {
        for (j = 0; keyword[j] && text[i + j]; ++j)
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)keyword[j])) break;
        if (keyword[j] == '\0') return 1;
    }
    return 0;
}

int find_location_matches(const Graph *graph, const char *keyword, int matches[])
{
    int i, count = 0;
    const char *p = keyword;
    while (isspace((unsigned char)*p)) ++p;
    if (*p == '\0') return 0;
    for (i = 0; i < graph->count; ++i)
        if (contains_ignore_case(graph->locations[i].name, keyword)) matches[count++] = i;
    return count;
}

void search_locations(const Graph *graph, const char *keyword)
{
    int matches[MAX_LOCATIONS];
    int i, count = find_location_matches(graph, keyword, matches);
    if (count == 0) { puts("No matching locations."); return; }
    print_header();
    for (i = 0; i < count; ++i) print_location(&graph->locations[matches[i]]);
}

MapSummary calculate_map_summary(const Graph *graph)
{
    MapSummary summary = {0};
    int i, j;
    for (i = 0; i < graph->count; ++i) {
        for (j = i + 1; j < graph->count; ++j) {
            if (graph->adjacency[i][j] != INF_DISTANCE) {
                ++summary.road_count;
                summary.total_distance += graph->adjacency[i][j];
                ++summary.degrees[i];
                ++summary.degrees[j];
            }
        }
    }
    for (i = 0; i < graph->count; ++i) {
        if (summary.degrees[i] == 0) ++summary.isolated_count;
        if (summary.degrees[i] > summary.max_degree) summary.max_degree = summary.degrees[i];
    }
    return summary;
}

void show_map_summary(const Graph *graph)
{
    MapSummary summary = calculate_map_summary(graph);
    int i, j, count;
    printf("Locations: %d\nRoads: %d\nTotal road length: %lld m\n", graph->count,
           summary.road_count, summary.total_distance);
    printf("Average road length: %.2f m\n", summary.road_count ?
           (double)summary.total_distance / summary.road_count : 0.0);
    printf("Isolated locations: %d\n", summary.isolated_count);
    puts("Most connected location(s):");
    if (summary.max_degree == 0) puts("  None (no roads).");
    else for (i = 0; i < graph->count; ++i)
        if (summary.degrees[i] == summary.max_degree)
            printf("  %s (%d roads)\n", graph->locations[i].name, summary.max_degree);
    puts("Location types:");
    if (graph->count == 0) puts("  None.");
    for (i = 0; i < graph->count; ++i) {
        for (j = 0; j < i; ++j)
            if (strcmp(graph->locations[i].type, graph->locations[j].type) == 0) break;
        if (j != i) continue;
        count = 0;
        for (j = i; j < graph->count; ++j)
            if (strcmp(graph->locations[i].type, graph->locations[j].type) == 0) ++count;
        printf("  %s: %d\n", graph->locations[i].type, count);
    }
}
