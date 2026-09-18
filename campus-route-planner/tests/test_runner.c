#include "graph.h"
#include "input.h"
#include "pathfinder.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static const char *fixtures = "tests/fixtures";
static int passed = 0, failed = 0;

#define REQUIRE(condition) do { if (!(condition)) { \
    printf("  Assertion failed at line %d: %s\n", __LINE__, #condition); return 0; } } while (0)

static int load_fixture(Graph *graph, const char *roads)
{
    char path[1024];
    graph_init(graph);
    if (snprintf(path, sizeof path, "%s/test_locations.csv", fixtures) >= (int)sizeof path) return 0;
    if (!load_locations(graph, path)) return 0;
    if (roads == NULL) return 1;
    if (snprintf(path, sizeof path, "%s/%s", fixtures, roads) >= (int)sizeof path) return 0;
    return load_roads(graph, path);
}

static int write_fixture(const char *content)
{
    FILE *file = fopen("route-test-generated.csv", "w");
    int ok;
    if (file == NULL) return 0;
    ok = fputs(content, file) >= 0;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int valid_path(const Graph *graph, PathResult result, int start, int end)
{
    int i, j, sum = 0;
    if (!result.found || result.path_length < 1 || result.path_length > graph->count ||
        result.path[0] != start || result.path[result.path_length - 1] != end) return 0;
    for (i = 0; i < result.path_length; ++i) {
        if (result.path[i] < 0 || result.path[i] >= graph->count) return 0;
        for (j = 0; j < i; ++j) if (result.path[i] == result.path[j]) return 0;
        if (i > 0) {
            int weight = graph->adjacency[result.path[i - 1]][result.path[i]];
            if (weight == INF_DISTANCE) return 0;
            sum += weight;
        }
    }
    return sum == result.distance;
}

static int test_initialization(void)
{
    Graph graph;
    graph_init(&graph);
    REQUIRE(graph.count == 0);
    REQUIRE(graph.adjacency[0][0] == 0);
    REQUIRE(graph.adjacency[0][49] == INF_DISTANCE);
    return 1;
}

static int test_locations(void)
{
    Graph graph;
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(graph.count == 5);
    REQUIRE(strcmp(graph.locations[1].name, "Library") == 0);
    return 1;
}

static int test_roads(void)
{
    Graph graph;
    REQUIRE(load_fixture(&graph, "test_roads.csv"));
    REQUIRE(graph.adjacency[0][1] == 5 && graph.adjacency[1][0] == 5);
    REQUIRE(graph.adjacency[0][0] == 0 && graph.adjacency[0][4] == INF_DISTANCE);
    return 1;
}

static int test_id_mapping(void)
{
    Graph graph;
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(find_location_index_by_id(&graph, 30) == 2);
    REQUIRE(find_location_index_by_id(&graph, 3) == -1);
    return 1;
}

static int test_direct_route(void)
{
    Graph graph;
    PathResult result;
    REQUIRE(load_fixture(&graph, "test_roads.csv"));
    result = dijkstra_shortest_path(&graph, 0, 1);
    REQUIRE(result.distance == 5 && result.path_length == 2);
    REQUIRE(valid_path(&graph, result, 0, 1));
    return 1;
}

static int test_multi_step(void)
{
    Graph graph;
    PathResult result;
    REQUIRE(load_fixture(&graph, "test_roads.csv"));
    result = dijkstra_shortest_path(&graph, 0, 3);
    REQUIRE(result.distance == 15 && result.path_length == 4);
    REQUIRE(result.path[1] == 1 && result.path[2] == 2);
    REQUIRE(valid_path(&graph, result, 0, 3));
    return 1;
}

static int test_same_location(void)
{
    Graph graph;
    PathResult result;
    REQUIRE(load_fixture(&graph, "test_roads.csv"));
    result = dijkstra_shortest_path(&graph, 4, 4);
    REQUIRE(result.found && result.distance == 0 && result.path_length == 1 && result.path[0] == 4);
    return 1;
}

static int test_disconnected(void)
{
    Graph graph;
    PathResult result;
    REQUIRE(load_fixture(&graph, "disconnected_roads.csv"));
    result = dijkstra_shortest_path(&graph, 0, 3);
    REQUIRE(!result.found && result.distance == INF_DISTANCE && result.path_length == 0);
    return 1;
}

static int test_isolated(void)
{
    Graph graph;
    REQUIRE(load_fixture(&graph, "test_roads.csv"));
    REQUIRE(!dijkstra_shortest_path(&graph, 0, 4).found);
    REQUIRE(!dijkstra_shortest_path(&graph, 4, 0).found);
    return 1;
}

static int test_invalid_indices(void)
{
    Graph graph;
    graph_init(&graph);
    REQUIRE(!dijkstra_shortest_path(&graph, 0, 0).found);
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(!dijkstra_shortest_path(&graph, -1, 2).found);
    REQUIRE(!dijkstra_shortest_path(&graph, 0, graph.count).found);
    return 1;
}

static int test_search(void)
{
    Graph graph;
    int matches[MAX_LOCATIONS];
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(find_location_matches(&graph, "lIbRa", matches) == 1 && matches[0] == 1);
    REQUIRE(find_location_matches(&graph, "a", matches) == 5);
    REQUIRE(find_location_matches(&graph, "missing", matches) == 0);
    return 1;
}

static int test_empty_search(void)
{
    Graph graph;
    int matches[MAX_LOCATIONS];
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(find_location_matches(&graph, "", matches) == 0);
    REQUIRE(find_location_matches(&graph, " \t ", matches) == 0);
    return 1;
}

static int test_invalid_road_api(void)
{
    Graph graph;
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(!graph_add_road(&graph, 10, 20, -5));
    REQUIRE(!graph_add_road(&graph, 10, 20, 0));
    REQUIRE(!graph_add_road(&graph, 10, 10, 5));
    REQUIRE(!graph_add_road(&graph, 10, 999, 5));
    REQUIRE(!graph_add_road(&graph, 10, 20, MAX_ROAD_DISTANCE + 1));
    REQUIRE(graph.adjacency[0][1] == INF_DISTANCE);
    REQUIRE(graph.adjacency[0][0] == 0);
    return 1;
}

static int test_invalid_road_csv(void)
{
    Graph graph;
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(write_fixture("10,20,-1\n10,20,0\n10,99,3\n10,10,4\nwrong\n10,20,9,extra\n10,20,999999999999999\n\n20,30,8\n"));
    REQUIRE(load_roads(&graph, "route-test-generated.csv"));
    REQUIRE(graph.adjacency[0][1] == INF_DISTANCE && graph.adjacency[1][2] == 8);
    REQUIRE(calculate_map_summary(&graph).road_count == 1);
    return 1;
}

static int test_invalid_location_csv(void)
{
    Graph graph;
    graph_init(&graph);
    REQUIRE(write_fixture("\n10,Valid,Study\n10,Duplicate,Study\n0,Zero,Study\nabc,Wrong,Study\n20,,Study\n20,X,Y,Z\n30,Next,Education\n"));
    REQUIRE(load_locations(&graph, "route-test-generated.csv"));
    REQUIRE(graph.count == 2 && graph.locations[1].id == 30);
    return 1;
}

static int test_duplicate_roads(void)
{
    Graph graph;
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(write_fixture("10,20,12\n20,10,7\n10,20,9\n"));
    REQUIRE(load_roads(&graph, "route-test-generated.csv"));
    REQUIRE(graph.adjacency[0][1] == 7 && graph.adjacency[1][0] == 7);
    REQUIRE(calculate_map_summary(&graph).road_count == 1);
    return 1;
}

static int test_summary(void)
{
    Graph graph;
    MapSummary summary;
    REQUIRE(load_fixture(&graph, "test_roads.csv"));
    summary = calculate_map_summary(&graph);
    REQUIRE(summary.road_count == 5 && summary.total_distance == 85);
    REQUIRE(summary.isolated_count == 1 && summary.max_degree == 3);
    REQUIRE(summary.degrees[1] == 3 && summary.degrees[3] == 3);
    return 1;
}

static int test_empty_summary(void)
{
    Graph graph;
    MapSummary summary;
    graph_init(&graph);
    summary = calculate_map_summary(&graph);
    REQUIRE(summary.road_count == 0 && summary.total_distance == 0 && summary.max_degree == 0);
    REQUIRE(summary.isolated_count == 0);
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(calculate_map_summary(&graph).isolated_count == 5);
    return 1;
}

static int test_location_capacity(void)
{
    Graph graph;
    int i;
    graph_init(&graph);
    for (i = 1; i <= MAX_LOCATIONS; ++i) REQUIRE(graph_add_location(&graph, i, "Place", "Type"));
    REQUIRE(!graph_add_location(&graph, 51, "Overflow", "Type"));
    REQUIRE(!graph_add_location(&graph, 1, "Duplicate", "Type"));
    REQUIRE(graph.count == MAX_LOCATIONS);
    return 1;
}

static int test_csv_capacity(void)
{
    Graph graph;
    FILE *file = fopen("route-test-generated.csv", "w");
    int i;
    REQUIRE(file != NULL);
    for (i = 1; i <= 51; ++i) fprintf(file, "%d,Place,Type\n", i);
    REQUIRE(fclose(file) == 0);
    graph_init(&graph);
    REQUIRE(load_locations(&graph, "route-test-generated.csv"));
    REQUIRE(graph.count == 50 && find_location_index_by_id(&graph, 51) == -1);
    return 1;
}

static int test_max_distance_chain(void)
{
    Graph graph;
    PathResult result;
    int i;
    graph_init(&graph);
    for (i = 1; i <= MAX_LOCATIONS; ++i) REQUIRE(graph_add_location(&graph, i, "Place", "Type"));
    for (i = 1; i < MAX_LOCATIONS; ++i) REQUIRE(graph_add_road(&graph, i, i + 1, MAX_ROAD_DISTANCE));
    result = dijkstra_shortest_path(&graph, 0, MAX_LOCATIONS - 1);
    REQUIRE(result.distance == MAX_ROAD_DISTANCE * (MAX_LOCATIONS - 1));
    REQUIRE(result.path_length == MAX_LOCATIONS && valid_path(&graph, result, 0, MAX_LOCATIONS - 1));
    return 1;
}

static int test_large_summary(void)
{
    Graph graph;
    int i, j;
    MapSummary summary;
    graph_init(&graph);
    for (i = 1; i <= MAX_LOCATIONS; ++i) REQUIRE(graph_add_location(&graph, i, "Place", "Type"));
    for (i = 1; i <= MAX_LOCATIONS; ++i)
        for (j = i + 1; j <= MAX_LOCATIONS; ++j) REQUIRE(graph_add_road(&graph, i, j, MAX_ROAD_DISTANCE));
    summary = calculate_map_summary(&graph);
    REQUIRE(summary.road_count == 1225);
    REQUIRE(summary.total_distance == 1225LL * MAX_ROAD_DISTANCE && summary.total_distance > INT_MAX);
    return 1;
}

static int test_equal_routes(void)
{
    Graph graph;
    PathResult first, second;
    REQUIRE(load_fixture(&graph, NULL));
    REQUIRE(graph_add_road(&graph, 10, 20, 5));
    REQUIRE(graph_add_road(&graph, 10, 30, 5));
    REQUIRE(graph_add_road(&graph, 20, 40, 5));
    REQUIRE(graph_add_road(&graph, 30, 40, 5));
    first = dijkstra_shortest_path(&graph, 0, 3);
    second = dijkstra_shortest_path(&graph, 0, 3);
    REQUIRE(first.distance == 10 && first.path[1] == 1);
    REQUIRE(first.path[1] == second.path[1] && valid_path(&graph, first, 0, 3));
    return 1;
}

static int test_integer_validation(void)
{
    int value;
    REQUIRE(parse_integer(" 42 \t", 1, 100, &value) && value == 42);
    REQUIRE(!parse_integer("", 1, 100, &value));
    REQUIRE(!parse_integer("4x", 1, 100, &value));
    REQUIRE(!parse_integer("0", 1, 100, &value));
    REQUIRE(!parse_integer("999999999999999999999999", 1, INT_MAX, &value));
    return 1;
}

static int test_long_line_recovery(void)
{
    char line[8];
    FILE *file;
    REQUIRE(write_fixture("01234567890123456789\n42\n"));
    file = fopen("route-test-generated.csv", "r");
    REQUIRE(file != NULL);
    REQUIRE(read_line(file, line, (int)sizeof line) == -1);
    REQUIRE(read_line(file, line, (int)sizeof line) == 1 && strcmp(line, "42") == 0);
    REQUIRE(read_line(file, line, (int)sizeof line) == 0);
    REQUIRE(fclose(file) == 0);
    return 1;
}

static int test_bom_and_whitespace(void)
{
    Graph graph;
    graph_init(&graph);
    REQUIRE(write_fixture("\xEF\xBB\xBF" " 10 , Main Gate , Entrance \n \t\n20,Library,Study"));
    REQUIRE(load_locations(&graph, "route-test-generated.csv"));
    REQUIRE(graph.count == 2 && strcmp(graph.locations[0].name, "Main Gate") == 0);
    return 1;
}

static int test_bad_labels(void)
{
    Graph graph;
    char text[NAME_LENGTH + 1];
    graph_init(&graph);
    memset(text, 'a', sizeof text - 1); text[sizeof text - 1] = '\0';
    REQUIRE(!graph_add_location(&graph, 1, text, "Type"));
    REQUIRE(!graph_add_location(&graph, 1, " ", "Type"));
    REQUIRE(!graph_add_location(&graph, 1, "A,B", "Type"));
    REQUIRE(!graph_add_location(&graph, 1, "Name", "T\t"));
    text[NAME_LENGTH - 1] = '\0';
    REQUIRE(graph_add_location(&graph, 1, text, "Type"));
    return 1;
}

static int test_missing_files(void)
{
    Graph graph;
    graph_init(&graph);
    REQUIRE(!load_locations(&graph, "route-test-missing-dir/locations.csv"));
    REQUIRE(!load_roads(&graph, "route-test-missing-dir/roads.csv"));
    return 1;
}

static int test_reference_all_pairs(void)
{
    Graph graph;
    int reference[8][8], i, j, k;
    PathResult result;
    graph_init(&graph);
    for (i = 0; i < 8; ++i) REQUIRE(graph_add_location(&graph, i + 10, "Place", "Type"));
    for (i = 0; i < 7; ++i)
        for (j = i + 1; j < 7; ++j)
            if ((i + j) % 3 != 0) REQUIRE(graph_add_road(&graph, i + 10, j + 10, (i * 17 + j * 11) % 40 + 1));
    for (i = 0; i < 8; ++i) for (j = 0; j < 8; ++j) reference[i][j] = graph.adjacency[i][j];
    /* Independent Floyd-Warshall reference checks every source/destination pair. */
    for (k = 0; k < 8; ++k) for (i = 0; i < 8; ++i) for (j = 0; j < 8; ++j)
        if (reference[i][k] < INF_DISTANCE && reference[k][j] < INF_DISTANCE &&
            reference[i][k] + reference[k][j] < reference[i][j]) reference[i][j] = reference[i][k] + reference[k][j];
    for (i = 0; i < 8; ++i) for (j = 0; j < 8; ++j) {
        result = dijkstra_shortest_path(&graph, i, j);
        REQUIRE(result.distance == reference[i][j]);
        if (reference[i][j] < INF_DISTANCE) REQUIRE(valid_path(&graph, result, i, j));
        else REQUIRE(!result.found);
    }
    return 1;
}

static void run_test(const char *name, int (*test)(void))
{
    if (test()) { ++passed; printf("[PASS] %s\n", name); }
    else { ++failed; printf("[FAIL] %s\n", name); }
}

int main(int argc, char *argv[])
{
    if (argc == 2) fixtures = argv[1];
    else if (argc != 1) { fputs("Usage: route_tests [FIXTURE_DIRECTORY]\n", stderr); return 1; }
    run_test("graph initialization", test_initialization);
    run_test("load locations", test_locations);
    run_test("load roads", test_roads);
    run_test("ID to index mapping", test_id_mapping);
    run_test("direct shortest path", test_direct_route);
    run_test("multi-step shortest path and reconstruction", test_multi_step);
    run_test("same location", test_same_location);
    run_test("disconnected graph", test_disconnected);
    run_test("isolated location", test_isolated);
    run_test("invalid indices and empty graph", test_invalid_indices);
    run_test("case-insensitive multiple matches", test_search);
    run_test("empty keyword", test_empty_search);
    run_test("invalid road API", test_invalid_road_api);
    run_test("invalid road CSV", test_invalid_road_csv);
    run_test("invalid location CSV", test_invalid_location_csv);
    run_test("duplicate roads keep shortest", test_duplicate_roads);
    run_test("summary counts each road once", test_summary);
    run_test("empty summary", test_empty_summary);
    run_test("location capacity", test_location_capacity);
    run_test("CSV capacity", test_csv_capacity);
    run_test("maximum-distance 50-vertex chain", test_max_distance_chain);
    run_test("64-bit total road length", test_large_summary);
    run_test("stable equal-cost paths", test_equal_routes);
    run_test("integer validation", test_integer_validation);
    run_test("overlong line recovery", test_long_line_recovery);
    run_test("BOM and whitespace", test_bom_and_whitespace);
    run_test("invalid text fields", test_bad_labels);
    run_test("missing files", test_missing_files);
    run_test("all pairs versus independent reference", test_reference_all_pairs);
    (void)remove("route-test-generated.csv");
    printf("\n%d tests passed, %d tests failed.\n", passed, failed);
    return failed ? 1 : 0;
}
