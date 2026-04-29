#ifndef CATEGORY_H
#define CATEGORY_H

#include "person.h"   /* Guest, Side, side_to_string */

// The GuestRef struct represents one node in the inner linked list.
// Each node stores the ID of a guest assigned to a category.
typedef struct GuestRef {
    int guest_id;
    struct GuestRef *next;
} GuestRef;

// The Category struct represents one node in the outer linked list.
// Each category has a name (code), a guest count, and a nested
// linked list of GuestRef nodes for the guests assigned to it.
typedef struct Category {
    int       id;
    char      code[50];
    GuestRef *guests;       /* Head of the inner GuestRef list for this category */
    int       guest_count;
    struct Category *next;
} Category;

// Function Declarations (The Prototypes)
// We only tell the computer these exist here.
// The actual logic goes into category.c

// Menu actions -- called from category.c's own main()
void action_create_category(void);
void action_assign_guest(void);
void action_display(void);
void action_sort_display(void);
void action_update_category(void);
void action_delete_category(void);
void action_remove_guest(void);

// Persistence -- load and save categories to disk
void load_categories_from_csv(void);
void save_categories_to_csv(void);

#endif /* CATEGORY_H */
