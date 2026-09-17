#ifndef TREE_MANAGER_H
#define TREE_MANAGER_H

#define MAX_TREES 100
#define DATA_FILE "data/trees.csv"

typedef struct {
    int id;
    char species[50];
    char location[100];
    float diameter;
    int health;
} Tree;

/* Returns 0 on end of input; otherwise retries until a valid integer. */
int read_integer(const char *prompt, int minimum, int maximum, int *value);
void add_tree(Tree trees[], int *count);
void list_trees(const Tree trees[], int count);
void search_tree(const Tree trees[], int count);
void sort_by_diameter(Tree trees[], int count);
void show_statistics(const Tree trees[], int count);
int save_to_file(const Tree trees[], int count, const char *filename);
/* 1: loaded (possibly with skipped rows); 0: missing; -1: I/O error. */
int load_from_file(Tree trees[], int *count, const char *filename);

#endif
