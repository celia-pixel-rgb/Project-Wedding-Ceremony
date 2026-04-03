#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// PROPOSITION B — NESTED LINKED LIST (List of Lists)
// Changes: menus collapse after successful actions
// ============================================================

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

const char *GUEST_CSV = "guests.csv";
const char *PASSWORD  = "group3wed!";

const char* side_to_string(Side s) {
    return (s == GROOM) ? "Groom" : (s == BRIDE) ? "Bride" : "Both";
}

gboolean find_guest_by_id(int guest_id, Guest *out) {
    FILE *f = fopen(GUEST_CSV, "r");
    if (!f) return FALSE;
    char line[512]; Guest g;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &g.id, g.name, &g.age, g.status,
                   g.phone, (int*)&g.side, g.parking) == 7) {
            if (g.id == guest_id) { *out = g; fclose(f); return TRUE; }
        }
    }
    fclose(f);
    return FALSE;
}

gboolean guest_exists(int guest_id) {
    Guest g; return find_guest_by_id(guest_id, &g);
}

// ============================================================
// GUEST REFERENCE NODE
// ============================================================
typedef struct GuestRef {
    int guest_id;
    struct GuestRef *next;
} GuestRef;

GuestRef* create_guest_ref(int guest_id) {
    GuestRef *ref = (GuestRef*)malloc(sizeof(GuestRef));
    if (!ref) return NULL;
    ref->guest_id = guest_id;
    ref->next     = NULL;
    return ref;
}

void free_guest_refs(GuestRef **head) {
    GuestRef *curr = *head;
    while (curr) { GuestRef *n = curr->next; free(curr); curr = n; }
    *head = NULL;
}

// ============================================================
// CATEGORY
// ============================================================
typedef struct Category {
    int       id;
    char      code[50];
    GuestRef *guests;
    int       guest_count;
    struct Category *next;
} Category;

Category *category_head = NULL;
int       next_cat_id   = 0;
const char *CAT_CSV = "categories.csv";

Category* create_category(const char *code) {
    Category *c = (Category*)malloc(sizeof(Category));
    if (!c) return NULL;
    c->id = next_cat_id++;
    strncpy(c->code, code, sizeof(c->code) - 1);
    c->code[sizeof(c->code) - 1] = '\0';
    c->guests      = NULL;
    c->guest_count = 0;
    c->next        = NULL;
    return c;
}

void insert_category(Category **tete, Category *new_cat) {
    if (!new_cat) return;
    new_cat->next = *tete;
    *tete = new_cat;
}

void delete_category(Category **tete, int id) {
    Category *curr = *tete, *prev = NULL;
    while (curr) {
        if (curr->id == id) {
            if (prev) prev->next = curr->next;
            else      *tete = curr->next;
            free_guest_refs(&curr->guests);
            free(curr);
            return;
        }
        prev = curr; curr = curr->next;
    }
}

void update_category(Category *tete, int id, const char *new_code) {
    while (tete) {
        if (tete->id == id) {
            strncpy(tete->code, new_code, sizeof(tete->code) - 1);
            tete->code[sizeof(tete->code) - 1] = '\0';
            return;
        }
        tete = tete->next;
    }
}

int count_guest(Category *tete) {
    int total = 0;
    while (tete) { total += tete->guest_count; tete = tete->next; }
    return total;
}

void sort_categories_desc(Category **tete) {
    if (!*tete || !(*tete)->next) return;
    Category *sorted = NULL, *curr = *tete;
    while (curr) {
        Category *next_node = curr->next;
        curr->next = NULL;
        if (!sorted || curr->guest_count >= sorted->guest_count) {
            curr->next = sorted; sorted = curr;
        } else {
            Category *s = sorted;
            while (s->next && s->next->guest_count > curr->guest_count) s = s->next;
            curr->next = s->next; s->next = curr;
        }
        curr = next_node;
    }
    *tete = sorted;
}

void display_all_guests(Category *tete, GString *out) {
    while (tete) {
        g_string_append_printf(out,
            "\n[Category %d | %-20s]  (%d guest(s))\n",
            tete->id, tete->code, tete->guest_count);
        GuestRef *ref = tete->guests;
        while (ref) {
            Guest g;
            if (find_guest_by_id(ref->guest_id, &g))
                g_string_append_printf(out,
                    "  [GuestID %d] %-20s | Age: %-3d | %-10s | %-12s | %-5s | Parking: %s\n",
                    g.id, g.name, g.age, g.status,
                    g.phone, side_to_string(g.side), g.parking);
            else
                g_string_append_printf(out,
                    "  [GuestID %d] *** not found in guests.csv ***\n", ref->guest_id);
            ref = ref->next;
        }
        if (tete->guest_count == 0) g_string_append(out, "  (no guests)\n");
        tete = tete->next;
    }
}

gboolean assign_guest_to_category(Category *tete, int cat_id, int guest_id) {
    while (tete) {
        if (tete->id == cat_id) {
            if (!guest_exists(guest_id)) return FALSE;
            GuestRef *ref = tete->guests;
            while (ref) { if (ref->guest_id == guest_id) return FALSE; ref = ref->next; }
            GuestRef *new_ref = create_guest_ref(guest_id);
            if (!new_ref) return FALSE;
            if (!tete->guests) {
                tete->guests = new_ref;
            } else {
                GuestRef *tail = tete->guests;
                while (tail->next) tail = tail->next;
                tail->next = new_ref;
            }
            tete->guest_count++;
            return TRUE;
        }
        tete = tete->next;
    }
    return FALSE;
}

void remove_guest_from_category(Category *tete, int cat_id, int guest_id) {
    while (tete) {
        if (tete->id == cat_id) {
            GuestRef *curr = tete->guests, *prev = NULL;
            while (curr) {
                if (curr->guest_id == guest_id) {
                    if (prev) prev->next = curr->next;
                    else      tete->guests = curr->next;
                    free(curr);
                    tete->guest_count--;
                    return;
                }
                prev = curr; curr = curr->next;
            }
        }
        tete = tete->next;
    }
}

void free_list(Category **tete) {
    Category *curr = *tete;
    while (curr) {
        Category *n = curr->next;
        free_guest_refs(&curr->guests);
        free(curr); curr = n;
    }
    *tete = NULL;
}

// ============================================================
// CSV PERSISTENCE
// ============================================================
void save_categories_to_csv() {
    FILE *f = fopen(CAT_CSV, "w");
    if (!f) return;
    Category *c = category_head;
    while (c) {
        fprintf(f, "CAT,%d,%s,%d\n", c->id, c->code, c->guest_count);
        GuestRef *ref = c->guests;
        while (ref) { fprintf(f, "ID,%d\n", ref->guest_id); ref = ref->next; }
        c = c->next;
    }
    fclose(f);
}

void load_categories_from_csv() {
    free_list(&category_head);
    next_cat_id = 0;
    FILE *f = fopen(CAT_CSV, "r");
    if (!f) return;
    char line[256];
    Category *current = NULL, *last = NULL;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        if (strncmp(line, "CAT,", 4) == 0) {
            int id, gc; char code[50];
            if (sscanf(line + 4, "%d,%49[^,],%d", &id, code, &gc) == 3) {
                current = create_category(code);
                current->id = id;
                if (id >= next_cat_id) next_cat_id = id + 1;
                if (!category_head) { category_head = current; last = current; }
                else { last->next = current; last = current; }
            }
        } else if (strncmp(line, "ID,", 3) == 0 && current) {
            int gid;
            if (sscanf(line + 3, "%d", &gid) == 1) {
                GuestRef *ref = create_guest_ref(gid);
                if (!current->guests) {
                    current->guests = ref;
                } else {
                    GuestRef *tail = current->guests;
                    while (tail->next) tail = tail->next;
                    tail->next = ref;
                }
                current->guest_count++;
            }
        }
    }
    fclose(f);
}

// ============================================================
// GTK WIDGET GLOBALS
// ============================================================
GtkWidget *cat_code_entry;
GtkWidget *cat_fields_box, *toggle_cat_btn;          // collapsible create

GtkWidget *assign_cat_id_entry, *assign_guest_id_entry;
GtkWidget *assign_fields_box, *toggle_assign_btn;    // collapsible assign

GtkWidget *disp_text, *disp_password_entry;

GtkWidget *del_cat_id_entry, *del_password_entry;
GtkWidget *del_fields_box, *toggle_del_btn;          // collapsible delete cat

GtkWidget *rmv_cat_id_entry, *rmv_guest_id_entry, *rmv_password_entry;
GtkWidget *rmv_fields_box, *toggle_rmv_btn;          // collapsible remove guest

GtkWidget *upd_cat_id_entry, *upd_cat_code_entry, *upd_password_entry;
GtkWidget *upd_fields_box, *toggle_upd_btn;          // collapsible update

// ============================================================
// TOGGLE HELPERS
// ============================================================
static void toggle_box(GtkButton *btn, GtkWidget *box) {
    gboolean vis = gtk_widget_get_visible(box);
    gtk_widget_set_visible(box, !vis);
}

void on_toggle_cat    (GtkButton *b, gpointer d) { toggle_box(b, cat_fields_box);    gtk_button_set_label(b, gtk_widget_get_visible(cat_fields_box)    ? "? Close" : "? New Category"); }
void on_toggle_assign (GtkButton *b, gpointer d) { toggle_box(b, assign_fields_box); gtk_button_set_label(b, gtk_widget_get_visible(assign_fields_box) ? "? Close" : "?? Assign Guest"); }
void on_toggle_del    (GtkButton *b, gpointer d) { toggle_box(b, del_fields_box);    gtk_button_set_label(b, gtk_widget_get_visible(del_fields_box)    ? "? Close" : "?? Delete Category"); }
void on_toggle_rmv    (GtkButton *b, gpointer d) { toggle_box(b, rmv_fields_box);    gtk_button_set_label(b, gtk_widget_get_visible(rmv_fields_box)    ? "? Close" : "? Remove Guest"); }
void on_toggle_upd    (GtkButton *b, gpointer d) { toggle_box(b, upd_fields_box);    gtk_button_set_label(b, gtk_widget_get_visible(upd_fields_box)    ? "? Close" : "? Update Category"); }

// ============================================================
// CALLBACKS
// ============================================================
void cb_create_category(GtkButton *btn, gpointer data) {
    const char *code = gtk_editable_get_text(GTK_EDITABLE(cat_code_entry));
    if (!strlen(code)) return;
    Category *c = create_category(code);
    insert_category(&category_head, c);
    save_categories_to_csv();
    gtk_editable_set_text(GTK_EDITABLE(cat_code_entry), "");
    // Collapse after success
    gtk_widget_set_visible(cat_fields_box, FALSE);
    gtk_button_set_label(GTK_BUTTON(toggle_cat_btn), "? New Category");
    g_print("Created category ID %d\n", c->id);
}

void cb_assign_guest(GtkButton *btn, gpointer data) {
    int cat_id   = atoi(gtk_editable_get_text(GTK_EDITABLE(assign_cat_id_entry)));
    int guest_id = atoi(gtk_editable_get_text(GTK_EDITABLE(assign_guest_id_entry)));
    if (!guest_exists(guest_id)) { g_print("Guest ID %d not in guests.csv\n", guest_id); return; }
    if (!assign_guest_to_category(category_head, cat_id, guest_id)) {
        g_print("Could not assign: not found or duplicate\n"); return;
    }
    save_categories_to_csv();
    gtk_editable_set_text(GTK_EDITABLE(assign_cat_id_entry),   "");
    gtk_editable_set_text(GTK_EDITABLE(assign_guest_id_entry), "");
    // Collapse after success
    gtk_widget_set_visible(assign_fields_box, FALSE);
    gtk_button_set_label(GTK_BUTTON(toggle_assign_btn), "?? Assign Guest");
}

void refresh_display() {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(disp_password_entry));
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(disp_text));
    if (strcmp(pw, PASSWORD) != 0) {
        gtk_text_buffer_set_text(buf, "Enter password to view data.", -1); return;
    }
    int cat_count = 0;
    Category *c = category_head;
    while (c) { cat_count++; c = c->next; }
    GString *out = g_string_new("");
    g_string_append_printf(out,
        "NESTED LINKED LIST | Categories: %d | Total guests assigned: %d\n"
        "(No fixed guest limit per category)\n",
        cat_count, count_guest(category_head));
    display_all_guests(category_head, out);
    gtk_text_buffer_set_text(buf, out->str, -1);
    g_string_free(out, TRUE);
}

void cb_refresh(GtkButton *btn, gpointer data)          { refresh_display(); }
void cb_sort_and_display(GtkButton *btn, gpointer data) {
    sort_categories_desc(&category_head);
    save_categories_to_csv();
    refresh_display();
}

void cb_delete_category(GtkButton *btn, gpointer data) {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(del_password_entry));
    if (strcmp(pw, PASSWORD) != 0) return;
    delete_category(&category_head, atoi(gtk_editable_get_text(GTK_EDITABLE(del_cat_id_entry))));
    save_categories_to_csv();
    // Collapse after success
    gtk_editable_set_text(GTK_EDITABLE(del_cat_id_entry),   "");
    gtk_editable_set_text(GTK_EDITABLE(del_password_entry), "");
    gtk_widget_set_visible(del_fields_box, FALSE);
    gtk_button_set_label(GTK_BUTTON(toggle_del_btn), "?? Delete Category");
}

void cb_remove_guest(GtkButton *btn, gpointer data) {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(rmv_password_entry));
    if (strcmp(pw, PASSWORD) != 0) return;
    remove_guest_from_category(category_head,
        atoi(gtk_editable_get_text(GTK_EDITABLE(rmv_cat_id_entry))),
        atoi(gtk_editable_get_text(GTK_EDITABLE(rmv_guest_id_entry))));
    save_categories_to_csv();
    // Collapse after success
    gtk_editable_set_text(GTK_EDITABLE(rmv_cat_id_entry),   "");
    gtk_editable_set_text(GTK_EDITABLE(rmv_guest_id_entry), "");
    gtk_editable_set_text(GTK_EDITABLE(rmv_password_entry), "");
    gtk_widget_set_visible(rmv_fields_box, FALSE);
    gtk_button_set_label(GTK_BUTTON(toggle_rmv_btn), "? Remove Guest");
}

void cb_update_category(GtkButton *btn, gpointer data) {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(upd_password_entry));
    if (strcmp(pw, PASSWORD) != 0) return;
    update_category(category_head,
        atoi(gtk_editable_get_text(GTK_EDITABLE(upd_cat_id_entry))),
        gtk_editable_get_text(GTK_EDITABLE(upd_cat_code_entry)));
    save_categories_to_csv();
    // Collapse after success
    gtk_editable_set_text(GTK_EDITABLE(upd_cat_id_entry),   "");
    gtk_editable_set_text(GTK_EDITABLE(upd_cat_code_entry), "");
    gtk_editable_set_text(GTK_EDITABLE(upd_password_entry), "");
    gtk_widget_set_visible(upd_fields_box, FALSE);
    gtk_button_set_label(GTK_BUTTON(toggle_upd_btn), "? Update Category");
}

// ============================================================
// PAGE BUILDERS
// ============================================================
static GtkWidget* make_row(const char *label, GtkWidget *widget) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(row), gtk_label_new(label));
    gtk_box_append(GTK_BOX(row), widget);
    return row;
}

GtkWidget* create_category_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    toggle_cat_btn = gtk_button_new_with_label("? New Category");
    g_signal_connect(toggle_cat_btn, "clicked", G_CALLBACK(on_toggle_cat), NULL);
    gtk_box_append(GTK_BOX(box), toggle_cat_btn);

    cat_fields_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    cat_code_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(cat_code_entry), "e.g. VIP, Family, Friends");
    GtkWidget *btn = gtk_button_new_with_label("Create Category");
    g_signal_connect(btn, "clicked", G_CALLBACK(cb_create_category), NULL);
    gtk_box_append(GTK_BOX(cat_fields_box), gtk_label_new("Create a new Category"));
    gtk_box_append(GTK_BOX(cat_fields_box), make_row("Code:", cat_code_entry));
    gtk_box_append(GTK_BOX(cat_fields_box), btn);
    gtk_widget_set_visible(cat_fields_box, FALSE);
    gtk_box_append(GTK_BOX(box), cat_fields_box);
    return box;
}

GtkWidget* create_assign_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    toggle_assign_btn = gtk_button_new_with_label("?? Assign Guest");
    g_signal_connect(toggle_assign_btn, "clicked", G_CALLBACK(on_toggle_assign), NULL);
    gtk_box_append(GTK_BOX(box), toggle_assign_btn);

    assign_fields_box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    assign_cat_id_entry   = gtk_entry_new();
    assign_guest_id_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(assign_cat_id_entry),   "Category ID");
    gtk_entry_set_placeholder_text(GTK_ENTRY(assign_guest_id_entry), "Guest ID (from guests.csv)");
    GtkWidget *btn = gtk_button_new_with_label("Assign Guest to Category");
    g_signal_connect(btn, "clicked", G_CALLBACK(cb_assign_guest), NULL);
    GtkWidget *info = gtk_label_new(
        "Guest ID must exist in guests.csv.\n"
        "No limit on guests per category.\n"
        "Each guest reference is a linked list node.");
    gtk_box_append(GTK_BOX(assign_fields_box), gtk_label_new("Assign Guest ID to Category"));
    gtk_box_append(GTK_BOX(assign_fields_box), make_row("Category ID:", assign_cat_id_entry));
    gtk_box_append(GTK_BOX(assign_fields_box), make_row("Guest ID:",    assign_guest_id_entry));
    gtk_box_append(GTK_BOX(assign_fields_box), btn);
    gtk_box_append(GTK_BOX(assign_fields_box), info);
    gtk_widget_set_visible(assign_fields_box, FALSE);
    gtk_box_append(GTK_BOX(box), assign_fields_box);
    return box;
}

GtkWidget* create_display_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    disp_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(disp_password_entry), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(disp_password_entry), FALSE);
    GtkWidget *btn_refresh = gtk_button_new_with_label("Show All");
    GtkWidget *btn_sort    = gtk_button_new_with_label("Sort by Guest Count");
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(cb_refresh),         NULL);
    g_signal_connect(btn_sort,    "clicked", G_CALLBACK(cb_sort_and_display), NULL);
    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(btn_row), btn_refresh);
    gtk_box_append(GTK_BOX(btn_row), btn_sort);
    disp_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(disp_text), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(disp_text), TRUE);
    gtk_widget_set_vexpand(disp_text, TRUE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), disp_text);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(box), make_row("Password:", disp_password_entry));
    gtk_box_append(GTK_BOX(box), btn_row);
    gtk_box_append(GTK_BOX(box), scroll);
    return box;
}

GtkWidget* create_update_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    toggle_upd_btn = gtk_button_new_with_label("? Update Category");
    g_signal_connect(toggle_upd_btn, "clicked", G_CALLBACK(on_toggle_upd), NULL);
    gtk_box_append(GTK_BOX(box), toggle_upd_btn);

    upd_fields_box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    upd_cat_id_entry   = gtk_entry_new();
    upd_cat_code_entry = gtk_entry_new();
    upd_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(upd_cat_id_entry),   "Category ID");
    gtk_entry_set_placeholder_text(GTK_ENTRY(upd_cat_code_entry), "New Code");
    gtk_entry_set_placeholder_text(GTK_ENTRY(upd_password_entry), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(upd_password_entry), FALSE);
    GtkWidget *btn = gtk_button_new_with_label("Update Category");
    g_signal_connect(btn, "clicked", G_CALLBACK(cb_update_category), NULL);
    gtk_box_append(GTK_BOX(upd_fields_box), gtk_label_new("Update Category Code"));
    gtk_box_append(GTK_BOX(upd_fields_box), make_row("Category ID:", upd_cat_id_entry));
    gtk_box_append(GTK_BOX(upd_fields_box), make_row("New Code:",    upd_cat_code_entry));
    gtk_box_append(GTK_BOX(upd_fields_box), make_row("Password:",    upd_password_entry));
    gtk_box_append(GTK_BOX(upd_fields_box), btn);
    gtk_widget_set_visible(upd_fields_box, FALSE);
    gtk_box_append(GTK_BOX(box), upd_fields_box);
    return box;
}

GtkWidget* create_delete_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    // --- Delete Category (collapsible) ---
    toggle_del_btn = gtk_button_new_with_label("?? Delete Category");
    g_signal_connect(toggle_del_btn, "clicked", G_CALLBACK(on_toggle_del), NULL);
    gtk_box_append(GTK_BOX(box), toggle_del_btn);

    del_fields_box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    del_cat_id_entry   = gtk_entry_new();
    del_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(del_cat_id_entry),   "Category ID");
    gtk_entry_set_placeholder_text(GTK_ENTRY(del_password_entry), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(del_password_entry), FALSE);
    GtkWidget *btn_del = gtk_button_new_with_label("Delete Category");
    g_signal_connect(btn_del, "clicked", G_CALLBACK(cb_delete_category), NULL);
    gtk_box_append(GTK_BOX(del_fields_box), make_row("Category ID:", del_cat_id_entry));
    gtk_box_append(GTK_BOX(del_fields_box), make_row("Password:",    del_password_entry));
    gtk_box_append(GTK_BOX(del_fields_box), btn_del);
    gtk_widget_set_visible(del_fields_box, FALSE);
    gtk_box_append(GTK_BOX(box), del_fields_box);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // --- Remove Guest from Category (collapsible) ---
    toggle_rmv_btn = gtk_button_new_with_label("? Remove Guest");
    g_signal_connect(toggle_rmv_btn, "clicked", G_CALLBACK(on_toggle_rmv), NULL);
    gtk_box_append(GTK_BOX(box), toggle_rmv_btn);

    rmv_fields_box     = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    rmv_cat_id_entry   = gtk_entry_new();
    rmv_guest_id_entry = gtk_entry_new();
    rmv_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(rmv_cat_id_entry),   "Category ID");
    gtk_entry_set_placeholder_text(GTK_ENTRY(rmv_guest_id_entry), "Guest ID");
    gtk_entry_set_placeholder_text(GTK_ENTRY(rmv_password_entry), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(rmv_password_entry), FALSE);
    GtkWidget *btn_rmv = gtk_button_new_with_label("Remove Guest from Category");
    g_signal_connect(btn_rmv, "clicked", G_CALLBACK(cb_remove_guest), NULL);
    gtk_box_append(GTK_BOX(rmv_fields_box), gtk_label_new("Remove Guest from Category"));
    gtk_box_append(GTK_BOX(rmv_fields_box), make_row("Category ID:", rmv_cat_id_entry));
    gtk_box_append(GTK_BOX(rmv_fields_box), make_row("Guest ID:",    rmv_guest_id_entry));
    gtk_box_append(GTK_BOX(rmv_fields_box), make_row("Password:",    rmv_password_entry));
    gtk_box_append(GTK_BOX(rmv_fields_box), btn_rmv);
    gtk_widget_set_visible(rmv_fields_box, FALSE);
    gtk_box_append(GTK_BOX(box), rmv_fields_box);

    return box;
}

// ============================================================
// MAIN
// ============================================================
void activate(GtkApplication *app, gpointer data) {
    load_categories_from_csv();
    GtkWidget *win      = gtk_application_window_new(app);
    GtkWidget *stack    = gtk_stack_new();
    GtkWidget *switcher = gtk_stack_switcher_new();
    gtk_window_set_title(GTK_WINDOW(win), "Wedding – Category Manager [Nested List]");
    gtk_window_set_default_size(GTK_WINDOW(win), 680, 520);
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
    gtk_widget_set_hexpand(stack, TRUE);
    gtk_widget_set_vexpand(stack, TRUE);
    gtk_stack_add_titled(GTK_STACK(stack), create_category_page(), "create",  "New Category");
    gtk_stack_add_titled(GTK_STACK(stack), create_assign_page(),   "assign",  "Assign Guest");
    gtk_stack_add_titled(GTK_STACK(stack), create_display_page(),  "display", "Display");
    gtk_stack_add_titled(GTK_STACK(stack), create_update_page(),   "update",  "Update");
    gtk_stack_add_titled(GTK_STACK(stack), create_delete_page(),   "delete",  "Delete");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(box), switcher);
    gtk_box_append(GTK_BOX(box), stack);
    gtk_window_set_child(GTK_WINDOW(win), box);
    gtk_window_present(GTK_WINDOW(win));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.example.categorymanager.nested",
                                               G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    free_list(&category_head);
    return status;
}
