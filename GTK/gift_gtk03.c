/*
 * gift_gtk03.c  --  GTK 4 GUI for the Wedding Gift Manager  (group3wed)
 *
 * Compile (GTK 4):
 *   gcc gift_gtk03.c -o gift_management $(pkg-config --cflags --libs gtk4) -lm
 *
 * Changes from gift_gtk02.c:
 *  - Register Gift  : NO password required. Identity verified by NAME only
 *                     (Guest ID field removed). ID is looked up from guests.csv.
 *  - Display Gifts  : Password required. Full info shown:
 *                     Guest Name | Guest ID | Category | Gift Item | Qty | Total.
 *  - Delete Gift    : Password required. Identity by NAME only (no ID field).
 *                     Wrong name prompts the user to re-enter.
 *  - Update Gift    : Password required. Identity by NAME only (no ID field).
 *                     Total auto-calculated on gift/qty change.
 *                     CSV and Display tab refreshed immediately on success.
 *
 * GTK 4 compatibility:
 *  gtk_dialog_*        -> inline password entry + flag pattern
 *  gtk_message_dialog  -> show_alert() plain GtkWindow
 *  gtk_widget_show     -> gtk_window_present()
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* =========================================================================
 * Constants & catalogue
 * ========================================================================= */

#define GIFT_ITEM_COUNT 25
#define EUR_RATE        655.957

static const char *PERSON_CSV = "persons.csv";
static const char *CAT_CSV   = "category.csv";
static const char *GIFT_CSV  = "gifts.csv";
static const char *TMP_GIFT  = "gifts_tmp.csv";
static const char *PASSWORD  = "group3wed!";

typedef struct { int id; const char *name; double price_fcfa; } GiftItem;

static const GiftItem CATALOGUE[GIFT_ITEM_COUNT] = {
    {  1, "House",                       45000000.0 },
    {  2, "Car",                          8500000.0 },
    {  3, "Pots Set",                       35000.0 },
    {  4, "Spoon Set",                       8000.0 },
    {  5, "Fork Set",                        8000.0 },
    {  6, "Dishes Set",                     25000.0 },
    {  7, "Glass Set",                      15000.0 },
    {  8, "Microwave",                      55000.0 },
    {  9, "Customized Photo Frame",          12000.0 },
    { 10, "Travel Tickets (per person)",   350000.0 },
    { 11, "Hotel Reservation (per night)", 120000.0 },
    { 12, "Wine (bottle)",                  18000.0 },
    { 13, "Smoothing Iron",                 22000.0 },
    { 14, "Bedside Table",                  45000.0 },
    { 15, "Cushion Set",                    20000.0 },
    { 16, "Curtain Set",                    30000.0 },
    { 17, "Television (43\")",             180000.0 },
    { 18, "Flower Pots",                     7000.0 },
    { 19, "Kettle",                          15000.0 },
    { 20, "Cups Set",                        10000.0 },
    { 21, "Sheet Set",                       28000.0 },
    { 22, "Jewelry Set",                    150000.0 },
    { 23, "Couples Watches",               120000.0 },
    { 24, "Home Decor Set",                  40000.0 },
    { 25, "Gas Stove",                       65000.0 },
};

/* =========================================================================
 * Data types
 * ========================================================================= */

typedef struct {
    int  id;
    char name[100];
    char status[50];
} GuestBrief;

typedef struct {
    int    record_id;
    int    guest_id;
    char   guest_name[100];
    char   category[50];
    int    item_id;
    char   item_name[100];
    int    quantity;
    double total_fcfa;
    double total_eur;
    char   thanked_by[100];
} GiftRecord;

/* =========================================================================
 * CSV helpers
 * ========================================================================= */

/*
 * find_guest_by_name -- looks up a guest by full name ONLY (case-insensitive).
 * The ID is read automatically from guests.csv; no ID entry needed from user.
 * Returns 1 and fills *out on success; 0 if not found.
 */
static int find_guest_by_name(const char *name, GuestBrief *out) {
    FILE *f = fopen(PERSON_CSV, "r");
    if (!f) return 0;
    char b[100];
    int i;
    for (i = 0; name[i] && i < 99; i++)
        b[i] = (char)tolower((unsigned char)name[i]);
    b[i] = '\0';
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        GuestBrief g;
        char age_s[16], phone[20], side_s[10], parking[10];
        if (sscanf(line, "%d,%99[^,],%15[^,],%49[^,],%19[^,],%9[^,],%9s",
                   &g.id, g.name, age_s, g.status,
                   phone, side_s, parking) >= 2) {
            char a[100];
            for (i = 0; g.name[i] && i < 99; i++)
                a[i] = (char)tolower((unsigned char)g.name[i]);
            a[i] = '\0';
            if (strcmp(a, b) == 0) {
                *out = g;
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

static void find_category_for_guest(int guest_id,
                                    char *out_code, int size) {
    FILE *f = fopen(CAT_CSV, "r");
    strncpy(out_code, "Unassigned", size - 1);
    out_code[size - 1] = '\0';
    if (!f) return;
    char line[256], current[50] = {0};
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        if (strncmp(line, "CAT,", 4) == 0) {
            int id, gc;
            if (sscanf(line + 4, "%d,%49[^,],%d",
                       &id, current, &gc) < 2)
                current[0] = '\0';
        } else if (strncmp(line, "ID,", 3) == 0) {
            int gid;
            if (sscanf(line + 3, "%d", &gid) == 1
                    && gid == guest_id) {
                strncpy(out_code, current, size - 1);
                out_code[size - 1] = '\0';
                break;
            }
        }
    }
    fclose(f);
}

static int get_next_gift_id(void) {
    FILE *f = fopen(GIFT_CSV, "r");
    if (!f) return 1;
    int max_id = 0, id;
    char line[640];
    while (fgets(line, sizeof(line), f))
        if (sscanf(line, "%d,", &id) == 1 && id > max_id)
            max_id = id;
    fclose(f);
    return max_id + 1;
}

static void write_gift_line(FILE *f, const GiftRecord *g) {
    fprintf(f, "%d,%d,%s,%s,%d,%s,%d,%.2f,%.2f,%s\n",
            g->record_id, g->guest_id,
            g->guest_name, g->category,
            g->item_id, g->item_name,
            g->quantity,
            g->total_fcfa, g->total_eur,
            g->thanked_by);
}

static int parse_gift_line(const char *line, GiftRecord *g) {
    return sscanf(line,
        "%d,%d,%99[^,],%49[^,],%d,%99[^,],%d,%lf,%lf,%99[^\n]",
        &g->record_id, &g->guest_id,
        g->guest_name, g->category,
        &g->item_id, g->item_name,
        &g->quantity,
        &g->total_fcfa, &g->total_eur,
        g->thanked_by) == 10;
}

/* =========================================================================
 * GTK 4 alert window  (replaces removed gtk_message_dialog_new)
 *
 * Uses a plain GtkWindow with a label and OK button.
 * gtk_window_present() is the GTK 4 replacement for gtk_widget_show().
 * ========================================================================= */

static void on_alert_ok(GtkButton *btn, gpointer win) {
    (void)btn;
    gtk_window_destroy(GTK_WINDOW(win));
}

/* Unused in the current flow but kept for future use. */
static void show_alert(GtkWidget *parent_widget, const char *msg) {
    GtkWindow *parent = NULL;
    if (parent_widget)
        parent = GTK_WINDOW(gtk_widget_get_root(parent_widget));

    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Notice");
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
    gtk_window_set_default_size(GTK_WINDOW(win), 340, 130);
    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(win), parent);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(win), box);

    GtkWidget *lbl = gtk_label_new(msg);
    gtk_label_set_wrap(GTK_LABEL(lbl), TRUE);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.5f);
    gtk_box_append(GTK_BOX(box), lbl);

    GtkWidget *btn = gtk_button_new_with_label("OK");
    gtk_widget_set_halign(btn, GTK_ALIGN_CENTER);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_alert_ok), win);
    gtk_box_append(GTK_BOX(box), btn);

    gtk_window_present(GTK_WINDOW(win));   /* GTK 4 way */
}

/* =========================================================================
 * Shared helpers
 * ========================================================================= */

/*
 * check_pw_entry validate the password entry on a tab.
 * Returns 1 if correct; updates the status label either way.
 */
static int check_pw_entry(GtkWidget *pw_entry,
                           GtkWidget *pw_status_lbl) {
    const char *pw = gtk_editable_get_text(
        GTK_EDITABLE(pw_entry));
    if (strcmp(pw, PASSWORD) == 0) {
        gtk_label_set_text(GTK_LABEL(pw_status_lbl),
                           "Password accepted ");
        return 1;
    }
    gtk_label_set_text(GTK_LABEL(pw_status_lbl),
                       "Wrong password please try again.");
    gtk_editable_set_text(GTK_EDITABLE(pw_entry), "");
    return 0;
}

/* update_price_labels recalculate unit price and total. */
static void update_price_labels(GtkDropDown   *combo,
                                GtkSpinButton *spin,
                                GtkLabel      *lbl_unit,
                                GtkLabel      *lbl_total) {
    guint idx = gtk_drop_down_get_selected(combo);
    if (idx >= (guint)GIFT_ITEM_COUNT) return;
    double unit  = CATALOGUE[idx].price_fcfa;
    int    qty   = (int)gtk_spin_button_get_value(spin);
    double total = unit * qty;
    char buf[160];
    snprintf(buf, sizeof(buf),
             "Unit price:  %.0f FCFA  (%.2f EUR)",
             unit, unit / EUR_RATE);
    gtk_label_set_text(lbl_unit, buf);
    snprintf(buf, sizeof(buf),
             "Total:  %.0f FCFA  (%.2f EUR)",
             total, total / EUR_RATE);
    gtk_label_set_text(lbl_total, buf);
}

/* build_catalogue_list builds the GtkStringList shown in dropdowns. */
static GtkStringList *build_catalogue_list(void) {
    static char labels[GIFT_ITEM_COUNT][96];
    const char *ptrs[GIFT_ITEM_COUNT + 1];
    for (int i = 0; i < GIFT_ITEM_COUNT; i++) {
        snprintf(labels[i], sizeof(labels[i]),
                 "%2d. %-35s %10.0f FCFA  (%7.2f EUR)",
                 CATALOGUE[i].id, CATALOGUE[i].name,
                 CATALOGUE[i].price_fcfa,
                 CATALOGUE[i].price_fcfa / EUR_RATE);
        ptrs[i] = labels[i];
    }
    ptrs[GIFT_ITEM_COUNT] = NULL;
    return gtk_string_list_new(ptrs);
}

/* Suppress unused-function warning for show_alert (used on demand). */
static void _suppress_unused(void) { (void)show_alert; }

/* =========================================================================
 * TAB 1 = Register a gift
 * ========================================================================= */

typedef struct {
    /* No password on Register tab */
    GtkWidget  *entry_name;
    GtkWidget  *lbl_guest_status;
    GuestBrief  verified_guest;
    int         guest_ok;
    GtkWidget  *combo_item;
    GtkWidget  *lbl_unit_price;
    GtkWidget  *spin_qty;
    GtkWidget  *lbl_total;
    GtkWidget  *entry_thanked;
    GtkWidget  *lbl_result;
} RegisterWidgets;

/* Register tab: identity by name only, no password required */
static void on_reg_verify(GtkButton *b, gpointer d) {
    (void)b;
    RegisterWidgets *w = (RegisterWidgets *)d;
    const char *nm = gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    if (strlen(nm) < 1) {
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
                           "Please enter your full name.");
        return;
    }
    if (find_guest_by_name(nm, &w->verified_guest)) {
        w->guest_ok = 1;
        char msg[160];
        snprintf(msg, sizeof(msg), "Verified: %s",
                 w->verified_guest.name);
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), msg);
    } else {
        w->guest_ok = 0;
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
            "No guest found with that name. "
            "Please re-enter the correct and corresponding name.");
    }
}

static void on_reg_item_changed(GObject *o, GParamSpec *ps, gpointer d) {
    (void)ps;
    RegisterWidgets *w = (RegisterWidgets *)d;
    update_price_labels(GTK_DROP_DOWN(o),
                        GTK_SPIN_BUTTON(w->spin_qty),
                        GTK_LABEL(w->lbl_unit_price),
                        GTK_LABEL(w->lbl_total));
}

static void on_reg_qty_changed(GtkSpinButton *s, gpointer d) {
    (void)s;
    RegisterWidgets *w = (RegisterWidgets *)d;
    update_price_labels(GTK_DROP_DOWN(w->combo_item),
                        GTK_SPIN_BUTTON(w->spin_qty),
                        GTK_LABEL(w->lbl_unit_price),
                        GTK_LABEL(w->lbl_total));
}

static void on_reg_submit(GtkButton *b, gpointer d) {
    (void)b;
    RegisterWidgets *w = (RegisterWidgets *)d;
    if (!w->guest_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Please verify your identity first."); return;
    }
    guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(w->combo_item));
    if (idx >= (guint)GIFT_ITEM_COUNT) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Please select a gift item."); return;
    }
    const GiftItem *chosen = &CATALOGUE[idx];
    int qty = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(w->spin_qty));
    if (qty < 1) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Quantity must be at least 1."); return;
    }
    const char *thanked =
        gtk_editable_get_text(GTK_EDITABLE(w->entry_thanked));

    GiftRecord g;
    memset(&g, 0, sizeof(g));
    g.record_id  = get_next_gift_id();
    g.guest_id   = w->verified_guest.id;
    strncpy(g.guest_name, w->verified_guest.name,
            sizeof(g.guest_name) - 1);
    find_category_for_guest(g.guest_id,
                            g.category, sizeof(g.category));
    g.item_id    = chosen->id;
    strncpy(g.item_name, chosen->name, sizeof(g.item_name) - 1);
    g.quantity   = qty;
    g.total_fcfa = chosen->price_fcfa * qty;
    g.total_eur  = g.total_fcfa / EUR_RATE;
    if (strlen(thanked) >= 1)
        strncpy(g.thanked_by, thanked, sizeof(g.thanked_by) - 1);
    else
        strncpy(g.thanked_by, w->verified_guest.name,
                sizeof(g.thanked_by) - 1);

    FILE *f = fopen(GIFT_CSV, "a");
    if (!f) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Error: could not open gifts.csv"); return;
    }
    write_gift_line(f, &g);
    fclose(f);

    char msg[300];
    snprintf(msg, sizeof(msg),
             "Gift registered!  Record ID: %d\n"
             "%s  x%d  =  %.0f FCFA  (%.2f EUR)\n"
             "Thank you, %s!",
             g.record_id, g.item_name, g.quantity,
             g.total_fcfa, g.total_eur, g.thanked_by);
    gtk_label_set_text(GTK_LABEL(w->lbl_result), msg);

    /* Reset for next guest */
    w->guest_ok = 0;
    gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),    "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_thanked), "");
}

/* Small helper macro: bold section label, left-aligned. */
#define SECTION_LBL(box, txt) \
    do { \
        GtkWidget *_h = gtk_label_new(NULL); \
        gtk_label_set_markup(GTK_LABEL(_h), "<b>" txt "</b>"); \
        gtk_label_set_xalign(GTK_LABEL(_h), 0.0f); \
        gtk_box_append(GTK_BOX(box), _h); \
    } while (0)

static GtkWidget *build_register_tab(RegisterWidgets *w) {
    memset(w, 0, sizeof(*w));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    /* Info: no password needed here */
    GtkWidget *note = gtk_label_new(
        "No password required to register a gift. "
        "Enter your full name to verify your identity.");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(note), TRUE);
    gtk_box_append(GTK_BOX(box), note);
    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Step 1 = Identity by name only */
    SECTION_LBL(box, "Step 1 : Verify your identity (Full Name)");
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_box_append(GTK_BOX(box), grid);

    gtk_grid_attach(GTK_GRID(grid),
                    gtk_label_new("Full Name:"), 0, 0, 1, 1);
    w->entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name),
                                   "Exactly as registered");
    gtk_widget_set_hexpand(w->entry_name, TRUE);
    gtk_grid_attach(GTK_GRID(grid), w->entry_name, 1, 0, 1, 1);

    GtkWidget *bv = gtk_button_new_with_label("Verify Identity");
    g_signal_connect(bv, "clicked", G_CALLBACK(on_reg_verify), w);
    gtk_grid_attach(GTK_GRID(grid), bv, 1, 1, 1, 1);

    w->lbl_guest_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_guest_status), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_guest_status), TRUE);
    gtk_box_append(GTK_BOX(box), w->lbl_guest_status);
    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Step 2 = Gift selection */
    SECTION_LBL(box, "Step 2 : Choose your gift");
    w->combo_item = gtk_drop_down_new(
        G_LIST_MODEL(build_catalogue_list()), NULL);
    gtk_widget_set_hexpand(w->combo_item, TRUE);
    gtk_box_append(GTK_BOX(box), w->combo_item);

    w->lbl_unit_price = gtk_label_new("Unit price: ");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_unit_price), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_unit_price);

    GtkWidget *qty_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), qty_row);
    gtk_box_append(GTK_BOX(qty_row), gtk_label_new("Quantity:"));
    w->spin_qty = gtk_spin_button_new_with_range(1, 9999, 1);
    gtk_box_append(GTK_BOX(qty_row), w->spin_qty);

    w->lbl_total = gtk_label_new("Total: ");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_total), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_total);

    g_signal_connect(w->combo_item, "notify::selected",
                     G_CALLBACK(on_reg_item_changed), w);
    g_signal_connect(w->spin_qty, "value-changed",
                     G_CALLBACK(on_reg_qty_changed), w);
    update_price_labels(GTK_DROP_DOWN(w->combo_item),
                        GTK_SPIN_BUTTON(w->spin_qty),
                        GTK_LABEL(w->lbl_unit_price),
                        GTK_LABEL(w->lbl_total));
    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Step 3 = Thank-you name */
    SECTION_LBL(box, "Step 3 : Thank-you name (optional)");
    w->entry_thanked = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_thanked),
        "e.g. Aunt Marie  (blank = use registered name)");
    gtk_box_append(GTK_BOX(box), w->entry_thanked);

    /* Submit */
    GtkWidget *bs = gtk_button_new_with_label("  Register Gift");
    g_signal_connect(bs, "clicked", G_CALLBACK(on_reg_submit), w);
    gtk_box_append(GTK_BOX(box), bs);

    w->lbl_result = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(w->lbl_result), TRUE);
    gtk_label_set_xalign(GTK_LABEL(w->lbl_result), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_result);

    return box;
}

/* =========================================================================
 * TAB 2 = Display all gifts
 * ========================================================================= */

typedef struct {
    GtkWidget *entry_pw;
    GtkWidget *lbl_pw_status;
    GtkWidget *list_view;
    int        pw_ok;
} DisplayWidgets;

static void do_refresh(DisplayWidgets *w) {
    GtkTextBuffer *buf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(w->list_view));
    gtk_text_buffer_set_text(buf, "", -1);

    FILE *f = fopen(GIFT_CSV, "r");
    if (!f) {
        gtk_text_buffer_set_text(buf, "No gifts registered yet.", -1);
        return;
    }
    GString *sb = g_string_new(NULL);
    /* Header: Name | ID | Category | Gift Item | Qty | Total FCFA | Total EUR */
    g_string_append(sb,
        "RID  | Guest Name                    | Guest ID | Category        "
        "| Gift Item                         | Qty |  Total FCFA  | Total EUR\n"
        "-----|-------------------------------|----------|-----------------"
        "|-----------------------------------|-----|--------------|----------\n");
    char line[640];
    GiftRecord g;
    int count = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!parse_gift_line(line, &g)) continue;
        g_string_append_printf(sb,
            "%-4d | %-29s | %-8d | %-15s | %-33s | %-3d | %12.0f | %9.2f\n",
            g.record_id,
            g.guest_name,
            g.guest_id,
            g.category,
            g.item_name,
            g.quantity,
            g.total_fcfa,
            g.total_eur);
        count++;
    }
    fclose(f);
    if (count == 0)
        g_string_append(sb, "(No gift records found)\n");
    g_string_append_printf(sb, "\nTotal gift records: %d\n", count);
    gtk_text_buffer_set_text(buf, sb->str, -1);
    g_string_free(sb, TRUE);
}

static void on_disp_load(GtkButton *b, gpointer d) {
    (void)b;
    DisplayWidgets *w = (DisplayWidgets *)d;
    if (check_pw_entry(w->entry_pw, w->lbl_pw_status)) {
        w->pw_ok = 1;
        do_refresh(w);
    }
}

static GtkWidget *build_display_tab(DisplayWidgets *w) {
    memset(w, 0, sizeof(*w));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 14);
    gtk_widget_set_margin_bottom(box, 14);

    SECTION_LBL(box, "Password Required");

    GtkWidget *pw_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(box), pw_row);
    w->entry_pw = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(w->entry_pw), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_pw), "Password");
    gtk_widget_set_hexpand(w->entry_pw, TRUE);
    gtk_box_append(GTK_BOX(pw_row), w->entry_pw);
    GtkWidget *btn = gtk_button_new_with_label("Load / Refresh Gift List");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_disp_load), w);
    gtk_box_append(GTK_BOX(pw_row), btn);

    w->lbl_pw_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_pw_status), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_pw_status);

    w->list_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(w->list_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(w->list_view), TRUE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(w->list_view), FALSE);
    gtk_widget_set_vexpand(w->list_view, TRUE);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),
                                  w->list_view);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(box), scroll);

    return box;
}

/* =========================================================================
 * TAB 3 = Delete a gift
 * ========================================================================= */

typedef struct {
    GtkWidget  *entry_pw;
    GtkWidget  *lbl_pw_status;
    int         pw_ok;
    /* No entry_id: identity is by full name only */
    GtkWidget  *entry_name;
    GtkWidget  *lbl_guest_status;
    GuestBrief  verified_guest;
    int         guest_ok;
    GtkWidget  *entry_rec_id;
    GtkWidget  *lbl_result;
} DeleteWidgets;

static void on_del_pw_confirm(GtkButton *b, gpointer d) {
    (void)b;
    DeleteWidgets *w = (DeleteWidgets *)d;
    w->pw_ok = check_pw_entry(w->entry_pw, w->lbl_pw_status);
}

static void on_del_verify(GtkButton *b, gpointer d) {
    (void)b;
    DeleteWidgets *w = (DeleteWidgets *)d;
    if (!w->pw_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
                           "Please confirm the password first.");
        return;
    }
    const char *nm = gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    if (strlen(nm) < 1) {
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
                           "Please enter your full name."); return;
    }
    if (find_guest_by_name(nm, &w->verified_guest)) {
        w->guest_ok = 1;
        char msg[160];
        snprintf(msg, sizeof(msg), "Verified: %s (ID %d)",
                 w->verified_guest.name, w->verified_guest.id);
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), msg);
    } else {
        w->guest_ok = 0;
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
            "No guest found with that name. "
            "Please re-enter the correct and corresponding name.");
    }
}

static void on_del_submit(GtkButton *b, gpointer d) {
    (void)b;
    DeleteWidgets *w = (DeleteWidgets *)d;
    if (!w->pw_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Confirm the password first."); return;
    }
    if (!w->guest_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Verify your identity first."); return;
    }
    int record_id = atoi(
        gtk_editable_get_text(GTK_EDITABLE(w->entry_rec_id)));
    if (record_id < 1) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Enter a valid gift record ID."); return;
    }

    FILE *f = fopen(GIFT_CSV, "r");
    if (!f) { gtk_label_set_text(GTK_LABEL(w->lbl_result),
                                 "No gifts found."); return; }
    FILE *tmp = fopen(TMP_GIFT, "w");
    if (!tmp) { gtk_label_set_text(GTK_LABEL(w->lbl_result),
                                   "Error: temp file."); fclose(f); return; }

    char line[640]; GiftRecord gr; int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!parse_gift_line(line, &gr)) continue;
        if (gr.record_id == record_id
                && gr.guest_id == w->verified_guest.id)
            found = 1;
        else
            write_gift_line(tmp, &gr);
    }
    fclose(f); fclose(tmp);
    remove(GIFT_CSV); rename(TMP_GIFT, GIFT_CSV);

    char msg[80];
    if (found)
        snprintf(msg, sizeof(msg),
                 "Record %d deleted successfully.", record_id);
    else
        snprintf(msg, sizeof(msg),
                 "Record not found for your account.");
    gtk_label_set_text(GTK_LABEL(w->lbl_result), msg);

    w->pw_ok = w->guest_ok = 0;
    gtk_label_set_text(GTK_LABEL(w->lbl_pw_status),    "");
    gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_pw),     "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),   "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_rec_id), "");
}

static GtkWidget *build_delete_tab(DeleteWidgets *w) {
    memset(w, 0, sizeof(*w));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    /* Step 1 = Password */
    SECTION_LBL(box, "Step 1 : Password");
    GtkWidget *pr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(box), pr);
    w->entry_pw = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(w->entry_pw), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_pw),"group3wed!");
    gtk_widget_set_hexpand(w->entry_pw, TRUE);
    gtk_box_append(GTK_BOX(pr), w->entry_pw);
    GtkWidget *bp = gtk_button_new_with_label("Confirm Password");
    g_signal_connect(bp, "clicked", G_CALLBACK(on_del_pw_confirm), w);
    gtk_box_append(GTK_BOX(pr), bp);
    w->lbl_pw_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_pw_status), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_pw_status);
    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Step 2 = Identity by name only */
    SECTION_LBL(box, "Step 2 : Verify your identity (Full Name only)");
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_box_append(GTK_BOX(box), grid);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Full Name:"), 0, 0, 1, 1);
    w->entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name),
                                   "Exactly as registered");
    gtk_widget_set_hexpand(w->entry_name, TRUE);
    gtk_grid_attach(GTK_GRID(grid), w->entry_name, 1, 0, 1, 1);
    GtkWidget *bv = gtk_button_new_with_label("Verify Identity");
    g_signal_connect(bv, "clicked", G_CALLBACK(on_del_verify), w);
    gtk_grid_attach(GTK_GRID(grid), bv, 1, 1, 1, 1);
    w->lbl_guest_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_guest_status), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_guest_status), TRUE);
    gtk_box_append(GTK_BOX(box), w->lbl_guest_status);
    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Step 3 = Record ID */
    SECTION_LBL(box, "Step 3 : Gift Record to delete");
    GtkWidget *rr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(box), rr);
    gtk_box_append(GTK_BOX(rr), gtk_label_new("Record ID:"));
    w->entry_rec_id = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_rec_id),
                                   "Gift record ID (from Display tab)");
    gtk_widget_set_hexpand(w->entry_rec_id, TRUE);
    gtk_box_append(GTK_BOX(rr), w->entry_rec_id);

    GtkWidget *bs = gtk_button_new_with_label("  Delete Gift Record");
    g_signal_connect(bs, "clicked", G_CALLBACK(on_del_submit), w);
    gtk_box_append(GTK_BOX(box), bs);

    w->lbl_result = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(w->lbl_result), TRUE);
    gtk_label_set_xalign(GTK_LABEL(w->lbl_result), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_result);
    return box;
}

/* =========================================================================
 * TAB 4 = Update a gift
 * ========================================================================= */

typedef struct {
    GtkWidget  *entry_pw;
    GtkWidget  *lbl_pw_status;
    int         pw_ok;
    /* No entry_id: identity is by full name only */
    GtkWidget  *entry_name;
    GtkWidget  *lbl_guest_status;
    GuestBrief  verified_guest;
    int         guest_ok;
    GtkWidget  *entry_rec_id;
    GtkWidget  *combo_item;
    GtkWidget  *lbl_unit_price;
    GtkWidget  *spin_qty;
    GtkWidget  *lbl_total;
    GtkWidget  *entry_thanked;
    GtkWidget  *lbl_result;
    void       *disp_w_ptr;   /* pointer to DisplayWidgets for auto-refresh */
} UpdateWidgets;

static void on_upd_pw_confirm(GtkButton *b, gpointer d) {
    (void)b;
    UpdateWidgets *w = (UpdateWidgets *)d;
    w->pw_ok = check_pw_entry(w->entry_pw, w->lbl_pw_status);
}

static void on_upd_verify(GtkButton *b, gpointer d) {
    (void)b;
    UpdateWidgets *w = (UpdateWidgets *)d;
    if (!w->pw_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
                           "Please confirm the password first."); return;
    }
    const char *nm = gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    if (strlen(nm) < 1) {
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
                           "Please enter your full name."); return;
    }
    if (find_guest_by_name(nm, &w->verified_guest)) {
        w->guest_ok = 1;
        char msg[160];
        snprintf(msg, sizeof(msg), "Verified: %s (ID %d)",
                 w->verified_guest.name, w->verified_guest.id);
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), msg);
    } else {
        w->guest_ok = 0;
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
            "No guest found with that name. "
            "Please re-enter the correct and corresponding name.");
    }
}

static void on_upd_item_changed(GObject *o, GParamSpec *ps, gpointer d) {
    (void)ps;
    UpdateWidgets *w = (UpdateWidgets *)d;
    update_price_labels(GTK_DROP_DOWN(o),
                        GTK_SPIN_BUTTON(w->spin_qty),
                        GTK_LABEL(w->lbl_unit_price),
                        GTK_LABEL(w->lbl_total));
}

static void on_upd_qty_changed(GtkSpinButton *s, gpointer d) {
    (void)s;
    UpdateWidgets *w = (UpdateWidgets *)d;
    update_price_labels(GTK_DROP_DOWN(w->combo_item),
                        GTK_SPIN_BUTTON(w->spin_qty),
                        GTK_LABEL(w->lbl_unit_price),
                        GTK_LABEL(w->lbl_total));
}

static void on_upd_submit(GtkButton *b, gpointer d) {
    (void)b;
    UpdateWidgets *w = (UpdateWidgets *)d;
    if (!w->pw_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Confirm the password first."); return;
    }
    if (!w->guest_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Verify your identity first."); return;
    }
    int record_id = atoi(
        gtk_editable_get_text(GTK_EDITABLE(w->entry_rec_id)));
    if (record_id < 1) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Enter a valid gift record ID."); return;
    }
    guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(w->combo_item));
    if (idx >= (guint)GIFT_ITEM_COUNT) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Select a gift item."); return;
    }
    const GiftItem *chosen = &CATALOGUE[idx];
    int qty = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(w->spin_qty));
    if (qty < 1) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Quantity must be >= 1."); return;
    }
    const char *thanked =
        gtk_editable_get_text(GTK_EDITABLE(w->entry_thanked));

    FILE *f = fopen(GIFT_CSV, "r");
    if (!f) { gtk_label_set_text(GTK_LABEL(w->lbl_result),
                                 "No gifts found."); return; }
    FILE *tmp = fopen(TMP_GIFT, "w");
    if (!tmp) { gtk_label_set_text(GTK_LABEL(w->lbl_result),
                                   "Error: temp file."); fclose(f); return; }

    char line[640]; GiftRecord gr; int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!parse_gift_line(line, &gr)) continue;
        if (gr.record_id == record_id
                && gr.guest_id == w->verified_guest.id) {
            gr.item_id   = chosen->id;
            strncpy(gr.item_name, chosen->name,
                    sizeof(gr.item_name) - 1);
            gr.item_name[sizeof(gr.item_name)-1] = '\0';
            gr.quantity   = qty;
            gr.total_fcfa = chosen->price_fcfa * qty;
            gr.total_eur  = gr.total_fcfa / EUR_RATE;
            if (strlen(thanked) >= 1) {
                strncpy(gr.thanked_by, thanked,
                        sizeof(gr.thanked_by) - 1);
                gr.thanked_by[sizeof(gr.thanked_by)-1] = '\0';
            }
            found = 1;
        }
        write_gift_line(tmp, &gr);
    }
    fclose(f); fclose(tmp);
    remove(GIFT_CSV); rename(TMP_GIFT, GIFT_CSV);

    if (found) {
        double total_fcfa = chosen->price_fcfa * qty;
        double total_eur  = total_fcfa / EUR_RATE;
        char msg[280];
        snprintf(msg, sizeof(msg),
                 "Record %d updated!\n"
                 "New gift: %s  x%d\n"
                 "Total: %.0f FCFA  (%.2f EUR)",
                 record_id, chosen->name, qty, total_fcfa, total_eur);
        gtk_label_set_text(GTK_LABEL(w->lbl_result), msg);
        /* Auto-refresh the Display tab so changes are visible immediately */
        if (w->disp_w_ptr) {
            DisplayWidgets *dw = (DisplayWidgets *)w->disp_w_ptr;
            if (dw->pw_ok) do_refresh(dw);
        }
    } else {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Record not found for your account.");
    }
    w->pw_ok = w->guest_ok = 0;
    gtk_label_set_text(GTK_LABEL(w->lbl_pw_status),    "");
    gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_pw),      "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),    "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_rec_id),  "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_thanked), "");
}

static GtkWidget *build_update_tab(UpdateWidgets *w) {
    memset(w, 0, sizeof(*w));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    /* Step 1 = Password */
    SECTION_LBL(box, "Step 1 : Password");
    GtkWidget *pr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(box), pr);
    w->entry_pw = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(w->entry_pw), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_pw),"group3wed!");
    gtk_widget_set_hexpand(w->entry_pw, TRUE);
    gtk_box_append(GTK_BOX(pr), w->entry_pw);
    GtkWidget *bp = gtk_button_new_with_label("Confirm Password");
    g_signal_connect(bp, "clicked", G_CALLBACK(on_upd_pw_confirm), w);
    gtk_box_append(GTK_BOX(pr), bp);
    w->lbl_pw_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_pw_status), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_pw_status);
    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Step 2 = Identity by name only + Record ID */
    SECTION_LBL(box, "Step 2 : Verify your identity (Full Name only)");
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_box_append(GTK_BOX(box), grid);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Full Name:"), 0, 0, 1, 1);
    w->entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name),
                                   "Exactly as registered");
    gtk_widget_set_hexpand(w->entry_name, TRUE);
    gtk_grid_attach(GTK_GRID(grid), w->entry_name, 1, 0, 1, 1);

    GtkWidget *bv = gtk_button_new_with_label("Verify Identity");
    g_signal_connect(bv, "clicked", G_CALLBACK(on_upd_verify), w);
    gtk_grid_attach(GTK_GRID(grid), bv, 1, 1, 1, 1);

    w->lbl_guest_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_guest_status), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_guest_status), TRUE);
    gtk_box_append(GTK_BOX(box), w->lbl_guest_status);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Record ID:"), 0, 2, 1, 1);
    w->entry_rec_id = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_rec_id),
                                   "Gift record ID to update (see Display tab)");
    gtk_widget_set_hexpand(w->entry_rec_id, TRUE);
    gtk_grid_attach(GTK_GRID(grid), w->entry_rec_id, 1, 2, 1, 1);

    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Step 3 = New gift (total auto-calculated) */
    SECTION_LBL(box, "Step 3 : Select new gift and quantity (total auto-calculated)");
    w->combo_item = gtk_drop_down_new(
        G_LIST_MODEL(build_catalogue_list()), NULL);
    gtk_widget_set_hexpand(w->combo_item, TRUE);
    gtk_box_append(GTK_BOX(box), w->combo_item);

    w->lbl_unit_price = gtk_label_new("Unit price: ");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_unit_price), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_unit_price);

    GtkWidget *qr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), qr);
    gtk_box_append(GTK_BOX(qr), gtk_label_new("Quantity:"));
    w->spin_qty = gtk_spin_button_new_with_range(1, 9999, 1);
    gtk_box_append(GTK_BOX(qr), w->spin_qty);

    w->lbl_total = gtk_label_new("Total: ");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_total), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_total);

    g_signal_connect(w->combo_item, "notify::selected",
                     G_CALLBACK(on_upd_item_changed), w);
    g_signal_connect(w->spin_qty, "value-changed",
                     G_CALLBACK(on_upd_qty_changed), w);
    update_price_labels(GTK_DROP_DOWN(w->combo_item),
                        GTK_SPIN_BUTTON(w->spin_qty),
                        GTK_LABEL(w->lbl_unit_price),
                        GTK_LABEL(w->lbl_total));

    gtk_box_append(GTK_BOX(box),
        gtk_label_new("Thank-you name (blank = keep original):"));
    w->entry_thanked = gtk_entry_new();
    gtk_box_append(GTK_BOX(box), w->entry_thanked);

    GtkWidget *bs = gtk_button_new_with_label("  Update Gift Record");
    g_signal_connect(bs, "clicked", G_CALLBACK(on_upd_submit), w);
    gtk_box_append(GTK_BOX(box), bs);

    w->lbl_result = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(w->lbl_result), TRUE);
    gtk_label_set_xalign(GTK_LABEL(w->lbl_result), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_result);

    return box;
}

/* =========================================================================
 * Application entry point
 * ========================================================================= */

static RegisterWidgets reg_w;
static DisplayWidgets  disp_w;
static DeleteWidgets   del_w;
static UpdateWidgets   upd_w;

static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    _suppress_unused();   /* Suppress unused-function warning */

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window),
                         "Wedding Gift Manager - group3wed");
    gtk_window_set_default_size(GTK_WINDOW(window), 920, 660);

    /* Header bar */
    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *title_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_lbl),
                         "<b>Wedding Gift Manager</b>  -  group3wed");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title_lbl);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    /* Notebook */
    GtkWidget *nb = gtk_notebook_new();
    gtk_window_set_child(GTK_WINDOW(window), nb);

    GtkWidget *s1 = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(s1),
                                  build_register_tab(&reg_w));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), s1,
                             gtk_label_new("Register Gift"));

    gtk_notebook_append_page(GTK_NOTEBOOK(nb),
                             build_display_tab(&disp_w),
                             gtk_label_new("Display Gifts"));

    GtkWidget *s3 = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(s3),
                                  build_delete_tab(&del_w));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), s3,
                             gtk_label_new("Delete Gift"));

    GtkWidget *s4 = gtk_scrolled_window_new();
    upd_w.disp_w_ptr = &disp_w;   /* link update tab to display tab for auto-refresh */
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(s4),
                                  build_update_tab(&upd_w));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), s4,
                             gtk_label_new("Update Gift"));

    /* GTK 4: gtk_window_present replaces gtk_widget_show */
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new(
        "com.group3wed.giftmanager",
        G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
