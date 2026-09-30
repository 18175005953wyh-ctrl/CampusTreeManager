#include "tree_manager.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    Tree trees[MAX_TREES];
    int count = 0;
    int dirty = 0;
    int option;
    if (load_from_file(trees, &count, DATA_FILE) < 0) {
        fputs("Startup failed; existing data will not be overwritten.\n", stderr);
        return EXIT_FAILURE;
    }
    for (;;) {
        puts("\n===== Campus Tree Manager =====\n"
             "1. Add tree\n2. Edit tree\n3. Delete tree\n4. List all trees\n"
             "5. Search tree\n6. Sort by diameter\n7. Show statistics\n8. Save data\n0. Exit");
        if (!read_integer("Select an option: ", 0, 8, &option) || option == 0) {
            if (!dirty) { puts("No unsaved changes. Goodbye."); return EXIT_SUCCESS; }
            puts("Saving before exit...");
            return save_to_file(trees, count, DATA_FILE) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        switch (option) {
        case 1: dirty |= add_tree(trees, &count); break;
        case 2: dirty |= edit_tree(trees, count); break;
        case 3: dirty |= delete_tree(trees, &count); break;
        case 4: list_trees(trees, count); break;
        case 5: search_tree(trees, count); break;
        case 6: dirty |= sort_by_diameter(trees, count); break;
        case 7: show_statistics(trees, count); break;
        case 8: if (save_to_file(trees, count, DATA_FILE)) dirty = 0; break;
        default: break;
        }
    }
}
