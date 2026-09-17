#include "tree_manager.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

/* 1: complete line, 0: EOF, -1: overlong line. Drain rejected input. */
static int read_line(FILE *stream, char *buffer, int capacity)
{
    int ch;
    size_t length;
    if (fgets(buffer, capacity, stream) == NULL) return 0;
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
    } else {
        ch = fgetc(stream);
        if (ch != '\n' && ch != EOF) {
            while ((ch = fgetc(stream)) != '\n' && ch != EOF) { }
            return -1;
        }
    }
    if (length > 0 && buffer[length - 1] == '\r') buffer[length - 1] = '\0';
    return 1;
}

static char *trim(char *text)
{
    char *end;
    while (isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int parse_integer(const char *text, int minimum, int maximum, int *value)
{
    char *end;
    long parsed;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (text == end || errno == ERANGE) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0' || parsed < minimum || parsed > maximum) return 0;
    *value = (int)parsed;
    return 1;
}

static int parse_diameter(const char *text, float *value)
{
    char *end;
    float parsed;
    errno = 0;
    parsed = strtof(text, &end);
    if (text == end || errno == ERANGE) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0' || !isfinite(parsed) || parsed <= 0.0f) return 0;
    *value = parsed;
    return 1;
}

int read_integer(const char *prompt, int minimum, int maximum, int *value)
{
    char buffer[256];
    int status;
    for (;;) {
        fputs(prompt, stdout);
        fflush(stdout);
        status = read_line(stdin, buffer, (int)sizeof buffer);
        if (status == 0) return 0;
        if (status == 1 && parse_integer(buffer, minimum, maximum, value)) return 1;
        printf("Invalid input. Enter an integer from %d to %d.\n", minimum, maximum);
    }
}

static int valid_text(const char *text, size_t capacity)
{
    const unsigned char *p = (const unsigned char *)text;
    if (*text == '\0' || strlen(text) >= capacity || strchr(text, ',') != NULL) return 0;
    for (; *p; ++p) if (iscntrl(*p)) return 0;
    return 1;
}

static int read_text(const char *prompt, char *destination, size_t capacity)
{
    char buffer[256];
    char *value;
    int status;
    for (;;) {
        fputs(prompt, stdout);
        fflush(stdout);
        status = read_line(stdin, buffer, (int)sizeof buffer);
        if (status == 0) return 0;
        value = trim(buffer);
        if (status == 1 && valid_text(value, capacity)) {
            memcpy(destination, value, strlen(value) + 1);
            return 1;
        }
        printf("Invalid text. Use 1-%zu bytes, without commas or control characters.\n", capacity - 1);
    }
}

static int find_id(const Tree trees[], int count, int id)
{
    int i;
    for (i = 0; i < count; ++i) if (trees[i].id == id) return i;
    return -1;
}

static const char *health_name(int health)
{
    switch (health) {
    case 1: return "Healthy";
    case 2: return "Average";
    case 3: return "Poor";
    default: return "Unknown";
    }
}

static void print_header(void)
{
    printf("%-10s | %-49s | %-99s | %13s | %-7s\n",
           "ID", "Species", "Location", "Diameter(cm)", "Health");
}

static void print_tree(const Tree *tree)
{
    printf("%-10d | %-49s | %-99s | %13.2f | %-7s\n",
           tree->id, tree->species, tree->location, (double)tree->diameter,
           health_name(tree->health));
}

void add_tree(Tree trees[], int *count)
{
    Tree tree;
    char buffer[256];
    int status;
    if (*count >= MAX_TREES) { puts("Storage full (100 trees)."); return; }
    for (;;) {
        if (!read_integer("ID: ", 1, INT_MAX, &tree.id)) goto cancelled;
        if (find_id(trees, *count, tree.id) < 0) break;
        puts("Duplicate ID. Please use a unique ID.");
    }
    if (!read_text("Species: ", tree.species, sizeof tree.species) ||
        !read_text("Location: ", tree.location, sizeof tree.location)) goto cancelled;
    for (;;) {
        fputs("Diameter (cm): ", stdout);
        fflush(stdout);
        status = read_line(stdin, buffer, (int)sizeof buffer);
        if (status == 0) goto cancelled;
        if (status == 1 && parse_diameter(buffer, &tree.diameter)) break;
        puts("Invalid diameter. Enter a finite number greater than 0.");
    }
    if (!read_integer("Health (1=Healthy, 2=Average, 3=Poor): ", 1, 3, &tree.health)) goto cancelled;
    trees[(*count)++] = tree;
    puts("Tree added.");
    return;
cancelled:
    puts("Input ended. Incomplete tree was not added.");
}

void list_trees(const Tree trees[], int count)
{
    int i;
    if (count == 0) { puts("No tree records."); return; }
    print_header();
    for (i = 0; i < count; ++i) print_tree(&trees[i]);
}

void search_tree(const Tree trees[], int count)
{
    int option, id, i, found;
    char species[50];
    for (;;) {
        puts("\n1. Search by ID\n2. Search by species\n0. Back");
        if (!read_integer("Select an option: ", 0, 2, &option) || option == 0) return;
        found = 0;
        if (option == 1) {
            if (!read_integer("ID: ", 1, INT_MAX, &id)) return;
            i = find_id(trees, count, id);
            if (i >= 0) { print_header(); print_tree(&trees[i]); found = 1; }
        } else {
            if (!read_text("Species (exact, case-sensitive): ", species, sizeof species)) return;
            for (i = 0; i < count; ++i) {
                if (strcmp(trees[i].species, species) == 0) {
                    if (!found) print_header();
                    print_tree(&trees[i]);
                    found = 1;
                }
            }
        }
        if (!found) puts("No matching trees.");
    }
}

void sort_by_diameter(Tree trees[], int count)
{
    int i, j;
    for (i = 0; i < count - 1; ++i) {
        for (j = 0; j < count - 1 - i; ++j) {
            if (trees[j].diameter < trees[j + 1].diameter) {
                Tree temporary = trees[j];
                trees[j] = trees[j + 1];
                trees[j + 1] = temporary;
            }
        }
    }
    list_trees(trees, count);
}

void show_statistics(const Tree trees[], int count)
{
    int health_counts[3] = {0, 0, 0};
    int i;
    float maximum = 0.0f;
    double sum = 0.0;
    printf("Total trees: %d\n", count);
    if (count == 0) { puts("No tree records."); return; }
    for (i = 0; i < count; ++i) {
        ++health_counts[trees[i].health - 1];
        sum += trees[i].diameter;
        if (trees[i].diameter > maximum) maximum = trees[i].diameter;
    }
    for (i = 0; i < 3; ++i)
        printf("%s: %d (%.2f%%)\n", health_name(i + 1), health_counts[i], 100.0 * health_counts[i] / count);
    printf("Average diameter: %.2f cm\n", sum / count);
    puts("Largest diameter tree(s):");
    print_header();
    for (i = 0; i < count; ++i) if (trees[i].diameter == maximum) print_tree(&trees[i]);
}

static int ensure_data_directory(void)
{
#ifdef _WIN32
    int result = _mkdir("data");
#else
    int result = mkdir("data", 0777);
#endif
    if (result == 0 || errno == EEXIST) return 1;
    perror("Cannot create data directory");
    return 0;
}

int save_to_file(const Tree trees[], int count, const char *filename)
{
    FILE *file;
    int i, ok = 1;
    if (!ensure_data_directory()) return 0;
    file = fopen(filename, "w");
    if (file == NULL) { perror("Cannot open data file for writing"); return 0; }
    for (i = 0; i < count; ++i) {
        /* Nine significant digits preserve a binary32 float across reloads. */
        if (fprintf(file, "%d,%s,%s,%.9g,%d\n", trees[i].id, trees[i].species,
                    trees[i].location, (double)trees[i].diameter, trees[i].health) < 0) {
            ok = 0;
            break;
        }
    }
    if (fclose(file) != 0) ok = 0;
    if (ok) printf("Saved %d tree(s).\n", count);
    else fputs("Save failed while writing data.\n", stderr);
    return ok;
}

static int parse_record(char *line, Tree *tree)
{
    char *fields[5];
    char *separator;
    int i;
    fields[0] = line;
    for (i = 0; i < 4; ++i) {
        separator = strchr(fields[i], ',');
        if (separator == NULL) return 0;
        *separator = '\0';
        fields[i + 1] = separator + 1;
    }
    if (strchr(fields[4], ',') != NULL) return 0;
    for (i = 0; i < 5; ++i) fields[i] = trim(fields[i]);
    if (!parse_integer(fields[0], 1, INT_MAX, &tree->id) ||
        !valid_text(fields[1], sizeof tree->species) ||
        !valid_text(fields[2], sizeof tree->location) ||
        !parse_diameter(fields[3], &tree->diameter) ||
        !parse_integer(fields[4], 1, 3, &tree->health)) return 0;
    memcpy(tree->species, fields[1], strlen(fields[1]) + 1);
    memcpy(tree->location, fields[2], strlen(fields[2]) + 1);
    return 1;
}

int load_from_file(Tree trees[], int *count, const char *filename)
{
    FILE *file;
    char line[512];
    Tree tree;
    int status, failed;
    unsigned long line_number = 0;
    *count = 0;
    if (!ensure_data_directory()) return -1;
    file = fopen(filename, "r");
    if (file == NULL) {
        if (errno == ENOENT) { puts("No data file found. Starting empty."); return 0; }
        perror("Cannot open data file for reading");
        return -1;
    }
    while ((status = read_line(file, line, (int)sizeof line)) != 0) {
        ++line_number;
        if (status < 0 || !parse_record(line, &tree)) {
            fprintf(stderr, "Skipped line %lu: invalid record.\n", line_number);
        } else if (find_id(trees, *count, tree.id) >= 0) {
            fprintf(stderr, "Skipped line %lu: duplicate ID.\n", line_number);
        } else if (*count == MAX_TREES) {
            fprintf(stderr, "Skipped line %lu: capacity limit (100).\n", line_number);
        } else {
            trees[(*count)++] = tree;
        }
    }
    failed = ferror(file);
    if (fclose(file) != 0) failed = 1;
    if (failed) { fputs("Error reading data file.\n", stderr); return -1; }
    printf("Loaded %d tree(s).\n", *count);
    return 1;
}
