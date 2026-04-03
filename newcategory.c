/*
 * newcategory.c  --  Wedding Category Manager (improved version)
 * Original author: group3wed
 *
 * Manages wedding guest categories using a nested linked list:
 *   - Outer list  : Category nodes
 *   - Inner list  : GuestRef nodes inside each Category (stores guest IDs only)
 *
 * Category data is saved to / loaded from categories.csv format:
 *   CAT,<id>,<code>,<guest_count>
 *   ID,<guest_id>        (one line per assigned guest)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { GROOM, BRIDE, BOTH } Side;

typedef struct {
    int  id;
    char name[100];
    int  age;
    char status[50];
    char phone[20];
    Side side;
    char parking[10];
} Guest;

static const char *GUEST_CSV = "guests.csv";
static const char *PASSWORD  = "group3wed!";
static const char *CAT_CSV   = "categories.csv";
static const char *TMP_CSV   = "categories_tmp.csv"; /* Temp file for safe saves */

/*
 * flush_stdin -- Discard leftover characters in the input buffer.
 * Called after a read to prevent stale input from corrupting the next read.
 */
static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void read_line(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    fflush(stdout);                      /* Force the prompt to appear before the user types */
    if (fgets(buf, size, stdin)) {
        if (buf[strlen(buf) - 1] != '\n') {
            /* The input was longer than the buffer -- flush the rest */
            flush_stdin();
        }
        buf[strcspn(buf, "\n")] = '\0';  /* strcspn finds the '\n' position and replaces it with '\0' */
    }
}

static int read_int(const char *prompt) {
    char buf[32];
    read_line(prompt, buf, sizeof(buf));
    return atoi(buf);
}

static int check_password(void) {
    char pw[64];
    read_line("Password: ", pw, sizeof(pw));
    return strcmp(pw, PASSWORD) == 0;
}

const char *side_to_string(Side s) {
    return (s == GROOM) ? "Groom" : (s == BRIDE) ? "Bride" : "Both";
}

/* -- Guest lookup ----------------------------------------------------------- */

static int find_guest_by_id(int guest_id, Guest *out) {
    FILE *f = fopen(GUEST_CSV, "r");
    if (!f) return 0;
    char  line[512];
    Guest g;
    while (fgets(line, sizeof(line), f)) {
        /* [^,] means "read characters until a comma" -- handles names/values with spaces */
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &g.id, g.name, &g.age, g.status,
                   g.phone, (int *)&g.side, g.parking) == 7) {
            if (g.id == guest_id) {
                *out = g;
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

static int guest_exists(int guest_id) {
    Guest g;
    return find_guest_by_id(guest_id, &g);  /* Reuse full lookup, discard the result */
}

/* -- GuestRef -- inner linked-list node ------------------------------------ */

typedef struct GuestRef {
    int guest_id;
    struct GuestRef *next;
} GuestRef;

static GuestRef *create_guest_ref(int guest_id) {
    GuestRef *ref = (GuestRef *)malloc(sizeof(GuestRef));
    if (!ref) return NULL;
    ref->guest_id = guest_id;
    ref->next     = NULL;
    return ref;
}

static void free_guest_refs(GuestRef **head) {
    GuestRef *curr = *head;
    while (curr) {
        GuestRef *n = curr->next;
        free(curr);
        curr = n;
    }
    *head = NULL;
}

static void append_guest_ref(GuestRef **head, GuestRef *ref) {
    if (!*head) { *head = ref; return; }  /* Empty list: ref becomes the first node */
    GuestRef *tail = *head;
    while (tail->next) tail = tail->next; /* Walk to the last node before appending */
    tail->next = ref;
}

/* -- Category -- outer linked-list node ------------------------------------ */

typedef struct Category {
    int       id;
    char      code[50];
    GuestRef *guests;       /* Head of the inner GuestRef list for this category */
    int       guest_count;
    struct Category *next;
} Category;

static Category *category_head = NULL;
static int       next_cat_id   = 0;  /* Auto-increment counter for category IDs */

static Category *create_category(const char *code) {
    Category *c = (Category *)malloc(sizeof(Category));
    if (!c) return NULL;
    c->id = next_cat_id++;  /* Assign current value then increment for next use */
    strncpy(c->code, code, sizeof(c->code) - 1);
    c->code[sizeof(c->code) - 1] = '\0';  /* strncpy does not guarantee null-termination when truncating */
    c->guests      = NULL;
    c->guest_count = 0;
    c->next        = NULL;
    return c;
}

static void insert_category(Category **head, Category *new_cat) {
    if (!new_cat) return;
    /* Insert at the head -- O(1), no need to walk the whole list */
    new_cat->next = *head;
    *head = new_cat;
}

static void delete_category(Category **head, int id) {
    Category *curr = *head, *prev = NULL;
    while (curr) {
        if (curr->id == id) {
            /* Unlink: make the previous node skip over curr */
            if (prev) prev->next = curr->next;
            else      *head = curr->next;  /* curr was the head */
            free_guest_refs(&curr->guests); /* Free inner list before freeing the node */
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

static void update_category(Category *head, int id, const char *new_code) {
    if (!new_code || !new_code[0]) { printf("Code cannot be empty.\n"); return; }
    while (head) {
        if (head->id == id) {
            strncpy(head->code, new_code, sizeof(head->code) - 1);
            head->code[sizeof(head->code) - 1] = '\0';
            return;
        }
        head = head->next;
    }
    printf("Category ID %d not found.\n", id);
}

static int count_total_guests(Category *head) {
    int total = 0;
    while (head) { total += head->guest_count; head = head->next; }
    return total;
}

static void sort_categories_desc(Category **head) {
    if (!*head || !(*head)->next) return;

    Category *sorted = NULL;  /* Builds the new sorted list one node at a time */
    Category *curr   = *head;

    while (curr) {
        Category *next_node = curr->next;  /* Save next before we rewire curr->next */
        curr->next = NULL;

        if (!sorted || curr->guest_count >= sorted->guest_count) {
            /* curr has the most guests so far -- place it at the front */
            curr->next = sorted;
            sorted = curr;
        } else {
            /* Find the correct insertion point inside the already-sorted list */
            Category *s = sorted;
            while (s->next && s->next->guest_count > curr->guest_count)
                s = s->next;
            curr->next = s->next;
            s->next = curr;
        }
        curr = next_node;
    }
    *head = sorted;
}

static void display_all_guests(Category *head) {
    if (!head) { printf("  (no categories)\n"); return; }
    while (head) {
        printf("\n[Category %d | %-20s]  (%d guest(s))\n",
               head->id, head->code, head->guest_count);
        GuestRef *ref = head->guests;
        while (ref) {
            Guest g;
            if (find_guest_by_id(ref->guest_id, &g))
                printf("  [GuestID %d] %-20s | Age: %-3d | %-10s | %-12s | %-5s | Parking: %s\n",
                       g.id, g.name, g.age, g.status,
                       g.phone, side_to_string(g.side), g.parking);
            else
                /* Guest ID is in the category but no longer exists in guests.csv */
                printf("  [GuestID %d] *** not found in guests.csv ***\n", ref->guest_id);
            ref = ref->next;
        }
        if (head->guest_count == 0) printf("  (no guests)\n");
        head = head->next;
    }
}

/*
 * assign_guest_to_category -- returns distinct codes so the caller can print
 * a precise error message instead of one generic failure message:
 *   1  = success
 *   0  = category not found or malloc failed
 *  -1  = guest does not exist in guests.csv
 *  -2  = guest already assigned to this category (duplicate blocked)
 */
static int assign_guest_to_category(Category *head, int cat_id, int guest_id) {
    while (head) {
        if (head->id == cat_id) {
            if (!guest_exists(guest_id)) return -1;

            /* Walk the inner list to check for duplicates before inserting */
            GuestRef *ref = head->guests;
            while (ref) {
                if (ref->guest_id == guest_id) return -2;
                ref = ref->next;
            }

            GuestRef *new_ref = create_guest_ref(guest_id);
            if (!new_ref) return 0;  /* malloc failed */
            append_guest_ref(&head->guests, new_ref);
            head->guest_count++;
            return 1;
        }
        head = head->next;
    }
    return 0;  /* Category not found */
}

static void remove_guest_from_category(Category *head, int cat_id, int guest_id) {
    while (head) {
        if (head->id == cat_id) {
            GuestRef *curr = head->guests, *prev = NULL;
            while (curr) {
                if (curr->guest_id == guest_id) {
                    /* Unlink the node from the inner list */
                    if (prev) prev->next = curr->next;
                    else      head->guests = curr->next;
                    free(curr);
                    head->guest_count--;
                    return;
                }
                prev = curr;
                curr = curr->next;
            }
            printf("Guest ID %d not found in category %d.\n", guest_id, cat_id);
            return;
        }
        head = head->next;
    }
    printf("Category ID %d not found.\n", cat_id);
}

static void free_list(Category **head) {
    Category *curr = *head;
    while (curr) {
        Category *n = curr->next;
        free_guest_refs(&curr->guests);  /* Always free the inner list before the outer node */
        free(curr);
        curr = n;
    }
    *head = NULL;
}

/* -- Persistence ------------------------------------------------------------ */

static void save_categories_to_csv(void) {
    /* Write to a temp file first, then rename over the real file.
       This prevents data corruption if the program crashes mid-write. */
    FILE *f = fopen(TMP_CSV, "w");
    if (!f) {
        perror("Could not open temp categories file");
        return;
    }

    Category *c = category_head;
    while (c) {
        fprintf(f, "CAT,%d,%s,%d\n", c->id, c->code, c->guest_count);
        GuestRef *ref = c->guests;
        while (ref) { fprintf(f, "ID,%d\n", ref->guest_id); ref = ref->next; }
        c = c->next;
    }
    fclose(f);

    remove(CAT_CSV);
    if (rename(TMP_CSV, CAT_CSV) != 0)  /* rename() returns non-zero on failure */
        perror("Warning: could not finalise categories file");
}

static void load_categories_from_csv(void) {
    free_list(&category_head);  /* Clear any existing in-memory data before loading */
    next_cat_id = 0;

    FILE *f = fopen(CAT_CSV, "r");
    if (!f) return;  /* No file yet -- start with an empty list */

    char      line[256];
    Category *last    = NULL;  /* Tail pointer so we can append in O(1) */
    Category *current = NULL;  /* The category currently being loaded */

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "CAT,", 4) == 0) {
            int id, gc; char code[50];
            if (sscanf(line + 4, "%d,%49[^,],%d", &id, code, &gc) == 3) {
                current = create_category(code);
                if (!current) continue;  /* malloc failed: skip this row safely */
                current->id = id;
                /* Keep next_cat_id ahead of the highest ID seen in the file */
                if (id >= next_cat_id) next_cat_id = id + 1;
                if (!category_head) { category_head = current; last = current; }
                else                { last->next = current; last = current; }
            }
        } else if (strncmp(line, "ID,", 3) == 0 && current) {
            /* ID lines belong to the most recently parsed CAT line */
            int gid;
            if (sscanf(line + 3, "%d", &gid) == 1) {
                GuestRef *ref = create_guest_ref(gid);
                if (!ref) continue;  /* malloc failed: skip this row safely */
                append_guest_ref(&current->guests, ref);
                current->guest_count++;
            }
        }
    }
    fclose(f);
}

/* -- Menu actions ----------------------------------------------------------- */

static void action_create_category(void) {
    char code[50];
    read_line("Category code (e.g. VIP, Family): ", code, sizeof(code));
    if (!strlen(code)) { printf("Code cannot be empty.\n"); return; }
    Category *c = create_category(code);
    if (!c) { printf("Memory error.\n"); return; }
    insert_category(&category_head, c);
    save_categories_to_csv();
    printf("Created category ID %d (%s).\n", c->id, c->code);
}

static void action_assign_guest(void) {
    int cat_id   = read_int("Category ID: ");
    int guest_id = read_int("Guest ID: ");
    int result   = assign_guest_to_category(category_head, cat_id, guest_id);

    /* Each return code maps to a specific, informative error message */
    if (result == 1) {
        save_categories_to_csv();
        printf("Guest %d assigned to category %d.\n", guest_id, cat_id);
    } else if (result == -1) {
        printf("Guest ID %d not found in guests.csv.\n", guest_id);
    } else if (result == -2) {
        printf("Guest %d is already assigned to category %d.\n", guest_id, cat_id);
    } else {
        printf("Category ID %d not found.\n", cat_id);
    }
}

static void action_display(void) {
    if (!check_password()) { printf("Incorrect password.\n"); return; }
    int cat_count = 0;
    Category *c = category_head;
    while (c) { cat_count++; c = c->next; }
    printf("\nNESTED LINKED LIST | Categories: %d | Total guests assigned: %d\n",
           cat_count, count_total_guests(category_head));
    display_all_guests(category_head);
    printf("\n");
}

static void action_sort_display(void) {
    if (!check_password()) { printf("Incorrect password.\n"); return; }
    sort_categories_desc(&category_head);
    save_categories_to_csv();  /* Persist the new sorted order to disk */
    int cat_count = 0;
    Category *c = category_head;
    while (c) { cat_count++; c = c->next; }
    printf("\n[Sorted by guest count desc] Categories: %d | Total: %d\n",
           cat_count, count_total_guests(category_head));
    display_all_guests(category_head);
    printf("\n");
}

static void action_update_category(void) {
    if (!check_password()) { printf("Incorrect password.\n"); return; }
    int cat_id = read_int("Category ID to update: ");
    char new_code[50];
    read_line("New code: ", new_code, sizeof(new_code));
    update_category(category_head, cat_id, new_code);
    save_categories_to_csv();
    printf("Category %d updated.\n", cat_id);
}

static void action_delete_category(void) {
    if (!check_password()) { printf("Incorrect password.\n"); return; }
    int cat_id = read_int("Category ID to delete: ");
    delete_category(&category_head, cat_id);
    save_categories_to_csv();
    printf("Category %d deleted.\n", cat_id);
}

static void action_remove_guest(void) {
    if (!check_password()) { printf("Incorrect password.\n"); return; }
    int cat_id   = read_int("Category ID: ");
    int guest_id = read_int("Guest ID to remove: ");
    remove_guest_from_category(category_head, cat_id, guest_id);
    save_categories_to_csv();
    printf("Guest %d removed from category %d.\n", guest_id, cat_id);
}

/* -- Entry point ------------------------------------------------------------ */

int main(void) {
    load_categories_from_csv();  /* Restore saved state from disk before showing the menu */

    int choice;
    do {
        printf("\nWedding -- Category Manager\n");
        printf("1. Create category\n");
        printf("2. Assign guest to category\n");
        printf("3. Display all categories & guests\n");
        printf("4. Sort categories by guest count & display\n");
        printf("5. Update category code\n");
        printf("6. Delete category\n");
        printf("7. Remove guest from category\n");
        printf("0. Exit\n");
        choice = read_int("Choice: ");

        switch (choice) {
            case 1: action_create_category(); break;
            case 2: action_assign_guest();    break;
            case 3: action_display();         break;
            case 4: action_sort_display();    break;
            case 5: action_update_category(); break;
            case 6: action_delete_category(); break;
            case 7: action_remove_guest();    break;
            case 0: printf("Goodbye!\n");     break;
            default: printf("Invalid choice.\n"); break;
        }
    } while (choice != 0);

    free_list(&category_head);  /* Free all heap memory before exiting */
    return 0;
}
