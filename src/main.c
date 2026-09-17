#include "tree_manager.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    Tree trees[MAX_TREES];
    int count = 0;
    int option;
    if (load_from_file(trees, &count, DATA_FILE) < 0) {
        fputs("Startup failed; existing data will not be overwritten.\n", stderr);
        return EXIT_FAILURE;
    }
    for (;;) {
        puts("\n===== Campus Tree Manager =====\n"
             "1. Add tree\n2. List all trees\n3. Search tree\n"
             "4. Sort by diameter\n5. Show statistics\n6. Save data\n0. Exit");
        if (!read_integer("Select an option: ", 0, 6, &option) || option == 0) {
            puts("Saving before exit...");
            return save_to_file(trees, count, DATA_FILE) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        switch (option) {
        case 1: add_tree(trees, &count); break;
        case 2: list_trees(trees, count); break;
        case 3: search_tree(trees, count); break;
        case 4: sort_by_diameter(trees, count); break;
        case 5: show_statistics(trees, count); break;
        case 6: (void)save_to_file(trees, count, DATA_FILE); break;
        default: break;
        }
    }
}
