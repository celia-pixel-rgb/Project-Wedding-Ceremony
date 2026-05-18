#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* readlink is POSIX-only; on Windows/MSYS2 use GetModuleFileNameA instead */
#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

/* ---- Currency conversion ---- */
#define EUR_RATE        655.957   /* 1 EUR = 655.957 FCFA (fixed CFA franc peg) */

/* ---- Runtime capacity for the in-memory catalogue ---- */
#define CAT_MAX         200      /* max catalogue entries at runtime */

/*
 * Absolute paths for every CSV file the program touches.
 * Populated once by init_csv_paths() so the binary works from any cwd.
 */
static char PERSON_CSV[512];   /* persons.csv   */
static char CAT_CSV[512];      /* category.csv  */
static char GIFT_CSV[512];     /* gifts.csv     */
static char TMP_GIFT[512];     /* gifts_tmp.csv */
static char FABRIC_CSV[512];   /* persists active fabric selection */

/* Admin password checked on the login screen and inside admin tabs */
static const char *PASSWORD  = "group3wed!";

/* =========================================================
 * Wedding Fabrics – data model
 * =========================================================
 *
 * Six fabric designs are hard-coded.  The admin picks one and activates it;
 * guests are then directed to the corresponding online shop URL.
 */
#define FABRIC_COUNT 6

/* Human-readable names displayed in the UI for each fabric choice */
static const char *FABRIC_NAMES[FABRIC_COUNT] = {
    "Real Dutch Spiral (Blue & White)",
    "African Mosaic Circles (Blue & White)",
    "Bold Floral Ankara (Navy & White)",
    "Geometric Compass Wax (Blue & Teal)",
    "Traditional Pottery Wax (Blue & Orange)",
    "Glitter Ankara Lace (Green & Gold)"
};

/* Image filenames placed next to the binary (admin drops these in) */
static const char *FABRIC_IMG_FILES[FABRIC_COUNT] = {
    "fabric_dutch_spiral.jpg",
    "fabric_mosaic_circles.jpg",
    "fabric_bold_floral.jpg",
    "fabric_geometric_compass.jpg",
    "fabric_pottery_wax.jpg",
    "fabric_glitter_lace.jpg"
};

/* Real online shops where guests are sent to buy each fabric */
static const char *FABRIC_SHOP_URLS[FABRIC_COUNT] = {
    "https://www.vlisco.com/products/spiral-wax-print",
    "https://www.vlisco.com/products/african-mosaic",
    "https://ankara-styles.com/product/bold-floral-ankara",
    "https://www.vlisco.com/products/geometric-compass-wax",
    "https://www.vlisco.com/products/traditional-pottery-wax",
    "https://lacebykimani.com/products/glitter-ankara-lace"
};

/* =========================================================
 * Payment configuration (set by admin, saved to fabric.csv)
 * =========================================================
 *
 * These strings are written to fabric.csv on every admin save and loaded
 * back at startup, so payment details survive application restarts.
 */
/* Phone number for Mobile Money (Orange / MTN) and bank details for card */
static char MOMO_PHONE[32]      = "+237600000000";   /* default placeholder */
static char MOMO_OPERATOR[16]   = "Orange";          /* "Orange" or "MTN"  */
static char BANK_NAME[64]       = "Afriland First Bank";
static char BANK_ACCOUNT[64]    = "12345-67890-XYZ";
static char BANK_ACCOUNT_NAME[64] = "Group3 Wedding Fund";

/*
 * FabricConfig – compact run-time state for the fabric feature.
 *   active          – 1 = guests can see and buy the fabric; 0 = hidden
 *   fabric_index    – which of the 6 designs is currently selected (0-5)
 *   low_stock_alert – (reserved) warn guests when remaining units fall below this
 */
typedef struct {
    int    active;           /* 1 = fabric offer is on, 0 = disabled        */
    int    fabric_index;     /* 0-5                                          */
    int    low_stock_alert;  /* warn guest side when remaining <= this       */
} FabricConfig;

/* Global fabric configuration; default = inactive, first design, alert at 10 */
static FabricConfig fabric_cfg = { 0, 0, 10 };

/*
 * init_csv_paths
 * --------------
 * Resolve the directory that contains the running binary and build absolute
 * paths for all five CSV files.  This ensures the files are always stored
 * next to the binary, not in whatever directory the user launched from.
 *
 * Implementation differs by platform:
 *   Windows / MSYS2  → GetModuleFileNameA() gives the binary's full path.
 *   Linux / macOS    → readlink("/proc/self/exe") gives the same.
 * Both strip the filename component to get the directory, then snprintf
 * each CSV path.  If resolution fails the bare filenames are used as a
 * fallback (resolved from cwd at open time).
 */
static void init_csv_paths(void) {
    char dir[512] = {0};

#ifdef _WIN32
    /* Windows / MSYS2: resolve the binary directory via GetModuleFileNameA */
    char path[512] = {0};
    DWORD len = GetModuleFileNameA(NULL, path, (DWORD)(sizeof(path) - 1));
    if (len > 0) {
        char *slash = strrchr(path, '\\');
        if (!slash) slash = strrchr(path, '/');
        if (slash) { *slash = '\0'; strncpy(dir, path, sizeof(dir) - 1); }
    }
#else
    /* Linux / macOS: resolve via /proc/self/exe */
    char exe[512] = {0};
    ssize_t elen = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (elen > 0) {
        exe[elen] = '\0';
        char *slash = strrchr(exe, '/');
        if (slash) { *slash = '\0'; strncpy(dir, exe, sizeof(dir) - 1); }
    }
#endif

    if (dir[0] != '\0') {
        /* Build absolute paths using the resolved binary directory */
        snprintf(PERSON_CSV, sizeof(PERSON_CSV), "%s/persons.csv",   dir);
        snprintf(CAT_CSV,    sizeof(CAT_CSV),    "%s/category.csv",  dir);
        snprintf(GIFT_CSV,   sizeof(GIFT_CSV),   "%s/gifts.csv",     dir);
        snprintf(TMP_GIFT,   sizeof(TMP_GIFT),   "%s/gifts_tmp.csv", dir);
        snprintf(FABRIC_CSV, sizeof(FABRIC_CSV), "%s/fabric.csv",    dir);
    } else {
        /* Fallback: use current working directory */
        strncpy(PERSON_CSV, "persons.csv",   sizeof(PERSON_CSV)  - 1);
        strncpy(CAT_CSV,    "category.csv",  sizeof(CAT_CSV)     - 1);
        strncpy(GIFT_CSV,   "gifts.csv",     sizeof(GIFT_CSV)    - 1);
        strncpy(TMP_GIFT,   "gifts_tmp.csv", sizeof(TMP_GIFT)    - 1);
        strncpy(FABRIC_CSV, "fabric.csv",    sizeof(FABRIC_CSV)  - 1);
    }
}

/* =========================================================
 * GiftItem – one row in the in-memory gift catalogue
 * =========================================================
 *   id         – unique 1-based catalogue ID
 *   name       – human-readable gift name (max 99 chars used)
 *   price_fcfa – unit price in FCFA
 */
typedef struct {
    int    id;
    char   name[100];
    double price_fcfa;
} GiftItem;

/*
 * cat[]      – in-memory catalogue array; pre-loaded with 25 default items.
 * cat_count  – number of entries currently in use (may grow at runtime).
 *
 * The array is static so it lives for the lifetime of the process.
 * add_catalogue_item() appends to it; CAT_MAX is the hard ceiling.
 */
static GiftItem cat[CAT_MAX] = {
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
static int cat_count = 25;   /* number of entries currently in cat[] */

/*
 * live_models[] – registry of every GtkStringList that mirrors cat[].
 *
 * When a new item is added at runtime (add_catalogue_item), or when an item
 * reaches GIFT_OFFER_LIMIT (mark_item_taken_in_models), all registered models
 * must be updated so every dropdown in the UI reflects the change instantly
 * without requiring a restart.
 *
 * register_live_model() is called once for each GtkStringList that is
 * created by build_catalogue_list().
 */
#define MAX_LIVE_MODELS 8
static GtkStringList *live_models[MAX_LIVE_MODELS];
static int            live_model_count = 0;

/* Register a newly-created GtkStringList so future catalogue mutations
 * (add / mark-taken) can be pushed to it automatically. */
static void register_live_model(GtkStringList *m) {
    if (live_model_count < MAX_LIVE_MODELS)
        live_models[live_model_count++] = m;
}

/*
 * add_catalogue_item
 * ------------------
 * Append a new entry to the in-memory catalogue and immediately push
 * a formatted label string to every live GtkStringList so all dropdowns
 * update without a restart.
 *
 * Returns 0 on success, -1 if CAT_MAX has been reached.
 */
static int add_catalogue_item(const char *name, double price_fcfa) {
    if (cat_count >= CAT_MAX) return -1;
    int idx = cat_count;
    /* Assign the next sequential ID (always higher than the current last) */
    cat[idx].id = (cat_count > 0) ? cat[cat_count - 1].id + 1 : 1;
    strncpy(cat[idx].name, name, sizeof(cat[idx].name) - 1);
    cat[idx].name[sizeof(cat[idx].name) - 1] = '\0';
    cat[idx].price_fcfa = price_fcfa;
    cat_count++;
    /* Build the formatted label and push it to every live model */
    char label[128];
    snprintf(label, sizeof(label),
             "%2d. %-35s %10.0f FCFA  (%7.2f EUR)",
             cat[idx].id, cat[idx].name,
             cat[idx].price_fcfa,
             cat[idx].price_fcfa / EUR_RATE);
    for (int i = 0; i < live_model_count; i++)
        gtk_string_list_append(live_models[i], label);
    return 0;
}

/*
 * make_item_label
 * ---------------
 * Build a printable catalogue row for the item at index i.
 * When taken != 0 the line is prefixed with "✗ TAKEN" (UTF-8 cross mark)
 * so the item appears greyed-out / unavailable in the dropdown.
 */
static void make_item_label(int i, int taken, char *buf, int bufsz) {
    if (taken)
        snprintf(buf, bufsz,
                 "✗ TAKEN  %2d. %-31s %10.0f FCFA  (%7.2f EUR)",
                 cat[i].id, cat[i].name,
                 cat[i].price_fcfa,
                 cat[i].price_fcfa / EUR_RATE);
    else
        snprintf(buf, bufsz,
                 "%2d. %-35s %10.0f FCFA  (%7.2f EUR)",
                 cat[i].id, cat[i].name,
                 cat[i].price_fcfa,
                 cat[i].price_fcfa / EUR_RATE);
}

/*
 * mark_item_taken_in_models
 * -------------------------
 * Replace the label for catalogue index cat_idx in every live GtkStringList
 * with the "✗ TAKEN" variant.  Called the moment an item's registration
 * count reaches GIFT_OFFER_LIMIT so the UI reflects it in real time.
 *
 * gtk_string_list_splice() with n_removals=1 and one replacement string
 * atomically swaps one row without affecting surrounding rows.
 */
static void mark_item_taken_in_models(int cat_idx) {
    char label[140];
    make_item_label(cat_idx, 1, label, sizeof(label));
    for (int i = 0; i < live_model_count; i++) {
        gtk_string_list_splice(live_models[i],
                               (guint)cat_idx, 1,
                               (const char *[]){ label, NULL });
    }
}

/* =========================================================
 * GuestBrief – minimal guest info loaded from persons.csv
 * =========================================================
 *   id     – unique guest ID (first column of persons.csv)
 *   name   – full name exactly as registered
 *   status – e.g. "Confirmed", "Pending"
 */
typedef struct {
    int  id;
    char name[100];
    char status[50];
} GuestBrief;

/* =========================================================
 * GiftRecord – one row in gifts.csv
 * =========================================================
 *   record_id   – auto-incremented unique record ID
 *   guest_id    – FK → persons.csv id
 *   guest_name  – denormalised for easy display / reporting
 *   category    – group/category code from category.csv
 *   item_id     – FK → GiftItem.id
 *                 Special values:
 *                    0  = financial contribution
 *                   -1  = fabric purchase interest
 *   item_name   – denormalised item name
 *   quantity    – number of units (fixed at 1 for most gift modes)
 *   total_fcfa  – quantity × unit price in FCFA
 *   total_eur   – total converted to EUR at EUR_RATE
 *   thanked_by  – name used on the thank-you card
 */
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

/* ---- Forward decl for FabricWidgets (needs GuestBrief) ---- */
/*
 * FabricWidgets – all GTK widget pointers for the fabric feature.
 *
 * Admin side  : radio buttons, image previews, payment config entries,
 *               activation checkbox, status label.
 * Guest side  : the fabric box container (shown/hidden based on active flag),
 *               fabric name label, image, stock label, result label,
 *               identity entry, verify label, and the verified-guest state.
 */
typedef struct {
    /* admin side */
    GtkWidget  *radio[FABRIC_COUNT];
    GtkWidget  *img[FABRIC_COUNT];        /* image widget per fabric choice */
    GtkWidget  *entry_alert;
    GtkWidget  *lbl_status;
    GtkWidget  *lbl_sold_info;
    GtkWidget  *chk_active;
    /* admin payment configuration */
    GtkWidget  *entry_momo_phone;
    GtkWidget  *entry_momo_operator;
    GtkWidget  *entry_bank_name;
    GtkWidget  *entry_bank_account;
    GtkWidget  *entry_bank_account_name;
    /* guest side */
    GtkWidget  *guest_fabric_box;
    GtkWidget  *guest_lbl_name;
    GtkWidget  *guest_img;                /* fabric image shown to guest */
    GtkWidget  *guest_lbl_stock;
    GtkWidget  *guest_lbl_result;
    GtkWidget  *guest_entry_name;
    GtkWidget  *guest_lbl_verify;
    GuestBrief  guest_verified;           /* guest verified for fabric purchase */
    int         guest_ok;                 /* 1 once identity is confirmed */
} FabricWidgets;

/* Single global instance of FabricWidgets shared by admin and guest tabs */
static FabricWidgets fab_w;

/* ---- fabric persistence ---- */

/*
 * fabric_save
 * -----------
 * Write the current fabric configuration and payment details to fabric.csv.
 * Format (5 lines):
 *   Line 1 : active,fabric_index,low_stock_alert
 *   Line 2 : MOMO_OPERATOR,MOMO_PHONE
 *   Line 3 : BANK_NAME
 *   Line 4 : BANK_ACCOUNT
 *   Line 5 : BANK_ACCOUNT_NAME
 *
 * Called after every admin save so settings survive application restarts.
 */
static void fabric_save(void) {
    if (!FABRIC_CSV[0]) return;
    FILE *f = fopen(FABRIC_CSV, "w");
    if (!f) return;
    /* Line 1: fabric config */
    fprintf(f, "%d,%d,%d\n",
            fabric_cfg.active,
            fabric_cfg.fabric_index,
            fabric_cfg.low_stock_alert);
    /* Line 2: mobile money: operator,phone */
    fprintf(f, "%s,%s\n", MOMO_OPERATOR, MOMO_PHONE);
    /* Line 3: bank: name */
    fprintf(f, "%s\n", BANK_NAME);
    /* Line 4: bank account number */
    fprintf(f, "%s\n", BANK_ACCOUNT);
    /* Line 5: bank account holder name */
    fprintf(f, "%s\n", BANK_ACCOUNT_NAME);
    fclose(f);
}

/*
 * fabric_load
 * -----------
 * Read fabric.csv and restore fabric_cfg and payment strings.
 * Called once at startup (before the GTK app is created) so the admin's
 * previous selections are already in effect when the first window opens.
 *
 * Mirrors the 5-line format written by fabric_save().
 * Missing or malformed lines are silently ignored; defaults remain.
 */
static void fabric_load(void) {
    if (!FABRIC_CSV[0]) return;
    FILE *f = fopen(FABRIC_CSV, "r");
    if (!f) return;
    char line[256];
    /* Line 1: fabric config */
    if (fgets(line, sizeof(line), f))
        sscanf(line, "%d,%d,%d",
               &fabric_cfg.active,
               &fabric_cfg.fabric_index,
               &fabric_cfg.low_stock_alert);
    /* Line 2: operator,phone (comma-separated; split on first comma) */
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        char *comma = strchr(line, ',');
        if (comma) {
            *comma = '\0';
            strncpy(MOMO_OPERATOR, line,       sizeof(MOMO_OPERATOR) - 1);
            strncpy(MOMO_PHONE,    comma + 1,  sizeof(MOMO_PHONE)    - 1);
        }
    }
    /* Line 3: bank name */
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        strncpy(BANK_NAME, line, sizeof(BANK_NAME) - 1);
    }
    /* Line 4: bank account */
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        strncpy(BANK_ACCOUNT, line, sizeof(BANK_ACCOUNT) - 1);
    }
    /* Line 5: account holder */
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        strncpy(BANK_ACCOUNT_NAME, line, sizeof(BANK_ACCOUNT_NAME) - 1);
    }
    fclose(f);
}

/*
 * find_guest_by_name
 * ------------------
 * Case-insensitive linear search through persons.csv.
 * Converts both the search key and each stored name to lower-case before
 * comparing, so "Jean-Pierre" matches "jean-pierre".
 *
 * persons.csv row format:
 *   id,name,age,status,phone,side,parking
 * Only id, name, and status are extracted into *out.
 *
 * Returns 1 and populates *out on success; returns 0 if not found.
 */
static int find_guest_by_name(const char *name, GuestBrief *out) {
    FILE *f = fopen(PERSON_CSV, "r");
    if (!f) return 0;
    /* Lower-case the search key */
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
            /* Lower-case the stored name for comparison */
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

/*
 * find_category_for_guest
 * -----------------------
 * Scan category.csv to determine which group/category a guest belongs to.
 *
 * category.csv uses a two-line-type format:
 *   CAT,<id>,<code>,<guest_count>   – header line defining a new category
 *   ID,<guest_id>                   – marks a guest as belonging to the
 *                                     most recently seen CAT
 *
 * The function tracks the current category code as it walks through the file
 * and writes it to out_code when it finds the matching ID line.
 * "Unassigned" is used as the default if no match is found.
 */
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
            /* New category header: parse the category code */
            int id, gc;
            if (sscanf(line + 4, "%d,%49[^,],%d",
                       &id, current, &gc) < 2)
                current[0] = '\0';
        } else if (strncmp(line, "ID,", 3) == 0) {
            /* Guest ID line: check if it matches the target */
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

/*
 * get_next_gift_id
 * ----------------
 * Scan gifts.csv for the highest record_id and return max + 1.
 * Returns 1 when the file is empty or missing (first record).
 * This gives each new gift record a unique, monotonically increasing ID.
 */
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

/*
 * write_gift_line
 * ---------------
 * Serialise a GiftRecord as one CSV line into an already-open file.
 *
 * Any commas inside the string fields (guest_name, category, item_name,
 * thanked_by) are replaced with semicolons before writing so the column
 * count remains unambiguous for parse_gift_line().
 *
 * Column order: rid,gid,name,cat,iid,item,qty,fcfa,eur,thanked
 */
static void write_gift_line(FILE *f, const GiftRecord *g) {
    char gname[100], cat[50], iname[100], tby[100];
    strncpy(gname, g->guest_name, sizeof(gname) - 1); gname[sizeof(gname)-1] = '\0';
    strncpy(cat,   g->category,   sizeof(cat)   - 1); cat[sizeof(cat)-1]     = '\0';
    strncpy(iname, g->item_name,  sizeof(iname) - 1); iname[sizeof(iname)-1] = '\0';
    strncpy(tby,   g->thanked_by, sizeof(tby)   - 1); tby[sizeof(tby)-1]     = '\0';
    /* Sanitise: replace commas with semicolons so CSV structure is preserved */
    for (char *p = gname; *p; p++) if (*p == ',') *p = ';';
    for (char *p = cat;   *p; p++) if (*p == ',') *p = ';';
    for (char *p = iname; *p; p++) if (*p == ',') *p = ';';
    for (char *p = tby;   *p; p++) if (*p == ',') *p = ';';
    fprintf(f, "%d,%d,%s,%s,%d,%s,%d,%.2f,%.2f,%s\n",
            g->record_id, g->guest_id,
            gname, cat,
            g->item_id, iname,
            g->quantity,
            g->total_fcfa, g->total_eur,
            tby);
}

/*
 * parse_gift_line
 * ---------------
 * Deserialise one raw CSV line from gifts.csv into a GiftRecord.
 * Expects exactly 10 fields (matching the format written by write_gift_line).
 * Returns 1 on success (all 10 fields parsed), 0 on malformed input.
 */
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

/*
 * fabric_units_sold
 * -----------------
 * Count how many fabric-interest records (item_id == -1) exist in gifts.csv.
 * Used by the admin panel to show how many guests have expressed interest.
 */
static int fabric_units_sold(void) {
    FILE *f = fopen(GIFT_CSV, "r");
    if (!f) return 0;
    char line[640];
    GiftRecord g;
    int total = 0;
    while (fgets(line, sizeof(line), f))
        if (parse_gift_line(line, &g) && g.item_id == -1)
            total += g.quantity;
    fclose(f);
    return total;
}

/*
 * fabric_remaining
 * ----------------
 * Stock tracking is intentionally not implemented (price is set on the
 * shop side).  Returns 999 when the fabric offer is active (meaning
 * "always available"), or 0 when it is disabled.
 */
static int fabric_remaining(void) {
    /* Stock tracking removed; fabric is always "available" when active */
    return fabric_cfg.active ? 999 : 0;
}

/* Forward declaration – fabric_refresh_guest_panel is defined after the
 * admin fabric tab builder but is called from fabric_refresh_admin_info. */
static void fabric_refresh_guest_panel(void);   /* forward decl */

/*
 * fabric_refresh_admin_info
 * -------------------------
 * Update the "Current Status" label on the admin fabric tab to reflect
 * how many fabric-interest records exist and whether the offer is active.
 * Safe to call any time fab_w.lbl_sold_info is valid (after UI is built).
 */
static void fabric_refresh_admin_info(void) {
    if (!fab_w.lbl_sold_info) return;
    int sold = fabric_units_sold();
    char buf[200];
    if (!fabric_cfg.active) {
        snprintf(buf, sizeof(buf),
                 "Status: INACTIVE  |  Fabric orders recorded: %d", sold);
    } else {
        snprintf(buf, sizeof(buf),
                 "✔ Active  |  Fabric orders recorded: %d  "
                 "(guests are directed to the online shop)",
                 sold);
    }
    gtk_label_set_text(GTK_LABEL(fab_w.lbl_sold_info), buf);
}

/*
 * on_alert_ok
 * -----------
 * Button callback for the OK button inside a show_alert() modal window.
 * Destroys the window, returning control to the parent.
 */
static void on_alert_ok(GtkButton *btn, gpointer win) {
    (void)btn;
    gtk_window_destroy(GTK_WINDOW(win));
}

/*
 * show_alert
 * ----------
 * Display a small modal dialog with a single text message and an OK button.
 * Used to surface error or informational messages that shouldn't modify the
 * main window's label widgets directly.
 *
 * parent_widget – any widget in the parent window (used to obtain the
 *                 GtkWindow via gtk_widget_get_root()); may be NULL.
 * msg           – NUL-terminated message string; wrapped automatically.
 */
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

    gtk_window_present(GTK_WINDOW(win));
}

/*
 * check_pw_entry
 * --------------
 * Read the text from pw_entry, compare it with PASSWORD, and update
 * pw_status_lbl with the result.
 *
 * Returns 1 if the password is correct; 0 otherwise.
 * On failure the entry is cleared so the user can retype.
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

/*
 * update_price_labels
 * -------------------
 * Refresh the unit-price and total labels whenever the selected item or
 * quantity changes.  Used by the Update Gift tab (which exposes a spin
 * button for quantity) so the admin always sees the live total.
 *
 * combo     – GtkDropDown bound to the catalogue model
 * spin      – GtkSpinButton for quantity
 * lbl_unit  – label to display "Unit price: X FCFA (Y EUR)"
 * lbl_total – label to display "Total: X FCFA (Y EUR)"
 */
static void update_price_labels(GtkDropDown   *combo,
                                GtkSpinButton *spin,
                                GtkLabel      *lbl_unit,
                                GtkLabel      *lbl_total) {
    guint idx = gtk_drop_down_get_selected(combo);
    if ((int)idx >= cat_count) return;
    double unit  = cat[idx].price_fcfa;
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

/*
 * show_unit_price
 * ---------------
 * Simplified variant of update_price_labels used when quantity is always 1
 * (single-guest and group-gift tabs).  Only the unit-price label is updated.
 */
static void show_unit_price(GtkDropDown *combo, GtkLabel *lbl_unit) {
    guint idx = gtk_drop_down_get_selected(combo);
    if ((int)idx >= cat_count) return;
    double unit = cat[idx].price_fcfa;
    char buf[160];
    snprintf(buf, sizeof(buf),
             "Price:  %.0f FCFA  (%.2f EUR)",
             unit, unit / EUR_RATE);
    gtk_label_set_text(lbl_unit, buf);
}

/*
 * count_gift_registrations
 * ------------------------
 * Return the number of times target_item_id appears as item_id in gifts.csv.
 * Used to enforce GIFT_OFFER_LIMIT and to decide when to mark an item taken.
 * Skips no rows (item_id == 0 = financial contrib, -1 = fabric; both counted
 * only if they happen to equal target_item_id, which they won't in practice).
 */
static int count_gift_registrations(int target_item_id) {
    FILE *f = fopen(GIFT_CSV, "r");
    if (!f) return 0;
    char line[640];
    GiftRecord g;
    int count = 0;
    while (fgets(line, sizeof(line), f))
        if (parse_gift_line(line, &g) && g.item_id == target_item_id)
            count++;
    fclose(f);
    return count;
}

/* Maximum number of times any single catalogue item may be offered */
#define GIFT_OFFER_LIMIT 3   /* maximum times any single gift may be offered */

/*
 * build_catalogue_list
 * --------------------
 * Construct a GtkStringList containing one formatted string per catalogue
 * entry.  Items that have already reached GIFT_OFFER_LIMIT are built with
 * make_item_label(..., taken=1, ...) so they display the "✗ TAKEN" prefix.
 *
 * The new model is registered with register_live_model() so future mutations
 * (new items added, items reaching the limit) are automatically propagated
 * to it without rebuilding the dropdown.
 */
static GtkStringList *build_catalogue_list(void) {
    GtkStringList *model = gtk_string_list_new(NULL);
    char label[140];
    for (int i = 0; i < cat_count; i++) {
        int taken = (count_gift_registrations(cat[i].id) >= GIFT_OFFER_LIMIT);
        make_item_label(i, taken, label, sizeof(label));
        gtk_string_list_append(model, label);
    }
    register_live_model(model);
    return model;
}

/* =========================================================
 * Custom GtkDropDown item factories
 * =========================================================
 *
 * GTK 4 GtkDropDown uses a list-item factory to render each row both
 * inside the popup (popup_factory) and on the collapsed button face
 * (header_factory).
 *
 * popup_factory  – on_factory_setup / on_factory_bind
 *   Creates a GtkLabel per row.  "Taken" rows (starting with the UTF-8
 *   cross mark ✗, bytes 0xE2 0x9C 0x97) are rendered in red italic and
 *   made non-interactive (gtk_widget_set_sensitive false).
 *
 * header_factory – on_header_setup / on_header_bind
 *   Creates the collapsed button face.  Shows "— Select a gift —" when
 *   nothing is selected, the item text otherwise, and "<span>✗ TAKEN</span>"
 *   in red italic for taken items.
 */

/* popup factory – setup: create and configure a GtkLabel for a list row */
static void on_factory_setup(GtkSignalListItemFactory *f,
                             GObject *item, gpointer d) {
    (void)f; (void)d;
    GtkWidget *lbl = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_NONE);
    gtk_label_set_wrap(GTK_LABEL(lbl), FALSE);
    gtk_widget_set_hexpand(lbl, TRUE);
    gtk_widget_set_margin_start(lbl, 6);
    gtk_widget_set_margin_end(lbl, 6);
    gtk_widget_set_margin_top(lbl, 3);
    gtk_widget_set_margin_bottom(lbl, 3);
    gtk_list_item_set_child(GTK_LIST_ITEM(item), lbl);
}

/* popup factory – bind: fill the label for a specific list row */
static void on_factory_bind(GtkSignalListItemFactory *f,
                            GObject *item, gpointer d) {
    (void)f; (void)d;
    GtkWidget *lbl = gtk_list_item_get_child(GTK_LIST_ITEM(item));
    GtkStringObject *so = GTK_STRING_OBJECT(
        gtk_list_item_get_item(GTK_LIST_ITEM(item)));
    const char *str = gtk_string_object_get_string(so);

    /* Detect "✗ TAKEN" rows by their UTF-8 prefix (E2 9C 97 = ✗) */
    if (str && strncmp(str, "\xe2\x9c\x97", 3) == 0) {
        char markup[300];
        char escaped[200];
        /* XML-escape the label text before using it in Pango markup */
        int si = 0, di = 0;
        while (str[si] && di < 190) {
            if (str[si] == '&') { strcpy(escaped+di,"&amp;"); di+=5; }
            else if (str[si] == '<') { strcpy(escaped+di,"&lt;"); di+=4; }
            else if (str[si] == '>') { strcpy(escaped+di,"&gt;"); di+=4; }
            else escaped[di++] = str[si];
            si++;
        }
        escaped[di] = '\0';
        snprintf(markup, sizeof(markup),
                 "<span foreground='#cc0000' style='italic'>%s</span>",
                 escaped);
        gtk_label_set_markup(GTK_LABEL(lbl), markup);
        /* Disable interaction so taken items cannot be selected */
        gtk_widget_set_sensitive(lbl, FALSE);
    } else {
        gtk_label_set_text(GTK_LABEL(lbl), str ? str : "");
        gtk_widget_set_sensitive(lbl, TRUE);
    }
}

/* header factory – setup: create the collapsed button face label */
static void on_header_setup(GtkSignalListItemFactory *f,
                            GObject *item, gpointer d) {
    (void)f; (void)d;
    GtkWidget *lbl = gtk_label_new("— Select a gift —");
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(lbl, TRUE);
    gtk_list_item_set_child(GTK_LIST_ITEM(item), lbl);
}

/* header factory – bind: update the collapsed face when selection changes */
static void on_header_bind(GtkSignalListItemFactory *f,
                           GObject *item, gpointer d) {
    (void)f; (void)d;
    GtkWidget *lbl = gtk_list_item_get_child(GTK_LIST_ITEM(item));
    GtkStringObject *so = GTK_STRING_OBJECT(
        gtk_list_item_get_item(GTK_LIST_ITEM(item)));
    if (!so) {
        /* No selection: show placeholder text */
        gtk_label_set_text(GTK_LABEL(lbl), "— Select a gift —");
        return;
    }
    const char *str = gtk_string_object_get_string(so);
    /* Render taken items with red markup even on the collapsed face */
    if (str && strncmp(str, "\xe2\x9c\x97", 3) == 0)
        gtk_label_set_markup(GTK_LABEL(lbl),
            "<span foreground='#cc0000' style='italic'>✗ TAKEN</span>");
    else
        gtk_label_set_text(GTK_LABEL(lbl), str ? str : "");
}

/*
 * build_gift_dropdown
 * -------------------
 * Construct a fully configured GtkDropDown for the gift catalogue.
 *
 * - The data model is build_catalogue_list() (also registered as a live model).
 * - The popup factory (on_factory_setup / on_factory_bind) renders individual
 *   rows with taken-item highlighting.
 * - The header factory (on_header_setup / on_header_bind) renders the collapsed
 *   button face.
 * - Search is enabled so guests can type to filter.
 * - Width is set to 600 px so the long item labels are fully visible.
 */
static GtkWidget *build_gift_dropdown(void) {
    GtkStringList *model = build_catalogue_list();   /* also registers model */

    GtkSignalListItemFactory *popup_factory =
        GTK_SIGNAL_LIST_ITEM_FACTORY(gtk_signal_list_item_factory_new());
    g_signal_connect(popup_factory, "setup",
                     G_CALLBACK(on_factory_setup), NULL);
    g_signal_connect(popup_factory, "bind",
                     G_CALLBACK(on_factory_bind),  NULL);

    GtkSignalListItemFactory *header_factory =
        GTK_SIGNAL_LIST_ITEM_FACTORY(gtk_signal_list_item_factory_new());
    g_signal_connect(header_factory, "setup",
                     G_CALLBACK(on_header_setup), NULL);
    g_signal_connect(header_factory, "bind",
                     G_CALLBACK(on_header_bind),  NULL);

    GtkWidget *dd = gtk_drop_down_new(
        G_LIST_MODEL(model),
        NULL);
    gtk_drop_down_set_factory(GTK_DROP_DOWN(dd),
        GTK_LIST_ITEM_FACTORY(popup_factory));
    gtk_drop_down_set_header_factory(GTK_DROP_DOWN(dd),
        GTK_LIST_ITEM_FACTORY(header_factory));
    gtk_drop_down_set_enable_search(GTK_DROP_DOWN(dd), TRUE);
    gtk_widget_set_hexpand(dd, TRUE);
    gtk_widget_set_size_request(dd, 600, -1);
    return dd;
}

/*
 * _suppress_unused
 * ----------------
 * Silences the "defined but not used" compiler warning for show_alert().
 * show_alert is available for future use / debugging but is not currently
 * called from any signal handler (alerts are shown via label text instead).
 */
static void _suppress_unused(void) { (void)show_alert; }

/* Forward declarations for navigation callbacks used before they are defined */
static void on_back_home_guest(GtkButton *b, gpointer d);
static void on_back_home_admin(GtkButton *b, gpointer d);

/* =========================================================
 * Shopping cart (in-session only, not persisted)
 * =========================================================
 *
 * A guest's cart exists only for the duration of their session.
 * Each CartEntry mirrors the GiftRecord that was written to gifts.csv,
 * so the bill at the end can be generated from in-memory data.
 */
#define CART_MAX 50   /* maximum gifts a single guest may add in one session */

/* CartEntry – one line in the guest's in-session shopping cart */
typedef struct {
    int    record_id;       /* record ID already persisted to gifts.csv */
    int    item_id;         /* catalogue item ID                        */
    char   item_name[100];  /* item name (for bill display)             */
    int    quantity;        /* always 1 for single/group gift modes     */
    double total_fcfa;      /* total price in FCFA                      */
    double total_eur;       /* total price in EUR                       */
} CartEntry;

/*
 * RegisterWidgets – widget state for the "One Guest → Gift(s)" tab.
 *
 * guest_ok          – 1 once the guest's identity is verified
 * verified_guest    – populated by find_guest_by_name() on verify
 * cart[] / cart_count – in-session shopping cart
 * btn_add_another   – enabled once at least one gift is in the cart
 * btn_done_shop     – enabled once at least one gift is in the cart
 * btn_return_home   – shown (not just enabled) after the bill is generated
 */
typedef struct {
    GtkWidget  *entry_name;
    GtkWidget  *lbl_guest_status;
    GuestBrief  verified_guest;
    int         guest_ok;
    GtkWidget  *combo_item;
    GtkWidget  *lbl_unit_price;
    GtkWidget  *entry_thanked;
    GtkWidget  *lbl_result;
    GtkWidget  *lbl_taken_status;
    CartEntry   cart[CART_MAX];
    int         cart_count;
    GtkWidget  *lbl_cart;
    GtkWidget  *btn_add_another;
    GtkWidget  *btn_done_shop;
    GtkWidget  *btn_return_home;
} RegisterWidgets;

/*
 * on_reg_verify
 * -------------
 * "Verify Identity" button callback for the single-guest registration tab.
 * Reads the name entry, calls find_guest_by_name(), and updates the status
 * label.  Sets w->guest_ok = 1 on success so on_reg_submit() is unblocked.
 */
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

/*
 * on_reg_item_changed
 * -------------------
 * "notify::selected" signal handler for the catalogue dropdown on the
 * single-guest registration tab.
 *
 * Checks the selected item's remaining offer slots and:
 *   – If fully taken  → shows a red bold warning; applies "taken-item" CSS class.
 *   – If partially taken → shows an orange informational slot count.
 *   – If available    → clears the taken-status label.
 * Also calls show_unit_price() to keep the price label current.
 */
static void on_reg_item_changed(GObject *o, GParamSpec *ps, gpointer d) {
    (void)ps;
    RegisterWidgets *w = (RegisterWidgets *)d;
    guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(o));
    if ((int)idx >= cat_count) return;

    int already = count_gift_registrations(cat[idx].id);
    if (already >= GIFT_OFFER_LIMIT) {
        /* Gift is fully booked: show red warning, dim the dropdown */
        char msg[220];
        snprintf(msg, sizeof(msg),
                 "⛔  \"%s\" has already been offered %d times and is no longer available.  "
                 "Please choose a different gift.",
                 cat[idx].name, already);
        gtk_label_set_markup(GTK_LABEL(w->lbl_taken_status),
                             "<span foreground='red' weight='bold'>"
                             "⛔  This gift is fully taken — please select another one."
                             "</span>");
        (void)msg;
        gtk_widget_add_css_class(w->combo_item, "taken-item");
    } else {
        gtk_label_set_text(GTK_LABEL(w->lbl_taken_status), "");
        gtk_widget_remove_css_class(w->combo_item, "taken-item");
        if (already > 0) {
            /* Gift has some registrations but slots remain: show orange info */
            char slots[120];
            snprintf(slots, sizeof(slots),
                     "ℹ  %d of %d offer%s used for this gift  (%d slot%s remaining)",
                     already, GIFT_OFFER_LIMIT,
                     already == 1 ? "" : "s",
                     GIFT_OFFER_LIMIT - already,
                     (GIFT_OFFER_LIMIT - already) == 1 ? "" : "s");
            gtk_label_set_markup(GTK_LABEL(w->lbl_taken_status),
                                 "<span foreground='orange'>"
                                 "⚠  Getting full — see slot count below."
                                 "</span>");
            gtk_label_set_text(GTK_LABEL(w->lbl_taken_status), slots);
        }
    }
    show_unit_price(GTK_DROP_DOWN(o), GTK_LABEL(w->lbl_unit_price));
}

/*
 * reg_refresh_cart
 * ----------------
 * Rebuild the cart display label from the in-memory cart[] array.
 * Shows a formatted table of items with a grand total row.
 * Also enables/disables the "Add Another" and "Done – Go to Shop" buttons
 * depending on whether the cart is empty.
 */
static void reg_refresh_cart(RegisterWidgets *w) {
    if (w->cart_count == 0) {
        gtk_label_set_text(GTK_LABEL(w->lbl_cart),
                           "(no gifts in cart yet)");
        gtk_widget_set_sensitive(w->btn_done_shop, FALSE);
        gtk_widget_set_sensitive(w->btn_add_another, FALSE);
        return;
    }
    GString *sb = g_string_new(NULL);
    g_string_append(sb,
        "  #  | Gift Item                         | Qty |   Total FCFA  |  Total EUR\n"
        "-----|-----------------------------------|-----|---------------|------------\n");
    double grand_fcfa = 0, grand_eur = 0;
    for (int i = 0; i < w->cart_count; i++) {
        CartEntry *e = &w->cart[i];
        g_string_append_printf(sb,
            " %-3d | %-33s | %-3d | %13.0f | %10.2f\n",
            i + 1, e->item_name, e->quantity, e->total_fcfa, e->total_eur);
        grand_fcfa += e->total_fcfa;
        grand_eur  += e->total_eur;
    }
    /* Grand total row; the padding accounts for 1- vs 2-digit item counts */
    g_string_append_printf(sb,
        "-----|-----------------------------------|-----|---------------|------------\n"
        "     | GRAND TOTAL (%d gift%s)%*s| %3s | %13.0f | %10.2f\n",
        w->cart_count, w->cart_count == 1 ? " " : "s",
        (int)(19 - (w->cart_count > 9 ? 2 : 1)), "",
        "", grand_fcfa, grand_eur);
    gtk_label_set_text(GTK_LABEL(w->lbl_cart), sb->str);
    g_string_free(sb, TRUE);
    gtk_widget_set_sensitive(w->btn_done_shop,    TRUE);
    gtk_widget_set_sensitive(w->btn_add_another,  TRUE);
}

/*
 * guest_already_offered
 * ---------------------
 * Check whether guest_id has already registered item_id in gifts.csv from
 * a previous session.  Prevents a guest from offering the same gift twice
 * even if they close and reopen the application.
 *
 * Returns 1 if a matching record exists; 0 otherwise.
 */
static int guest_already_offered(int guest_id, int item_id) {
    FILE *f = fopen(GIFT_CSV, "r");
    if (!f) return 0;
    char line[640];
    GiftRecord g;
    while (fgets(line, sizeof(line), f)) {
        if (parse_gift_line(line, &g)
                && g.guest_id == guest_id
                && g.item_id  == item_id) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

/*
 * on_reg_submit  ("Add to Cart" button)
 * -------------
 * Validates and records a single gift for the verified guest.
 *
 * Validation order:
 *   1. Guest must be verified (guest_ok == 1).
 *   2. A valid catalogue item must be selected.
 *   3. Cart must not be full (CART_MAX).
 *   4. Item must not already be in the session cart (in-session dedup).
 *   5. Guest must not have offered this item in a previous session (CSV dedup).
 *   6. Item must have remaining offer slots (< GIFT_OFFER_LIMIT).
 *
 * On success:
 *   - A GiftRecord is built and appended to gifts.csv.
 *   - A CartEntry is appended to w->cart[].
 *   - If the item just reached GIFT_OFFER_LIMIT, mark_item_taken_in_models()
 *     is called to update all live dropdowns immediately.
 *   - reg_refresh_cart() redraws the cart table.
 */
static void on_reg_submit(GtkButton *b, gpointer d) {
    (void)b;
    RegisterWidgets *w = (RegisterWidgets *)d;
    if (!w->guest_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Please verify your identity first."); return;
    }
    guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(w->combo_item));
    if ((int)idx >= cat_count) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Please select a gift item."); return;
    }
    if (w->cart_count >= CART_MAX) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Cart is full (50 gifts max). Please finalise first."); return;
    }
    const GiftItem *chosen = &cat[idx];
    int qty = 1;   /* quantity fixed at 1 per guest */
    const char *thanked =
        gtk_editable_get_text(GTK_EDITABLE(w->entry_thanked));

    /* In-session duplicate check: same item already added this session */
    for (int ci = 0; ci < w->cart_count; ci++) {
        if (w->cart[ci].item_id == chosen->id) {
            char dup_msg[220];
            snprintf(dup_msg, sizeof(dup_msg),
                     "You have already added \"%s\" to your cart. "
                     "Each guest may only offer the same gift once.",
                     chosen->name);
            gtk_label_set_text(GTK_LABEL(w->lbl_result), dup_msg);
            return;
        }
    }

    /* Cross-session duplicate check: guest already offered this item before */
    if (guest_already_offered(w->verified_guest.id, chosen->id)) {
        char dup_msg[220];
        snprintf(dup_msg, sizeof(dup_msg),
                 "You have already offered \"%s\" in a previous session. "
                 "Each guest may only offer the same gift once.",
                 chosen->name);
        gtk_label_set_text(GTK_LABEL(w->lbl_result), dup_msg);
        return;
    }

    /* Offer-limit check */
    int already = count_gift_registrations(chosen->id);
    if (already >= GIFT_OFFER_LIMIT) {
        char cap_msg[220];
        snprintf(cap_msg, sizeof(cap_msg),
                 "Sorry — \"%s\" has already been offered %d time%s "
                 "(limit: %d).  Please choose a different gift.",
                 chosen->name, already,
                 already == 1 ? "" : "s",
                 GIFT_OFFER_LIMIT);
        gtk_label_set_text(GTK_LABEL(w->lbl_result), cap_msg);
        return;
    }

    /* Build the GiftRecord and persist it */
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
    /* Use the thank-you name entry if non-empty; fall back to registered name */
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

    /* Mirror the persisted record into the in-session cart */
    CartEntry *ce = &w->cart[w->cart_count++];
    ce->record_id  = g.record_id;
    ce->item_id    = chosen->id;
    strncpy(ce->item_name, g.item_name, sizeof(ce->item_name) - 1);
    ce->quantity   = g.quantity;
    ce->total_fcfa = g.total_fcfa;
    ce->total_eur  = g.total_eur;

    /* If this registration just filled the last slot, update all dropdowns */
    if (already + 1 >= GIFT_OFFER_LIMIT) {
        for (int ci = 0; ci < cat_count; ci++) {
            if (cat[ci].id == chosen->id) {
                mark_item_taken_in_models(ci);
                break;
            }
        }
        gtk_label_set_markup(GTK_LABEL(w->lbl_taken_status),
                             "<span foreground='red' weight='bold'>"
                             "⛔  This gift is now fully taken."
                             "</span>");
        gtk_widget_add_css_class(w->combo_item, "taken-item");
    } else {
        gtk_label_set_text(GTK_LABEL(w->lbl_taken_status), "");
        gtk_widget_remove_css_class(w->combo_item, "taken-item");
    }

    /* Compose the success message showing slot usage */
    int remaining = GIFT_OFFER_LIMIT - (already + 1);
    char msg[320];
    snprintf(msg, sizeof(msg),
             "✔ Added to cart!  Record ID: %d\n"
             "%s  –  %.0f FCFA  (%.2f EUR)\n"
             "(%d/%d offer%s used for this gift — %d slot%s remaining)\n"
             "You can add more gifts below, or click \"Done – Go to Shop\" to finalise.",
             g.record_id, g.item_name,
             g.total_fcfa, g.total_eur,
             already + 1, GIFT_OFFER_LIMIT,
             (already + 1) == 1 ? "" : "s",
             remaining,
             remaining == 1 ? "" : "s");
    gtk_label_set_text(GTK_LABEL(w->lbl_result), msg);

    reg_refresh_cart(w);

    /* Clear the thank-you field so it is not accidentally reused */
    gtk_editable_set_text(GTK_EDITABLE(w->entry_thanked), "");
}

/* "Add Another Gift" – just clears the result label and scrolls focus back */
static void on_reg_add_another(GtkButton *b, gpointer d) {
    (void)b;
    RegisterWidgets *w = (RegisterWidgets *)d;
    gtk_label_set_text(GTK_LABEL(w->lbl_result), "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_thanked), "");
    gtk_widget_grab_focus(w->combo_item);
}

/* ---- Bill window shown on "Done – Go to Shop" ---- */

/* on_bill_ok – close the bill summary window */
static void on_bill_ok(GtkButton *btn, gpointer win) {
    (void)btn;
    gtk_window_destroy(GTK_WINDOW(win));
}

/*
 * on_reg_done_shop  ("Done – Generate Bill & Go to Shop" button)
 * ----------------
 * Builds a formatted text bill from the in-session cart, displays it in
 * a modal summary window, opens the checkout URL in the default browser,
 * and resets the registration tab for the next guest.
 *
 * The bill window is non-blocking (gtk_window_present, not a dialog loop)
 * so the guest can refer to it while the browser is opening.
 */
static void on_reg_done_shop(GtkButton *b, gpointer d) {
    (void)b;
    RegisterWidgets *w = (RegisterWidgets *)d;
    if (w->cart_count == 0) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Your cart is empty. Add at least one gift first."); return;
    }

    /* Build the bill string */
    GString *bill = g_string_new(NULL);
    g_string_append(bill,
        "╔══════════════════════════════════════════════════════════════╗\n"
        "║          WEDDING GIFT MANAGER  –  PAYMENT BILL               ║\n"
        "╚══════════════════════════════════════════════════════════════╝\n\n");
    g_string_append_printf(bill, "  Guest : %s\n\n",
                           w->verified_guest.name);
    g_string_append(bill,
        "  #   Gift Item                         Qty     Total FCFA    Total EUR\n"
        " ─────────────────────────────────────────────────────────────────────\n");
    double grand_fcfa = 0, grand_eur = 0;
    for (int i = 0; i < w->cart_count; i++) {
        CartEntry *e = &w->cart[i];
        g_string_append_printf(bill,
            "  %-2d  %-33s  %-4d  %12.0f  %10.2f\n",
            i + 1, e->item_name, e->quantity,
            e->total_fcfa, e->total_eur);
        grand_fcfa += e->total_fcfa;
        grand_eur  += e->total_eur;
    }
    g_string_append(bill,
        " ─────────────────────────────────────────────────────────────────────\n");
    g_string_append_printf(bill,
        "  TOTAL                                       %12.0f  %10.2f\n\n",
        grand_fcfa, grand_eur);
    g_string_append(bill,
        "  Please proceed to our online shop to complete your payment.\n"
        "  Your gifts will be prepared and ready for collection after payment.\n\n"
        "  → https://group3wed.shop/checkout\n");

    /* Open the bill in a modal summary window */
    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Your Bill – Wedding Gift Manager");
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 660, 420);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 16);
    gtk_widget_set_margin_bottom(vbox, 16);
    gtk_window_set_child(GTK_WINDOW(win), vbox);

    GtkWidget *title_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_lbl),
                         "<b>Your Gift Order Summary</b>");
    gtk_label_set_xalign(GTK_LABEL(title_lbl), 0.5f);
    gtk_box_append(GTK_BOX(vbox), title_lbl);

    /* Monospace text view for the bill – read-only */
    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tv), FALSE);
    gtk_widget_set_vexpand(tv, TRUE);
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_set_text(buf, bill->str, -1);
    g_string_free(bill, TRUE);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), tv);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(vbox), scroll);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), btn_row);

    GtkWidget *shop_btn = gtk_button_new_with_label("🛒  Open Online Shop / Pay");
    g_signal_connect_swapped(shop_btn, "clicked",
        G_CALLBACK(gtk_window_present), win); /* focus bill window; browser opened below */
    gtk_box_append(GTK_BOX(btn_row), shop_btn);

    GtkWidget *ok_btn = gtk_button_new_with_label("  Close");
    g_signal_connect(ok_btn, "clicked", G_CALLBACK(on_bill_ok), win);
    gtk_box_append(GTK_BOX(btn_row), ok_btn);

    gtk_window_present(GTK_WINDOW(win));

    /* Open checkout URL in the system's default browser */
    g_app_info_launch_default_for_uri(
        "https://group3wed.shop/checkout", NULL, NULL);

    /* Reset the registration tab for the next guest */
    w->cart_count = 0;
    w->guest_ok   = 0;
    gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), "");
    gtk_label_set_text(GTK_LABEL(w->lbl_result),
                       "Your order is complete! You can register another gift or return to the panel selection.");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),    "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_thanked), "");
    reg_refresh_cart(w);

    /* Make the "Return to Panel Selection" button visible */
    gtk_widget_set_visible(w->btn_return_home, TRUE);
}

/*
 * SECTION_LBL(box, txt)
 * ----------------------
 * Convenience macro: append a bold section header label to a GtkBox.
 * Uses Pango markup (<b>…</b>) and left-aligns the text.
 * Wrapped in do { … } while (0) so it behaves like a statement.
 */
#define SECTION_LBL(box, txt) \
    do { \
        GtkWidget *_h = gtk_label_new(NULL); \
        gtk_label_set_markup(GTK_LABEL(_h), "<b>" txt "</b>"); \
        gtk_label_set_xalign(GTK_LABEL(_h), 0.0f); \
        gtk_box_append(GTK_BOX(box), _h); \
    } while (0)

/*
 * build_register_tab
 * ------------------
 * Construct the "One Guest → Gift(s)" tab widget.
 *
 * Layout (top to bottom):
 *   Instructional note
 *   ── separator ──
 *   Step 1: Verify identity (name entry + Verify button + status label)
 *   ── separator ──
 *   Step 2: Choose your gift (dropdown + taken-status label + unit-price label)
 *   ── separator ──
 *   Step 3: Thank-you name (optional entry)
 *   [+ Add Gift to Cart] button  +  result label
 *   ── separator ──
 *   Your Gift Cart (monospace cart table label)
 *   [Add Another Gift]  [Done – Generate Bill & Go to Shop]  [Return Home]
 */
static GtkWidget *build_register_tab(RegisterWidgets *w) {
    memset(w, 0, sizeof(*w));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    GtkWidget *note = gtk_label_new(
        "No password required to register a gift. "
        "Enter your full name to verify your identity.");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(note), TRUE);
    gtk_box_append(GTK_BOX(box), note);
    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

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

    SECTION_LBL(box, "Step 2 : Choose your gift");
    w->combo_item = build_gift_dropdown();
    gtk_box_append(GTK_BOX(box), w->combo_item);

    /* Taken-status label: shows red warning when the selected gift is fully booked */
    w->lbl_taken_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_taken_status), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_taken_status), TRUE);
    gtk_box_append(GTK_BOX(box), w->lbl_taken_status);

    w->lbl_unit_price = gtk_label_new("Price: ");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_unit_price), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_unit_price);

    /* Wire up the dropdown change signal to update price and taken status */
    g_signal_connect(w->combo_item, "notify::selected",
                     G_CALLBACK(on_reg_item_changed), w);
    show_unit_price(GTK_DROP_DOWN(w->combo_item),
                    GTK_LABEL(w->lbl_unit_price));
    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    SECTION_LBL(box, "Step 3 : Thank-you name (optional)");
    w->entry_thanked = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_thanked),
        "e.g. Aunt Marie  (blank = use registered name)");
    gtk_box_append(GTK_BOX(box), w->entry_thanked);

    GtkWidget *bs = gtk_button_new_with_label("  + Add Gift to Cart");
    g_signal_connect(bs, "clicked", G_CALLBACK(on_reg_submit), w);
    gtk_box_append(GTK_BOX(box), bs);

    w->lbl_result = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(w->lbl_result), TRUE);
    gtk_label_set_xalign(GTK_LABEL(w->lbl_result), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_result);

    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    SECTION_LBL(box, "Your Gift Cart");

    /* Monospace cart table label – uses PangoAttrList for the font family */
    w->lbl_cart = gtk_label_new("(no gifts in cart yet)");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_cart), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_cart), FALSE);
    gtk_widget_set_margin_start(w->lbl_cart, 4);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_family_new("Monospace"));
    gtk_label_set_attributes(GTK_LABEL(w->lbl_cart), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_append(GTK_BOX(box), w->lbl_cart);

    GtkWidget *action_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_margin_top(action_row, 8);
    gtk_box_append(GTK_BOX(box), action_row);

    /* "Add Another Gift" – disabled until at least one item is in the cart */
    w->btn_add_another = gtk_button_new_with_label("  Add Another Gift");
    gtk_widget_set_sensitive(w->btn_add_another, FALSE);
    g_signal_connect(w->btn_add_another, "clicked",
                     G_CALLBACK(on_reg_add_another), w);
    gtk_box_append(GTK_BOX(action_row), w->btn_add_another);

    /* "Done" – disabled until at least one item is in the cart */
    w->btn_done_shop = gtk_button_new_with_label("  Done - Generate Bill & Go to Shop");
    gtk_widget_set_sensitive(w->btn_done_shop, FALSE);
    g_signal_connect(w->btn_done_shop, "clicked",
                     G_CALLBACK(on_reg_done_shop), w);
    gtk_box_append(GTK_BOX(action_row), w->btn_done_shop);

    /* "Return Home" – hidden until the bill has been generated */
    w->btn_return_home = gtk_button_new_with_label("⌂  Return to Panel Selection");
    gtk_widget_set_visible(w->btn_return_home, FALSE);
    g_signal_connect(w->btn_return_home, "clicked",
                     G_CALLBACK(on_back_home_guest), NULL);
    gtk_box_append(GTK_BOX(action_row), w->btn_return_home);

    return box;
}

/* =========================================================
 * Group Gift registration tab  (Guest Panel – 2nd tab)
 * =========================================================
 *
 * Allows 2 to GROUP_MAX guests to jointly offer one catalogue item.
 * Each guest's name is entered in a separate row and verified individually.
 * The "Register Group Gift" button is kept disabled until at least 2 names
 * pass verification.
 */
#define GROUP_MAX 20   /* max co-givers in one group registration */

/*
 * GroupRegWidgets – widget state for the "Many Guests → One Gift" tab.
 *
 * entry_name[] / lbl_verify[] – one row per co-giver (up to GROUP_MAX)
 * guests[] / guest_ok[]       – results of find_guest_by_name() per row
 * name_count                  – rows currently visible
 * names_box                   – VBox that rows are appended to dynamically
 * btn_add_name                – disabled at GROUP_MAX
 * btn_submit                  – enabled only when ok_count >= 2
 */
typedef struct {
    /* Name rows */
    GtkWidget  *entry_name[GROUP_MAX];
    GtkWidget  *lbl_verify[GROUP_MAX];  /* per-row status */
    GuestBrief  guests[GROUP_MAX];
    int         guest_ok[GROUP_MAX];
    int         name_count;             /* rows currently shown */
    GtkWidget  *names_box;             /* VBox holding all name rows */
    GtkWidget  *btn_add_name;
    GtkWidget  *btn_verify_all;
    /* Gift selection */
    GtkWidget  *combo_item;
    GtkWidget  *lbl_unit_price;
    GtkWidget  *lbl_taken_status;
    GtkWidget  *entry_thanked;
    /* Result / navigation */
    GtkWidget  *lbl_result;
    GtkWidget  *btn_submit;
    GtkWidget  *btn_return_home;
} GroupRegWidgets;

/* grp_verified_count – count how many name rows are currently verified */
static int grp_verified_count(GroupRegWidgets *w) {
    int n = 0;
    for (int i = 0; i < w->name_count; i++)
        if (w->guest_ok[i]) n++;
    return n;
}

/*
 * grp_update_ui_state
 * -------------------
 * Rebuild the summary status label and enable/disable the Submit button
 * based on how many names are currently verified.
 * Called after every verify-all operation and after adding a new name row.
 */
static void grp_update_ui_state(GroupRegWidgets *w) {
    int ok = grp_verified_count(w);
    int total = w->name_count;

    /* Submit requires at least 2 verified names */
    gtk_widget_set_sensitive(w->btn_submit, ok >= 2);

    char msg[160];
    if (ok < 2)
        snprintf(msg, sizeof(msg),
                 "ℹ  %d/%d name%s verified  —  at least 2 required to submit.",
                 ok, total, total == 1 ? "" : "s");
    else
        snprintf(msg, sizeof(msg),
                 "✔  %d/%d name%s verified  —  ready to submit.",
                 ok, total, total == 1 ? "" : "s");
    gtk_label_set_text(GTK_LABEL(w->lbl_result), msg);
}

/*
 * on_grp_verify_all
 * -----------------
 * "Verify All Names" button callback.
 * Iterates over all visible name rows, calls find_guest_by_name() for each,
 * checks for duplicate guest IDs among already-verified rows, and updates
 * each per-row status label.  Finally calls grp_update_ui_state().
 */
static void on_grp_verify_all(GtkButton *b, gpointer d) {
    (void)b;
    GroupRegWidgets *w = (GroupRegWidgets *)d;
    for (int i = 0; i < w->name_count; i++) {
        const char *nm = gtk_editable_get_text(GTK_EDITABLE(w->entry_name[i]));
        if (strlen(nm) < 1) {
            w->guest_ok[i] = 0;
            gtk_label_set_text(GTK_LABEL(w->lbl_verify[i]), "⚠ Enter a name.");
            continue;
        }
        if (find_guest_by_name(nm, &w->guests[i])) {
            /* Check for duplicates among already-verified rows */
            int dup = 0;
            for (int j = 0; j < i; j++) {
                if (w->guest_ok[j] && w->guests[j].id == w->guests[i].id) {
                    dup = 1; break;
                }
            }
            if (dup) {
                w->guest_ok[i] = 0;
                char dm[120];
                snprintf(dm, sizeof(dm), "✗ Duplicate: %s already in list.", w->guests[i].name);
                gtk_label_set_text(GTK_LABEL(w->lbl_verify[i]), dm);
            } else {
                w->guest_ok[i] = 1;
                char vm[120];
                snprintf(vm, sizeof(vm), "✔ %s (ID %d)", w->guests[i].name, w->guests[i].id);
                gtk_label_set_text(GTK_LABEL(w->lbl_verify[i]), vm);
            }
        } else {
            w->guest_ok[i] = 0;
            gtk_label_set_text(GTK_LABEL(w->lbl_verify[i]),
                               "✗ Not found. Check spelling.");
        }
    }
    grp_update_ui_state(w);
}

/*
 * on_grp_item_changed
 * -------------------
 * "notify::selected" signal handler for the group-gift dropdown.
 * Shows the taken-item CSS class and red warning when the selected item
 * is at its offer limit.  Also calls show_unit_price().
 */
static void on_grp_item_changed(GObject *o, GParamSpec *ps, gpointer d) {
    (void)ps;
    GroupRegWidgets *w = (GroupRegWidgets *)d;
    guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(o));
    if ((int)idx >= cat_count) return;
    int already = count_gift_registrations(cat[idx].id);
    if (already >= GIFT_OFFER_LIMIT) {
        gtk_label_set_markup(GTK_LABEL(w->lbl_taken_status),
                             "<span foreground='red' weight='bold'>"
                             "⛔  This gift is fully taken — please select another one."
                             "</span>");
        gtk_widget_add_css_class(w->combo_item, "taken-item");
    } else {
        gtk_label_set_text(GTK_LABEL(w->lbl_taken_status), "");
        gtk_widget_remove_css_class(w->combo_item, "taken-item");
    }
    show_unit_price(GTK_DROP_DOWN(o), GTK_LABEL(w->lbl_unit_price));
}

/*
 * on_grp_add_name
 * ---------------
 * "＋ Add Another Co-giver" button callback.
 * Dynamically appends a new name-entry row to w->names_box.
 * Disables the Add button once GROUP_MAX rows are visible.
 * Calls grp_update_ui_state() to refresh the summary.
 */
static void on_grp_add_name(GtkButton *b, gpointer d) {
    (void)b;
    GroupRegWidgets *w = (GroupRegWidgets *)d;
    if (w->name_count >= GROUP_MAX) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Maximum number of co-givers reached.");
        return;
    }
    int i = w->name_count;

    /* Build a horizontal row: "Guest N:" label + entry + verify status label */
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    char lbl_txt[16];
    snprintf(lbl_txt, sizeof(lbl_txt), "Guest %d:", i + 1);
    gtk_box_append(GTK_BOX(row), gtk_label_new(lbl_txt));

    w->entry_name[i] = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name[i]),
                                   "Full name as registered");
    gtk_widget_set_hexpand(w->entry_name[i], TRUE);
    gtk_box_append(GTK_BOX(row), w->entry_name[i]);

    w->lbl_verify[i] = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_verify[i]), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_verify[i]), TRUE);
    gtk_widget_set_hexpand(w->lbl_verify[i], TRUE);
    gtk_box_append(GTK_BOX(row), w->lbl_verify[i]);

    gtk_box_append(GTK_BOX(w->names_box), row);

    w->guest_ok[i] = 0;
    w->name_count++;

    /* Disable the Add button when the maximum is reached */
    if (w->name_count >= GROUP_MAX)
        gtk_widget_set_sensitive(w->btn_add_name, FALSE);

    grp_update_ui_state(w);
}

/*
 * on_grp_submit
 * -------------
 * "Register Group Gift" button callback.
 *
 * Validation:
 *   1. At least 2 verified guests.
 *   2. A valid catalogue item is selected.
 *   3. ok_count + already_registered <= GIFT_OFFER_LIMIT (slot check).
 *
 * For each verified, non-duplicate guest a GiftRecord is written to gifts.csv.
 * Guests who already offered this item in a previous session are skipped with
 * a note in the summary.  After writing, mark_item_taken_in_models() is called
 * if the item is now at its limit.  The form is then reset.
 */
static void on_grp_submit(GtkButton *b, gpointer d) {
    (void)b;
    GroupRegWidgets *w = (GroupRegWidgets *)d;

    int ok_count = grp_verified_count(w);
    if (ok_count < 2) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "At least 2 verified guests are required.");
        return;
    }

    guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(w->combo_item));
    if ((int)idx >= cat_count) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Please select a gift item.");
        return;
    }
    const GiftItem *chosen = &cat[idx];

    /* Ensure there are enough offer slots for all co-givers */
    int already = count_gift_registrations(chosen->id);
    if (already + ok_count > GIFT_OFFER_LIMIT) {
        char cap_msg[280];
        snprintf(cap_msg, sizeof(cap_msg),
                 "Sorry — only %d slot%s remain for \"%s\" "
                 "but you have %d co-givers. "
                 "Please choose a different gift or reduce the number of co-givers.",
                 GIFT_OFFER_LIMIT - already,
                 (GIFT_OFFER_LIMIT - already) == 1 ? "" : "s",
                 chosen->name, ok_count);
        gtk_label_set_text(GTK_LABEL(w->lbl_result), cap_msg);
        return;
    }

    int qty = 1;   /* quantity fixed at 1 per guest */
    const char *thanked = gtk_editable_get_text(GTK_EDITABLE(w->entry_thanked));

    FILE *f = fopen(GIFT_CSV, "a");
    if (!f) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Error: could not open gifts.csv");
        return;
    }

    GString *summary = g_string_new("✔ Group gift registered!\n\nRecords written:\n");
    int new_registrations = 0;

    for (int i = 0; i < w->name_count; i++) {
        if (!w->guest_ok[i]) continue;

        /* Skip guests who already offered this item in a previous session */
        if (guest_already_offered(w->guests[i].id, chosen->id)) {
            g_string_append_printf(summary,
                "  ⚠ Skipped %s — already offered this gift before.\n",
                w->guests[i].name);
            continue;
        }

        GiftRecord g;
        memset(&g, 0, sizeof(g));
        g.record_id  = get_next_gift_id();
        g.guest_id   = w->guests[i].id;
        strncpy(g.guest_name, w->guests[i].name, sizeof(g.guest_name) - 1);
        find_category_for_guest(g.guest_id, g.category, sizeof(g.category));
        g.item_id    = chosen->id;
        strncpy(g.item_name, chosen->name, sizeof(g.item_name) - 1);
        g.quantity   = qty;
        g.total_fcfa = chosen->price_fcfa * qty;
        g.total_eur  = g.total_fcfa / EUR_RATE;
        /* Use shared thank-you name if set; otherwise each guest's own name */
        if (strlen(thanked) >= 1)
            strncpy(g.thanked_by, thanked,          sizeof(g.thanked_by) - 1);
        else
            strncpy(g.thanked_by, w->guests[i].name, sizeof(g.thanked_by) - 1);

        write_gift_line(f, &g);
        g_string_append_printf(summary,
            "  #%d  %s  →  %s  =  %.0f FCFA\n",
            g.record_id, g.guest_name, g.item_name, g.total_fcfa);
        new_registrations++;
    }
    fclose(f);

    /* Check if the item is now at its limit and update dropdowns if so */
    int total_now = count_gift_registrations(chosen->id);
    if (total_now >= GIFT_OFFER_LIMIT) {
        for (int ci = 0; ci < cat_count; ci++) {
            if (cat[ci].id == chosen->id) {
                mark_item_taken_in_models(ci);
                break;
            }
        }
    }

    if (new_registrations == 0) {
        g_string_append(summary,
            "\n  All guests had already offered this gift. Nothing was saved.");
    } else {
        g_string_append_printf(summary,
            "\nGift: %s  =  %.0f FCFA (%.2f EUR) per person.",
            chosen->name, chosen->price_fcfa,
            chosen->price_fcfa / EUR_RATE);
    }

    gtk_label_set_text(GTK_LABEL(w->lbl_result), summary->str);
    g_string_free(summary, TRUE);

    /* Show the Return Home button after a successful submission */
    gtk_widget_set_visible(w->btn_return_home, TRUE);

    /* Reset all name rows and the form state */
    for (int i = 0; i < w->name_count; i++) {
        gtk_editable_set_text(GTK_EDITABLE(w->entry_name[i]), "");
        gtk_label_set_text(GTK_LABEL(w->lbl_verify[i]), "");
        w->guest_ok[i] = 0;
    }
    gtk_editable_set_text(GTK_EDITABLE(w->entry_thanked), "");
    gtk_widget_set_sensitive(w->btn_submit, FALSE);
}

/*
 * build_group_register_tab
 * ------------------------
 * Construct the "Many Guests → One Gift" tab widget.
 *
 * Starts with 2 name rows pre-built (the minimum required).
 * The "＋ Add Another Co-giver" button appends rows dynamically up to GROUP_MAX.
 *
 * Layout (top to bottom):
 *   Instructional note
 *   ── separator ──
 *   Step 1: Name rows VBox  +  [Add Co-giver] [Verify All Names]
 *   ── separator ──
 *   Step 2: Gift dropdown + taken-status label + unit-price label
 *   ── separator ──
 *   Step 3: Shared thank-you name (optional)
 *   [Register Group Gift] button  (disabled until ≥ 2 verified)
 *   Result label
 *   [Return to Panel Selection] (hidden until after successful submit)
 */
static GtkWidget *build_group_register_tab(GroupRegWidgets *w) {
    memset(w, 0, sizeof(*w));

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    GtkWidget *note = gtk_label_new(
        "Register one gift offered jointly by multiple guests (minimum 2). "
        "All names must be verified before submitting.");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(note), TRUE);
    gtk_box_append(GTK_BOX(box), note);
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    SECTION_LBL(box, "Step 1 : Enter co-givers' names (minimum 2)");

    /* Dynamic names container – new rows are appended here by on_grp_add_name */
    w->names_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(box), w->names_box);

    /* Pre-build the first 2 rows (minimum required) */
    w->name_count = 0;
    for (int i = 0; i < 2; i++) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        char lbl_txt[16];
        snprintf(lbl_txt, sizeof(lbl_txt), "Guest %d:", i + 1);
        gtk_box_append(GTK_BOX(row), gtk_label_new(lbl_txt));

        w->entry_name[i] = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_name[i]),
                                       "Full name as registered");
        gtk_widget_set_hexpand(w->entry_name[i], TRUE);
        gtk_box_append(GTK_BOX(row), w->entry_name[i]);

        w->lbl_verify[i] = gtk_label_new("");
        gtk_label_set_xalign(GTK_LABEL(w->lbl_verify[i]), 0.0f);
        gtk_label_set_wrap(GTK_LABEL(w->lbl_verify[i]), TRUE);
        gtk_widget_set_hexpand(w->lbl_verify[i], TRUE);
        gtk_box_append(GTK_BOX(row), w->lbl_verify[i]);

        gtk_box_append(GTK_BOX(w->names_box), row);
        w->guest_ok[i] = 0;
        w->name_count++;
    }

    GtkWidget *name_btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(name_btn_row, 4);
    gtk_box_append(GTK_BOX(box), name_btn_row);

    w->btn_add_name = gtk_button_new_with_label("＋ Add Another Co-giver");
    g_signal_connect(w->btn_add_name, "clicked", G_CALLBACK(on_grp_add_name), w);
    gtk_box_append(GTK_BOX(name_btn_row), w->btn_add_name);

    w->btn_verify_all = gtk_button_new_with_label("✔ Verify All Names");
    g_signal_connect(w->btn_verify_all, "clicked", G_CALLBACK(on_grp_verify_all), w);
    gtk_box_append(GTK_BOX(name_btn_row), w->btn_verify_all);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    SECTION_LBL(box, "Step 2 : Choose the gift");

    w->combo_item = build_gift_dropdown();
    gtk_box_append(GTK_BOX(box), w->combo_item);

    w->lbl_taken_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_taken_status), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_taken_status), TRUE);
    gtk_box_append(GTK_BOX(box), w->lbl_taken_status);

    w->lbl_unit_price = gtk_label_new("Price: ");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_unit_price), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_unit_price);

    g_signal_connect(w->combo_item, "notify::selected",
                     G_CALLBACK(on_grp_item_changed), w);
    show_unit_price(GTK_DROP_DOWN(w->combo_item),
                    GTK_LABEL(w->lbl_unit_price));

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    SECTION_LBL(box, "Step 3 : Thank-you name (optional)");
    w->entry_thanked = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_thanked),
        "e.g. The Smith Family  (blank = each guest's own name)");
    gtk_box_append(GTK_BOX(box), w->entry_thanked);

    /* Submit button starts disabled; grp_update_ui_state enables it when ready */
    w->btn_submit = gtk_button_new_with_label(
        "  Register Group Gift  (requires ≥ 2 verified names)");
    gtk_widget_set_sensitive(w->btn_submit, FALSE);
    g_signal_connect(w->btn_submit, "clicked", G_CALLBACK(on_grp_submit), w);
    gtk_box_append(GTK_BOX(box), w->btn_submit);

    w->lbl_result = gtk_label_new(
        "ℹ  Enter at least 2 names and click \"Verify All Names\" to continue.");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_result), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_result), TRUE);
    gtk_box_append(GTK_BOX(box), w->lbl_result);

    /* Hidden until after a successful submission */
    w->btn_return_home = gtk_button_new_with_label("⌂  Return to Panel Selection");
    gtk_widget_set_visible(w->btn_return_home, FALSE);
    gtk_widget_set_halign(w->btn_return_home, GTK_ALIGN_START);
    gtk_widget_set_margin_top(w->btn_return_home, 6);
    g_signal_connect(w->btn_return_home, "clicked",
                     G_CALLBACK(on_back_home_guest), NULL);
    gtk_box_append(GTK_BOX(box), w->btn_return_home);

    return box;
}

/* =========================================================
 * Financial Contribution tab (Guest Panel – 3rd tab)
 * =========================================================
 *
 * Allows a guest to record a cash contribution instead of a physical gift.
 * The contribution is stored in gifts.csv with item_id = 0 and the item_name
 * encoding the chosen payment method.
 *
 * Payment methods:
 *   Mobile Money  – opens an Orange Money or MTN MoMo payment URL in the browser
 *   Credit Card   – displays bank transfer details and opens the bank's portal
 */
typedef struct {
    GtkWidget  *entry_name;
    GtkWidget  *lbl_guest_status;
    GuestBrief  verified_guest;
    int         guest_ok;

    GtkWidget  *entry_amount;       /* amount in FCFA  */
    GtkWidget  *radio_momo;         /* Mobile Money    */
    GtkWidget  *radio_card;         /* Credit Card     */
    GtkWidget  *lbl_result;
    GtkWidget  *btn_return_home;
} FinContribWidgets;

static FinContribWidgets fin_w;   /* single global instance, like reg_w / grp_w */

/*
 * on_fin_verify
 * -------------
 * "Verify Identity" callback for the financial contribution tab.
 * Identical logic to on_reg_verify; uses fin_w fields.
 */
static void on_fin_verify(GtkButton *b, gpointer d) {
    (void)b;
    FinContribWidgets *w = (FinContribWidgets *)d;
    const char *nm = gtk_editable_get_text(GTK_EDITABLE(w->entry_name));
    if (strlen(nm) < 1) {
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
                           "Please enter your full name.");
        return;
    }
    if (find_guest_by_name(nm, &w->verified_guest)) {
        w->guest_ok = 1;
        char msg[160];
        snprintf(msg, sizeof(msg), "✔ Verified: %s", w->verified_guest.name);
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), msg);
    } else {
        w->guest_ok = 0;
        gtk_label_set_text(GTK_LABEL(w->lbl_guest_status),
            "No guest found with that name. "
            "Please re-enter the correct and corresponding name.");
    }
}

/*
 * on_fin_submit
 * -------------
 * "Submit Financial Contribution" button callback.
 *
 * Validation:
 *   1. Guest must be verified.
 *   2. Amount must be a positive number.
 *
 * Builds a GiftRecord with item_id = 0 (financial contribution sentinel)
 * and item_name encoding the payment method.  Writes to gifts.csv.
 *
 * For Mobile Money: builds an operator-specific payment URL and opens it.
 * For Credit Card : displays bank transfer details and opens the bank portal.
 * In both cases the form is reset after a successful submission.
 */
static void on_fin_submit(GtkButton *b, gpointer d) {
    (void)b;
    FinContribWidgets *w = (FinContribWidgets *)d;

    if (!w->guest_ok) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Please verify your identity first.");
        return;
    }

    const char *amt_str = gtk_editable_get_text(GTK_EDITABLE(w->entry_amount));
    char *end;
    double amount = strtod(amt_str, &end);
    if (end == amt_str || amount <= 0.0) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Please enter a valid contribution amount (positive number).");
        return;
    }

    /* Determine payment method from the radio button group */
    int use_momo = gtk_check_button_get_active(GTK_CHECK_BUTTON(w->radio_momo));
    const char *method = use_momo ? "Mobile Money" : "Credit Card / Bank Transfer";

    /* Build a GiftRecord using item_id = 0 for financial contributions */
    GiftRecord g;
    memset(&g, 0, sizeof(g));
    g.record_id  = get_next_gift_id();
    g.guest_id   = w->verified_guest.id;
    strncpy(g.guest_name, w->verified_guest.name, sizeof(g.guest_name) - 1);
    find_category_for_guest(g.guest_id, g.category, sizeof(g.category));
    g.item_id    = 0;
    snprintf(g.item_name, sizeof(g.item_name),
             "Financial Contribution [%s]", method);
    g.quantity   = 1;
    g.total_fcfa = amount;
    g.total_eur  = amount / EUR_RATE;
    strncpy(g.thanked_by, w->verified_guest.name, sizeof(g.thanked_by) - 1);

    FILE *f = fopen(GIFT_CSV, "a");
    if (!f) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Error: could not open gifts.csv");
        return;
    }
    write_gift_line(f, &g);
    fclose(f);

    char msg[600];
    if (use_momo) {
        /* Build Orange Money or MTN MoMo web payment URL */
        char pay_url[512];
        int is_orange = (strncasecmp(MOMO_OPERATOR, "Orange", 6) == 0);
        if (is_orange) {
            /* Orange Money Cameroon payment page */
            snprintf(pay_url, sizeof(pay_url),
                     "https://www.orangemoney.com/cm/pay?to=%s&amount=%.0f",
                     MOMO_PHONE, amount);
        } else {
            /* MTN MoMo payment page */
            snprintf(pay_url, sizeof(pay_url),
                     "https://momo.mtn.com/payment?phone=%s&amount=%.0f",
                     MOMO_PHONE, amount);
        }

        snprintf(msg, sizeof(msg),
                 "✔ Contribution recorded!  Record ID: %d\n"
                 "Guest  : %s\n"
                 "Amount : %.0f FCFA  (%.2f EUR)\n"
                 "Method : %s %s\n\n"
                 "📱 Opening %s Mobile Money payment page now...\n"
                 "Send %.0f FCFA to: %s\n\n"
                 "If the page does not open automatically, send the payment manually "
                 "to %s at %s.",
                 g.record_id, g.guest_name,
                 g.total_fcfa, g.total_eur,
                 MOMO_OPERATOR, method,
                 MOMO_OPERATOR,
                 amount, MOMO_PHONE,
                 MOMO_OPERATOR, MOMO_PHONE);
        gtk_label_set_text(GTK_LABEL(w->lbl_result), msg);

        /* Open MoMo payment URL in the browser */
        g_app_info_launch_default_for_uri(pay_url, NULL, NULL);

    } else {
        /* Credit card / bank transfer: display details and open bank portal */
        snprintf(msg, sizeof(msg),
                 "✔ Contribution recorded!  Record ID: %d\n"
                 "Guest  : %s\n"
                 "Amount : %.0f FCFA  (%.2f EUR)\n"
                 "Method : %s\n\n"
                 "🏦 Bank Transfer Details:\n"
                 "  Bank            : %s\n"
                 "  Account Number  : %s\n"
                 "  Account Name    : %s\n"
                 "  Amount to send  : %.0f FCFA\n\n"
                 "Please use your bank's online portal or visit a branch to complete "
                 "the transfer. Use your name as the payment reference.",
                 g.record_id, g.guest_name,
                 g.total_fcfa, g.total_eur,
                 method,
                 BANK_NAME, BANK_ACCOUNT, BANK_ACCOUNT_NAME,
                 amount);
        gtk_label_set_text(GTK_LABEL(w->lbl_result), msg);

        /* Open bank's general web portal - a real URL for common Cameroonian banks */
        const char *bank_url = "https://www.afrilandfirstbank.com/services/transfert";
        if (strstr(BANK_NAME, "UBA") || strstr(BANK_NAME, "uba"))
            bank_url = "https://www.ubagroup.com/cameroon/internet-banking/";
        else if (strstr(BANK_NAME, "SCB") || strstr(BANK_NAME, "scb"))
            bank_url = "https://www.scbcameroun.com/services";
        else if (strstr(BANK_NAME, "Ecobank") || strstr(BANK_NAME, "ecobank"))
            bank_url = "https://ecobank.com/cm/personal-banking/transact/online-banking";
        g_app_info_launch_default_for_uri(bank_url, NULL, NULL);
    }

    /* Reset form for the next contribution */
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),   "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_amount), "");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w->radio_momo), TRUE);
    w->guest_ok = 0;
    gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), "");

    gtk_widget_set_visible(w->btn_return_home, TRUE);
}

/*
 * build_financial_contribution_tab
 * ---------------------------------
 * Construct the "💰 Financial Contribution" tab widget.
 *
 * Layout:
 *   Instructional note
 *   ── separator ──
 *   Step 1: Identity verification (same pattern as other tabs)
 *   ── separator ──
 *   Step 2: Amount (FCFA) entry + payment method radio buttons
 *   ── separator ──
 *   [Submit Financial Contribution] button
 *   Result label
 *   [Return to Panel Selection] (hidden until after successful submit)
 */
static GtkWidget *build_financial_contribution_tab(FinContribWidgets *w) {
    memset(w, 0, sizeof(*w));

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    GtkWidget *note = gtk_label_new(
        "Prefer to contribute financially instead of a physical gift? "
        "Fill in your details below.");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(note), TRUE);
    gtk_box_append(GTK_BOX(box), note);
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Step 1 : identity ---- */
    SECTION_LBL(box, "Step 1 : Verify your identity (Full Name)");

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
    g_signal_connect(bv, "clicked", G_CALLBACK(on_fin_verify), w);
    gtk_grid_attach(GTK_GRID(grid), bv, 1, 1, 1, 1);

    w->lbl_guest_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_guest_status), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(w->lbl_guest_status), TRUE);
    gtk_box_append(GTK_BOX(box), w->lbl_guest_status);
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Step 2 : contribution details ---- */
    SECTION_LBL(box, "Step 2 : Contribution Details");

    GtkWidget *det_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(det_grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(det_grid), 10);
    gtk_box_append(GTK_BOX(box), det_grid);

    /* Amount */
    gtk_grid_attach(GTK_GRID(det_grid), gtk_label_new("Amount (FCFA):"), 0, 0, 1, 1);
    w->entry_amount = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_amount),
                                   "e.g. 25000");
    gtk_widget_set_hexpand(w->entry_amount, TRUE);
    gtk_grid_attach(GTK_GRID(det_grid), w->entry_amount, 1, 0, 1, 1);

    /* Payment method label */
    gtk_grid_attach(GTK_GRID(det_grid),
                    gtk_label_new("Payment Method:"), 0, 1, 1, 1);

    /* Radio buttons in a horizontal box (mutually exclusive via set_group) */
    GtkWidget *radio_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_grid_attach(GTK_GRID(det_grid), radio_box, 1, 1, 1, 1);

    w->radio_momo = gtk_check_button_new_with_label("📱  Mobile Money");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w->radio_momo), TRUE);
    gtk_box_append(GTK_BOX(radio_box), w->radio_momo);

    w->radio_card = gtk_check_button_new_with_label("💳  Credit Card");
    /* Link them so only one can be active at a time */
    gtk_check_button_set_group(GTK_CHECK_BUTTON(w->radio_card),
                               GTK_CHECK_BUTTON(w->radio_momo));
    gtk_box_append(GTK_BOX(radio_box), w->radio_card);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Submit button */
    GtkWidget *btn_submit = gtk_button_new_with_label(
        "💰  Submit Financial Contribution");
    g_signal_connect(btn_submit, "clicked", G_CALLBACK(on_fin_submit), w);
    gtk_box_append(GTK_BOX(box), btn_submit);

    /* Result label */
    w->lbl_result = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(w->lbl_result), TRUE);
    gtk_label_set_xalign(GTK_LABEL(w->lbl_result), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_result);

    /* Return home button (hidden until after a successful submission) */
    w->btn_return_home = gtk_button_new_with_label("⌂  Return to Panel Selection");
    gtk_widget_set_visible(w->btn_return_home, FALSE);
    gtk_widget_set_halign(w->btn_return_home, GTK_ALIGN_START);
    gtk_widget_set_margin_top(w->btn_return_home, 6);
    g_signal_connect(w->btn_return_home, "clicked",
                     G_CALLBACK(on_back_home_guest), NULL);
    gtk_box_append(GTK_BOX(box), w->btn_return_home);

    return box;
}

/* =========================================================
 * Admin Panel – Display Gifts tab
 * =========================================================
 *
 * Shows all gift records in a password-protected read-only table.
 * Password is re-checked each time "Load / Refresh" is clicked, so the
 * tab can be refreshed after new gifts are registered without re-logging-in.
 */

/*
 * DisplayWidgets – widget state for the "Display Gifts" admin tab.
 *   pw_ok – set to 1 after the password is accepted; used by UpdateWidgets
 *           to auto-refresh the display after an update.
 */
typedef struct {
    GtkWidget *entry_pw;
    GtkWidget *lbl_pw_status;
    GtkWidget *list_view;
    int        pw_ok;
    GtkWidget *btn_return_home;   /* always visible; lets admin go back to selection */
} DisplayWidgets;

/*
 * do_refresh
 * ----------
 * Read gifts.csv and render its contents into the GtkTextView as a formatted
 * table.  Called by on_disp_load() and also from on_upd_submit() whenever
 * the display tab has already been unlocked (pw_ok == 1).
 */
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

    /* Table header */
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

/* on_disp_load – "Load / Refresh Gift List" button callback.
 * Checks the password; on success sets pw_ok = 1 and calls do_refresh(). */
static void on_disp_load(GtkButton *b, gpointer d) {
    (void)b;
    DisplayWidgets *w = (DisplayWidgets *)d;
    if (check_pw_entry(w->entry_pw, w->lbl_pw_status)) {
        w->pw_ok = 1;
        do_refresh(w);
    }
}

/*
 * build_display_tab
 * -----------------
 * Construct the "Display Gifts" admin tab widget.
 *
 * Layout:
 *   Password entry row + [Load / Refresh Gift List] button
 *   Password status label
 *   Scrolled GtkTextView (monospace gift table)
 *   [Return to Panel Selection] button (always visible)
 */
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
    gtk_entry_set_visibility(GTK_ENTRY(w->entry_pw), FALSE);   /* masked input */
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->entry_pw), "Password");
    gtk_widget_set_hexpand(w->entry_pw, TRUE);
    gtk_box_append(GTK_BOX(pw_row), w->entry_pw);
    GtkWidget *btn = gtk_button_new_with_label("Load / Refresh Gift List");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_disp_load), w);
    gtk_box_append(GTK_BOX(pw_row), btn);

    w->lbl_pw_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_pw_status), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_pw_status);

    /* Read-only, monospace, scrollable text view for the gift table */
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

    /* Always-visible return button so admin can leave this tab at any time */
    w->btn_return_home = gtk_button_new_with_label("⌂  Return to Panel Selection");
    gtk_widget_set_halign(w->btn_return_home, GTK_ALIGN_START);
    gtk_widget_set_margin_top(w->btn_return_home, 6);
    g_signal_connect(w->btn_return_home, "clicked",
                     G_CALLBACK(on_back_home_admin), NULL);
    gtk_box_append(GTK_BOX(box), w->btn_return_home);

    return box;
}

/* =========================================================
 * Admin Panel – Delete Gift tab
 * =========================================================
 *
 * Requires both the admin password and the guest's own identity verification
 * before a record can be deleted.  This prevents accidental mass deletion and
 * ensures the record belongs to the claiming guest.
 *
 * Deletion is implemented as a temp-file rewrite:
 *   1. gifts.csv is read line by line into gifts_tmp.csv, omitting the target.
 *   2. gifts.csv is removed.
 *   3. gifts_tmp.csv is renamed to gifts.csv.
 */

/* DeleteWidgets – widget state for the "Delete Gift" admin tab */
typedef struct {
    GtkWidget  *entry_pw;
    GtkWidget  *lbl_pw_status;
    int         pw_ok;

    GtkWidget  *entry_name;
    GtkWidget  *lbl_guest_status;
    GuestBrief  verified_guest;
    int         guest_ok;
    GtkWidget  *entry_rec_id;
    GtkWidget  *lbl_result;
    GtkWidget  *btn_return_home;   /* shown after a successful delete */
} DeleteWidgets;

/* on_del_pw_confirm – "Confirm Password" callback; sets w->pw_ok on success */
static void on_del_pw_confirm(GtkButton *b, gpointer d) {
    (void)b;
    DeleteWidgets *w = (DeleteWidgets *)d;
    w->pw_ok = check_pw_entry(w->entry_pw, w->lbl_pw_status);
}

/* on_del_verify – "Verify Identity" callback for the delete tab.
 * Guards against deleting without both password and identity. */
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

/*
 * on_del_submit  ("Delete Gift Record" button)
 * -------------
 * Validates pw_ok, guest_ok, and a positive record_id, then rewrites
 * gifts.csv omitting the matching row.  The record must belong to the
 * verified guest (both record_id and guest_id must match).
 *
 * After the rewrite the form is reset and btn_return_home is shown.
 */
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

    /* Copy all rows except the one to delete */
    char line[640]; GiftRecord gr; int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!parse_gift_line(line, &gr)) continue;
        /* Both record_id and guest_id must match (ownership check) */
        if (gr.record_id == record_id
                && gr.guest_id == w->verified_guest.id)
            found = 1;
        else
            write_gift_line(tmp, &gr);
    }
    fclose(f); fclose(tmp);
    /* Atomic replace */
    remove(GIFT_CSV); rename(TMP_GIFT, GIFT_CSV);

    char msg[80];
    if (found)
        snprintf(msg, sizeof(msg),
                 "Record %d deleted successfully.", record_id);
    else
        snprintf(msg, sizeof(msg),
                 "Record not found for your account.");
    gtk_label_set_text(GTK_LABEL(w->lbl_result), msg);

    /* Reset auth state and form fields */
    w->pw_ok = w->guest_ok = 0;
    gtk_label_set_text(GTK_LABEL(w->lbl_pw_status),    "");
    gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_pw),     "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),   "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_rec_id), "");

    gtk_widget_set_visible(w->btn_return_home, TRUE);
}

/*
 * build_delete_tab
 * ----------------
 * Construct the "Delete Gift" admin tab widget.
 *
 * Layout:
 *   Step 1: Password confirmation
 *   ── separator ──
 *   Step 2: Identity verification (Full Name only)
 *   ── separator ──
 *   Step 3: Record ID entry + [Delete Gift Record] button
 *   Result label
 *   [Return to Panel Selection] (hidden until after a successful delete)
 */
static GtkWidget *build_delete_tab(DeleteWidgets *w) {
    memset(w, 0, sizeof(*w));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

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

    w->btn_return_home = gtk_button_new_with_label("⌂  Return to Panel Selection");
    gtk_widget_set_visible(w->btn_return_home, FALSE);
    gtk_widget_set_halign(w->btn_return_home, GTK_ALIGN_START);
    gtk_widget_set_margin_top(w->btn_return_home, 6);
    g_signal_connect(w->btn_return_home, "clicked",
                     G_CALLBACK(on_back_home_admin), NULL);
    gtk_box_append(GTK_BOX(box), w->btn_return_home);

    return box;
}

/* =========================================================
 * Admin Panel – Update Gift tab
 * =========================================================
 *
 * Allows an admin (with password) to change the gift item, quantity, and
 * thank-you name of any gift record owned by the verified guest.
 *
 * After a successful update, do_refresh() is called on the Display tab
 * (if it is already unlocked) so the table reflects the change immediately.
 * The rewrite mechanism is identical to the delete tab.
 */

/* UpdateWidgets – widget state for the "Update Gift" admin tab */
typedef struct {
    GtkWidget  *entry_pw;
    GtkWidget  *lbl_pw_status;
    int         pw_ok;

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
    GtkWidget  *btn_return_home;   /* shown after a successful update */
} UpdateWidgets;

/* on_upd_pw_confirm – "Confirm Password" callback for the update tab */
static void on_upd_pw_confirm(GtkButton *b, gpointer d) {
    (void)b;
    UpdateWidgets *w = (UpdateWidgets *)d;
    w->pw_ok = check_pw_entry(w->entry_pw, w->lbl_pw_status);
}

/* on_upd_verify – "Verify Identity" callback; guards against update without auth */
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

/* on_upd_item_changed – update price labels when the catalogue selection changes */
static void on_upd_item_changed(GObject *o, GParamSpec *ps, gpointer d) {
    (void)ps;
    UpdateWidgets *w = (UpdateWidgets *)d;
    update_price_labels(GTK_DROP_DOWN(o),
                        GTK_SPIN_BUTTON(w->spin_qty),
                        GTK_LABEL(w->lbl_unit_price),
                        GTK_LABEL(w->lbl_total));
}

/* on_upd_qty_changed – update price labels when the quantity spin changes */
static void on_upd_qty_changed(GtkSpinButton *s, gpointer d) {
    (void)s;
    UpdateWidgets *w = (UpdateWidgets *)d;
    update_price_labels(GTK_DROP_DOWN(w->combo_item),
                        GTK_SPIN_BUTTON(w->spin_qty),
                        GTK_LABEL(w->lbl_unit_price),
                        GTK_LABEL(w->lbl_total));
}

/*
 * on_upd_submit  ("Update Gift Record" button)
 * -------------
 * Rewrites gifts.csv, applying the new item / quantity / thank-you name to
 * the record identified by record_id and owned by verified_guest.
 *
 * If the display tab is already unlocked (disp_w_ptr != NULL && pw_ok),
 * do_refresh() is called to keep the table current.
 *
 * All input fields are cleared after a successful update and btn_return_home
 * is made visible.
 */
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
    if ((int)idx >= cat_count) {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Select a gift item."); return;
    }
    const GiftItem *chosen = &cat[idx];
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

    /* Rewrite gifts.csv, patching the target row in-place */
    char line[640]; GiftRecord gr; int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!parse_gift_line(line, &gr)) continue;
        if (gr.record_id == record_id
                && gr.guest_id == w->verified_guest.id) {
            /* Apply the new values */
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

        /* Auto-refresh the display tab if it is already unlocked */
        if (w->disp_w_ptr) {
            DisplayWidgets *dw = (DisplayWidgets *)w->disp_w_ptr;
            if (dw->pw_ok) do_refresh(dw);
        }
    } else {
        gtk_label_set_text(GTK_LABEL(w->lbl_result),
                           "Record not found for your account.");
    }
    /* Reset all auth state and input fields */
    w->pw_ok = w->guest_ok = 0;
    gtk_label_set_text(GTK_LABEL(w->lbl_pw_status),    "");
    gtk_label_set_text(GTK_LABEL(w->lbl_guest_status), "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_pw),      "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_name),    "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_rec_id),  "");
    gtk_editable_set_text(GTK_EDITABLE(w->entry_thanked), "");

    gtk_widget_set_visible(w->btn_return_home, TRUE);
}

/*
 * build_update_tab
 * ----------------
 * Construct the "Update Gift" admin tab widget.
 *
 * Layout:
 *   Step 1: Password confirmation
 *   ── separator ──
 *   Step 2: Identity verification + Record ID entry
 *   ── separator ──
 *   Step 3: New gift dropdown + quantity spin + price labels
 *           Thank-you name entry
 *   [Update Gift Record] button
 *   Result label
 *   [Return to Panel Selection] (hidden until after successful update)
 */
static GtkWidget *build_update_tab(UpdateWidgets *w) {
    memset(w, 0, sizeof(*w));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

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

    SECTION_LBL(box, "Step 3 : Select new gift and quantity (total auto-calculated)");
    /* Reuse build_catalogue_list() for a live-updated model */
    w->combo_item = gtk_drop_down_new(
        G_LIST_MODEL(build_catalogue_list()), NULL);
    gtk_widget_set_hexpand(w->combo_item, TRUE);
    gtk_box_append(GTK_BOX(box), w->combo_item);

    w->lbl_unit_price = gtk_label_new("Unit price: ");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_unit_price), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_unit_price);

    /* Quantity spin button (range 1–9999) */
    GtkWidget *qr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), qr);
    gtk_box_append(GTK_BOX(qr), gtk_label_new("Quantity:"));
    w->spin_qty = gtk_spin_button_new_with_range(1, 9999, 1);
    gtk_box_append(GTK_BOX(qr), w->spin_qty);

    w->lbl_total = gtk_label_new("Total: ");
    gtk_label_set_xalign(GTK_LABEL(w->lbl_total), 0.0f);
    gtk_box_append(GTK_BOX(box), w->lbl_total);

    /* Wire up item and quantity changes to keep price labels current */
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

    w->btn_return_home = gtk_button_new_with_label("⌂  Return to Panel Selection");
    gtk_widget_set_visible(w->btn_return_home, FALSE);
    gtk_widget_set_halign(w->btn_return_home, GTK_ALIGN_START);
    gtk_widget_set_margin_top(w->btn_return_home, 6);
    g_signal_connect(w->btn_return_home, "clicked",
                     G_CALLBACK(on_back_home_admin), NULL);
    gtk_box_append(GTK_BOX(box), w->btn_return_home);

    return box;
}

/*
 * Global widget instances (one per tab type; all built once at app startup).
 * Using globals avoids passing pointers through the GTK signal chain.
 */
static RegisterWidgets reg_w;
static GroupRegWidgets grp_w;   /* group gift mode */
/* fin_w is declared near its builder function above */
static DisplayWidgets  disp_w;
static DeleteWidgets   del_w;
static UpdateWidgets   upd_w;

/*
 * AppState – top-level navigation state.
 *   stack           – GtkStack holding all top-level pages
 *                     (named: "home", "guest", "admin_login", "admin")
 *   admin_pw_entry  – the password entry on the admin login page
 *   admin_pw_status – status label on the admin login page
 */
typedef struct {
    GtkWidget *stack;          /* GtkStack: "home", "guest", "admin_login", "admin" */
    GtkWidget *admin_pw_entry;
    GtkWidget *admin_pw_status;
} AppState;

static AppState app_state;

/*
 * show_page – switch the GtkStack to the named child page.
 * All navigation callbacks delegate to this one function.
 */
static void show_page(const char *name) {
    gtk_stack_set_visible_child_name(GTK_STACK(app_state.stack), name);
}

/* on_go_guest – "Guest Panel" button on the home page */
static void on_go_guest(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    show_page("guest");
}

/* on_go_admin_login – "Admin Panel" button on the home page.
 * Clears the password entry and status so the login screen is clean. */
static void on_go_admin_login(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    gtk_editable_set_text(GTK_EDITABLE(app_state.admin_pw_entry), "");
    gtk_label_set_text(GTK_LABEL(app_state.admin_pw_status), "");
    show_page("admin_login");
}

/* on_admin_login_submit – "Login" button on the admin login page.
 * Checks the password and navigates to "admin" on success, or shows an
 * error and clears the entry on failure. */
static void on_admin_login_submit(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    const char *pw = gtk_editable_get_text(
        GTK_EDITABLE(app_state.admin_pw_entry));
    if (strcmp(pw, PASSWORD) == 0) {
        gtk_label_set_text(GTK_LABEL(app_state.admin_pw_status), "");
        show_page("admin");
    } else {
        gtk_label_set_text(GTK_LABEL(app_state.admin_pw_status),
                           "Wrong password. Please try again.");
        gtk_editable_set_text(GTK_EDITABLE(app_state.admin_pw_entry), "");
    }
}

/* Navigation callbacks – all simply call show_page() */
static void on_back_home_guest(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    show_page("home");
}

static void on_back_home_admin(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    show_page("home");
}

static void on_back_login(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    show_page("home");
}

/*
 * build_home_page
 * ---------------
 * Construct the home/landing page widget.
 *
 * The page is centred both vertically and horizontally via GTK_ALIGN_CENTER
 * on the outer box.  Two side-by-side panels (Guest / Admin) are presented.
 *
 * Layout:
 *   "Wedding Gift Manager"  title (xx-large bold)
 *   "group3wed"             subtitle
 *   ── separator ──
 *   "Please select your access level:"
 *   [🎁 Guest Panel]    [🔧 Admin Panel]
 *   description under each button
 */
static GtkWidget *build_home_page(void) {
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(outer, TRUE);
    gtk_widget_set_hexpand(outer, TRUE);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 32);
    gtk_widget_set_margin_start(vbox, 60);
    gtk_widget_set_margin_end(vbox, 60);
    gtk_widget_set_margin_top(vbox, 60);
    gtk_widget_set_margin_bottom(vbox, 60);
    gtk_box_append(GTK_BOX(outer), vbox);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='xx-large' weight='bold'>Wedding Gift Manager</span>\n"
        "<span size='large' foreground='gray'>group3wed</span>");
    gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(vbox), title);

    gtk_box_append(GTK_BOX(vbox),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *subtitle = gtk_label_new("Please select your access level:");
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.5f);
    gtk_box_append(GTK_BOX(vbox), subtitle);

    /* Horizontal row holding both panel buttons */
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 40);
    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), hbox);

    /* --- Guest panel card --- */
    GtkWidget *guest_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(guest_vbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(hbox), guest_vbox);

    GtkWidget *guest_icon = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(guest_icon),
        "<span size='xx-large'></span>");
    gtk_label_set_xalign(GTK_LABEL(guest_icon), 0.5f);
    gtk_box_append(GTK_BOX(guest_vbox), guest_icon);

    GtkWidget *guest_btn = gtk_button_new_with_label("  Guest Panel  ");
    gtk_widget_set_size_request(guest_btn, 180, 60);
    g_signal_connect(guest_btn, "clicked", G_CALLBACK(on_go_guest), NULL);
    gtk_box_append(GTK_BOX(guest_vbox), guest_btn);

    GtkWidget *guest_desc = gtk_label_new("Offer gifts & go to shop\n(no password needed)");
    gtk_label_set_justify(GTK_LABEL(guest_desc), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(guest_vbox), guest_desc);

    /* --- Admin panel card --- */
    GtkWidget *admin_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(admin_vbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(hbox), admin_vbox);

    GtkWidget *admin_icon = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(admin_icon),
        "<span size='xx-large'></span>");
    gtk_label_set_xalign(GTK_LABEL(admin_icon), 0.5f);
    gtk_box_append(GTK_BOX(admin_vbox), admin_icon);

    GtkWidget *admin_btn = gtk_button_new_with_label("  Admin Panel  ");
    gtk_widget_set_size_request(admin_btn, 180, 60);
    g_signal_connect(admin_btn, "clicked", G_CALLBACK(on_go_admin_login), NULL);
    gtk_box_append(GTK_BOX(admin_vbox), admin_btn);

    GtkWidget *admin_desc = gtk_label_new("Display / Delete / Update gifts\n(password required)");
    gtk_label_set_justify(GTK_LABEL(admin_desc), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(admin_vbox), admin_desc);

    return outer;
}

/* Forward declaration – build_guest_fabric_tab is defined after build_guest_page */
static GtkWidget *build_guest_fabric_tab(void);

/*
 * build_guest_page
 * ----------------
 * Construct the full "Guest Panel" page widget.
 *
 * The page uses a GtkNotebook with 4 tabs:
 *   1. "One Guest → Gift(s)"              – single-guest registration
 *   2. "Group Gift (Many Guests → One Gift)" – group registration
 *   3. "💰 Financial Contribution"        – cash/MoMo contribution
 *   4. "🧵 Wedding Fabric"               – fabric purchase interest
 *
 * Each tab's content is wrapped in a GtkScrolledWindow so it remains
 * usable on small screens or when the window is resized.
 */
static GtkWidget *build_guest_page(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Top bar: [← Back] title */
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(top_bar, 12);
    gtk_widget_set_margin_end(top_bar, 12);
    gtk_widget_set_margin_top(top_bar, 8);
    gtk_widget_set_margin_bottom(top_bar, 8);
    gtk_box_append(GTK_BOX(vbox), top_bar);

    GtkWidget *back = gtk_button_new_with_label("← Back");
    g_signal_connect(back, "clicked", G_CALLBACK(on_back_home_guest), NULL);
    gtk_box_append(GTK_BOX(top_bar), back);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
                         "<b>Guest Panel</b>  –  Register a Gift");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_widget_set_hexpand(title, TRUE);
    gtk_box_append(GTK_BOX(top_bar), title);

    gtk_box_append(GTK_BOX(vbox),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Notebook with 4 tabs */
    GtkWidget *nb = gtk_notebook_new();
    gtk_widget_set_vexpand(nb, TRUE);
    gtk_box_append(GTK_BOX(vbox), nb);

    /* Tab 1: single-guest gift registration */
    GtkWidget *scroll_a = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll_a, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_a),
                                  build_register_tab(&reg_w));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), scroll_a,
                             gtk_label_new("One Guest → Gift(s)"));

    /* Tab 2: group gift registration */
    GtkWidget *scroll_b = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll_b, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_b),
                                  build_group_register_tab(&grp_w));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), scroll_b,
                             gtk_label_new("Group Gift (Many Guests → One Gift)"));

    /* Tab 3: financial contribution */
    GtkWidget *scroll_c = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll_c, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_c),
                                  build_financial_contribution_tab(&fin_w));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), scroll_c,
                             gtk_label_new("💰 Financial Contribution"));

    /* Tab 4: wedding fabric (defined later; forward-declared above) */
    GtkWidget *scroll_d = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll_d, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_d),
                                  build_guest_fabric_tab());
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), scroll_d,
                             gtk_label_new("🧵 Wedding Fabric"));

    return vbox;
}

/*
 * build_admin_login_page
 * ----------------------
 * Construct the admin password prompt page.
 *
 * A centred card contains a title, description, the password entry, a status
 * label, and [← Back] / [Login] buttons.
 *
 * Pressing Enter in the password entry activates the Login button via
 * g_signal_connect_swapped(entry, "activate", gtk_widget_activate, login_btn).
 */
static GtkWidget *build_admin_login_page(void) {
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(outer, TRUE);
    gtk_widget_set_hexpand(outer, TRUE);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(vbox, 60);
    gtk_widget_set_margin_end(vbox, 60);
    gtk_widget_set_size_request(vbox, 380, -1);
    gtk_box_append(GTK_BOX(outer), vbox);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='x-large' weight='bold'>Admin Panel Login</span>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.5f);
    gtk_box_append(GTK_BOX(vbox), title);

    GtkWidget *desc = gtk_label_new("Enter the admin password to continue:");
    gtk_label_set_xalign(GTK_LABEL(desc), 0.5f);
    gtk_box_append(GTK_BOX(vbox), desc);

    app_state.admin_pw_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app_state.admin_pw_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(app_state.admin_pw_entry),
                                   "Password");
    gtk_widget_set_hexpand(app_state.admin_pw_entry, TRUE);
    gtk_box_append(GTK_BOX(vbox), app_state.admin_pw_entry);

    app_state.admin_pw_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(app_state.admin_pw_status), 0.5f);
    gtk_box_append(GTK_BOX(vbox), app_state.admin_pw_status);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), btn_row);

    GtkWidget *back = gtk_button_new_with_label("← Back");
    g_signal_connect(back, "clicked", G_CALLBACK(on_back_login), NULL);
    gtk_box_append(GTK_BOX(btn_row), back);

    GtkWidget *login_btn = gtk_button_new_with_label("Login");
    g_signal_connect(login_btn, "clicked",
                     G_CALLBACK(on_admin_login_submit), NULL);
    gtk_box_append(GTK_BOX(btn_row), login_btn);

    /* Allow pressing Enter in the password field to trigger Login */
    g_signal_connect_swapped(app_state.admin_pw_entry, "activate",
                             G_CALLBACK(gtk_widget_activate),
                             login_btn);

    return outer;
}

/* =========================================================
 * Admin Panel – Manage Catalogue tab
 * =========================================================
 *
 * Displays the current catalogue in a read-only GtkTextView and provides
 * a form to add new items at runtime.  New items are pushed to all live
 * GtkStringList models via add_catalogue_item() so every dropdown in the
 * UI updates without a restart.
 */

/* CatWidgets – widget state for the "Manage Catalogue" admin tab */
typedef struct {
    GtkWidget *entry_name;
    GtkWidget *entry_price;
    GtkWidget *lbl_status;
    GtkWidget *list_view;   /* GtkTextView showing current catalogue */
} CatWidgets;

static CatWidgets cat_w;

/*
 * cat_refresh_list
 * ----------------
 * Rebuild the catalogue text view from the current in-memory cat[] array.
 * Called once when the tab is first shown and after every successful add.
 */
static void cat_refresh_list(void) {
    GtkTextBuffer *buf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(cat_w.list_view));
    GString *sb = g_string_new(NULL);
    g_string_append(sb,
        " ID  | Gift Item                              |   Price (FCFA)  |  Price (EUR)\n"
        "-----|----------------------------------------|-----------------|-------------\n");
    for (int i = 0; i < cat_count; i++) {
        g_string_append_printf(sb,
            " %-3d | %-38s | %15.0f | %12.2f\n",
            cat[i].id, cat[i].name,
            cat[i].price_fcfa,
            cat[i].price_fcfa / EUR_RATE);
    }
    g_string_append_printf(sb, "\nTotal items: %d\n", cat_count);
    gtk_text_buffer_set_text(buf, sb->str, -1);
    g_string_free(sb, TRUE);
}

/*
 * on_cat_add  ("＋ Add Gift to Catalogue" button)
 * ----------
 * Reads the name and price entries, validates them, calls add_catalogue_item()
 * (which also pushes the new row to all live models), and refreshes the
 * catalogue text view.  Clears the input fields on success.
 */
static void on_cat_add(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    const char *nm = gtk_editable_get_text(GTK_EDITABLE(cat_w.entry_name));
    const char *pr = gtk_editable_get_text(GTK_EDITABLE(cat_w.entry_price));
    if (!nm || strlen(nm) < 1) {
        gtk_label_set_text(GTK_LABEL(cat_w.lbl_status),
                           "Please enter a gift name.");
        return;
    }
    char *end;
    double price = strtod(pr, &end);
    if (end == pr || price <= 0.0) {
        gtk_label_set_text(GTK_LABEL(cat_w.lbl_status),
                           "Please enter a valid price in FCFA (positive number).");
        return;
    }
    if (add_catalogue_item(nm, price) < 0) {
        gtk_label_set_text(GTK_LABEL(cat_w.lbl_status),
                           "Catalogue is full (200 items max).");
        return;
    }
    /* Compose confirmation message using the newly-added entry */
    char msg[200];
    snprintf(msg, sizeof(msg),
             "Added: \"%s\"  –  %.0f FCFA (%.2f EUR)  [ID %d]",
             cat[cat_count - 1].name,
             cat[cat_count - 1].price_fcfa,
             cat[cat_count - 1].price_fcfa / EUR_RATE,
             cat[cat_count - 1].id);
    gtk_label_set_text(GTK_LABEL(cat_w.lbl_status), msg);
    gtk_editable_set_text(GTK_EDITABLE(cat_w.entry_name),  "");
    gtk_editable_set_text(GTK_EDITABLE(cat_w.entry_price), "");
    cat_refresh_list();
}

/* =========================================================
 * Wedding Fabrics – Admin tab
 * =========================================================
 *
 * Lets the admin:
 *   1. Select one of 6 fabric designs (radio buttons + image previews).
 *   2. Toggle the fabric offer on/off (checkbox).
 *   3. Configure Mobile Money and bank payment details.
 *   4. Save all settings to fabric.csv (fabric_save()).
 *
 * Saving also calls fabric_refresh_guest_panel() so the guest tab
 * reflects the change immediately without a restart.
 */

/*
 * fab_img_path
 * ------------
 * Build the absolute path for the image file of fabric design idx.
 * Derives the directory from FABRIC_CSV (which is set to the binary's
 * directory by init_csv_paths).  Falls back to the bare filename if the
 * path cannot be determined.
 */
static void fab_img_path(int idx, char *buf, int bufsz) {
    if (FABRIC_CSV[0]) {
        char dir[512] = {0};
        strncpy(dir, FABRIC_CSV, sizeof(dir) - 1);
        /* Strip "fabric.csv" to get the directory */
        char *sl = strrchr(dir, '/');
        if (!sl) sl = strrchr(dir, '\\');
        if (sl) {
            *sl = '\0';
            snprintf(buf, bufsz, "%s/%s", dir, FABRIC_IMG_FILES[idx]);
            return;
        }
    }
    strncpy(buf, FABRIC_IMG_FILES[idx], bufsz - 1);
    buf[bufsz - 1] = '\0';
}

/*
 * fab_load_image
 * --------------
 * Set a GtkPicture widget to display the fabric image at index idx.
 * Uses g_file_new_for_path() → gtk_picture_set_file().
 * The GFile ref is released immediately after; GTK holds its own reference.
 */
static void fab_load_image(GtkWidget *img_widget, int idx) {
    char img_path[600];
    fab_img_path(idx, img_path, sizeof(img_path));
    GFile *file = g_file_new_for_path(img_path);
    gtk_picture_set_file(GTK_PICTURE(img_widget), file);
    g_object_unref(file);
}

/*
 * on_fab_radio_toggled
 * --------------------
 * "toggled" signal callback for each fabric radio button.
 * When a radio is activated, sets its image to full opacity (1.0) and
 * dims all others (0.35) for a simple visual selection indicator.
 */
static void on_fab_radio_toggled(GtkCheckButton *btn, gpointer d) {
    (void)d;
    /* Find which radio was just activated and refresh its image */
    for (int i = 0; i < FABRIC_COUNT; i++) {
        if ((GtkWidget *)btn == fab_w.radio[i] &&
            gtk_check_button_get_active(btn)) {
            /* highlight selected image with opacity; dim others */
            for (int j = 0; j < FABRIC_COUNT; j++)
                gtk_widget_set_opacity(fab_w.img[j], j == i ? 1.0 : 0.35);
            break;
        }
    }
}

/*
 * on_fab_admin_save  ("💾 Save Settings & Activate" button)
 * -----------------
 * Reads all admin fabric settings from the widgets, updates the global
 * variables, calls fabric_save() to persist them, and refreshes both the
 * admin info label and the guest fabric panel.
 */
static void on_fab_admin_save(GtkButton *b, gpointer d) {
    (void)b; (void)d;

    /* Determine which fabric radio is active */
    for (int i = 0; i < FABRIC_COUNT; i++) {
        if (gtk_check_button_get_active(GTK_CHECK_BUTTON(fab_w.radio[i]))) {
            fabric_cfg.fabric_index = i;
            break;
        }
    }

    /* Active toggle */
    fabric_cfg.active =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(fab_w.chk_active)) ? 1 : 0;

    /* Mobile Money config – only update if the entry is non-empty */
    const char *op = gtk_editable_get_text(GTK_EDITABLE(fab_w.entry_momo_operator));
    const char *ph = gtk_editable_get_text(GTK_EDITABLE(fab_w.entry_momo_phone));
    if (op && strlen(op) > 0)
        strncpy(MOMO_OPERATOR, op, sizeof(MOMO_OPERATOR) - 1);
    if (ph && strlen(ph) > 0)
        strncpy(MOMO_PHONE,    ph, sizeof(MOMO_PHONE)    - 1);

    /* Bank config – only update non-empty fields */
    const char *bn  = gtk_editable_get_text(GTK_EDITABLE(fab_w.entry_bank_name));
    const char *bac = gtk_editable_get_text(GTK_EDITABLE(fab_w.entry_bank_account));
    const char *ban = gtk_editable_get_text(GTK_EDITABLE(fab_w.entry_bank_account_name));
    if (bn  && strlen(bn)  > 0) strncpy(BANK_NAME,         bn,  sizeof(BANK_NAME)         - 1);
    if (bac && strlen(bac) > 0) strncpy(BANK_ACCOUNT,      bac, sizeof(BANK_ACCOUNT)      - 1);
    if (ban && strlen(ban) > 0) strncpy(BANK_ACCOUNT_NAME, ban, sizeof(BANK_ACCOUNT_NAME) - 1);

    /* Persist all settings and push changes to both panels */
    fabric_save();
    fabric_refresh_admin_info();
    fabric_refresh_guest_panel();   /* push change to guest side immediately */

    /* Refresh guest image for newly selected fabric */
    if (fab_w.guest_img)
        fab_load_image(fab_w.guest_img, fabric_cfg.fabric_index);

    /* Compose confirmation message for the admin status label */
    char msg[300];
    snprintf(msg, sizeof(msg),
             "✔ Saved! Fabric: \"%s\"  |  Status: %s\n"
             "MoMo: %s %s  |  Bank: %s  Acct: %s",
             FABRIC_NAMES[fabric_cfg.fabric_index],
             fabric_cfg.active ? "ACTIVE" : "INACTIVE",
             MOMO_OPERATOR, MOMO_PHONE,
             BANK_NAME, BANK_ACCOUNT);
    gtk_label_set_text(GTK_LABEL(fab_w.lbl_status), msg);
}

/*
 * build_admin_fabric_tab
 * ----------------------
 * Construct the "🧵 Wedding Fabrics" admin tab widget.
 *
 * Layout:
 *   Instructional note
 *   ── separator ──
 *   Fabric Design section: 3×2 image grid with radio buttons below each image
 *   ── separator ──
 *   Activation checkbox
 *   ── separator ──
 *   Payment Configuration grid (MoMo operator/phone, bank name/account/holder)
 *   [💾 Save Settings & Activate] button
 *   ── separator ──
 *   Current Status label (lbl_sold_info)
 *   Status/confirmation label (lbl_status)
 */
static GtkWidget *build_admin_fabric_tab(void) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    SECTION_LBL(box, "🧵 Select the Wedding Fabric for City Council");

    GtkWidget *note = gtk_label_new(
        "Choose one of the 6 fabric designs below (images load from files named "
        "fabric_dutch_spiral.jpg … fabric_glitter_lace.jpg placed next to the program). "
        "Activate to make it visible to guests — they will be directed to the online shop to buy.");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(note), TRUE);
    gtk_box_append(GTK_BOX(box), note);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Fabric image grid with radio buttons ---- */
    SECTION_LBL(box, "Fabric Design  (select one)");

    /* Use a flow box for responsive image layout: 3 columns × 2 rows */
    GtkWidget *fab_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(fab_grid), 18);
    gtk_grid_set_row_spacing(GTK_GRID(fab_grid), 18);
    gtk_box_append(GTK_BOX(box), fab_grid);

    for (int i = 0; i < FABRIC_COUNT; i++) {
        /* Card container per fabric: image above, radio button below */
        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_halign(card, GTK_ALIGN_CENTER);

        /* Fabric image – GtkPicture scales to fill the allocated size */
        char _ipath[600];
        fab_img_path(i, _ipath, sizeof(_ipath));
        fab_w.img[i] = gtk_picture_new_for_filename(_ipath);
        gtk_picture_set_content_fit(GTK_PICTURE(fab_w.img[i]), GTK_CONTENT_FIT_COVER);
        gtk_widget_set_size_request(fab_w.img[i], 300, 300);
        gtk_widget_set_hexpand(fab_w.img[i], FALSE);
        gtk_widget_set_vexpand(fab_w.img[i], FALSE);
        gtk_box_append(GTK_BOX(card), fab_w.img[i]);

        /* First radio is the group anchor; others join its group */
        if (i == 0) {
            fab_w.radio[0] = gtk_check_button_new_with_label(FABRIC_NAMES[0]);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(fab_w.radio[0]),
                                        fabric_cfg.fabric_index == 0);
        } else {
            fab_w.radio[i] = gtk_check_button_new_with_label(FABRIC_NAMES[i]);
            gtk_check_button_set_group(GTK_CHECK_BUTTON(fab_w.radio[i]),
                                       GTK_CHECK_BUTTON(fab_w.radio[0]));
            gtk_check_button_set_active(GTK_CHECK_BUTTON(fab_w.radio[i]),
                                        fabric_cfg.fabric_index == i);
        }
        g_signal_connect(fab_w.radio[i], "toggled",
                         G_CALLBACK(on_fab_radio_toggled), NULL);
        gtk_box_append(GTK_BOX(card), fab_w.radio[i]);

        /* Dim non-selected images on first render */
        gtk_widget_set_opacity(fab_w.img[i],
                               fabric_cfg.fabric_index == i ? 1.0 : 0.35);

        /* Place in a 3-column grid: column = i%3, row = i/3 */
        gtk_grid_attach(GTK_GRID(fab_grid), card,
                        i % 3, i / 3, 1, 1);
    }

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Activation toggle ---- */
    SECTION_LBL(box, "Activation");

    GtkWidget *act_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(act_grid), 14);
    gtk_grid_set_row_spacing(GTK_GRID(act_grid), 8);
    gtk_box_append(GTK_BOX(box), act_grid);

    fab_w.chk_active = gtk_check_button_new_with_label(
        "Make this fabric offer visible to guests (directs them to online shop)");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(fab_w.chk_active),
                                fabric_cfg.active);
    gtk_grid_attach(GTK_GRID(act_grid), fab_w.chk_active, 0, 0, 2, 1);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Payment Configuration ---- */
    SECTION_LBL(box, "💳 Payment Configuration  (for Financial Contributions)");

    GtkWidget *pay_note = gtk_label_new(
        "Set the Mobile Money account and bank account below. "
        "When a guest chooses Financial Contribution and picks a payment method, "
        "they will be sent directly to the real payment page / account.");
    gtk_label_set_xalign(GTK_LABEL(pay_note), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(pay_note), TRUE);
    gtk_box_append(GTK_BOX(box), pay_note);

    GtkWidget *pay_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(pay_grid), 14);
    gtk_grid_set_row_spacing(GTK_GRID(pay_grid), 8);
    gtk_box_append(GTK_BOX(box), pay_grid);

    /* MoMo operator */
    gtk_grid_attach(GTK_GRID(pay_grid),
                    gtk_label_new("Mobile Money Operator:"), 0, 0, 1, 1);
    fab_w.entry_momo_operator = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(fab_w.entry_momo_operator),
                                   "Orange  or  MTN");
    gtk_editable_set_text(GTK_EDITABLE(fab_w.entry_momo_operator), MOMO_OPERATOR);
    gtk_widget_set_hexpand(fab_w.entry_momo_operator, TRUE);
    gtk_grid_attach(GTK_GRID(pay_grid), fab_w.entry_momo_operator, 1, 0, 1, 1);

    /* MoMo phone */
    gtk_grid_attach(GTK_GRID(pay_grid),
                    gtk_label_new("Mobile Money Phone Number:"), 0, 1, 1, 1);
    fab_w.entry_momo_phone = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(fab_w.entry_momo_phone),
                                   "e.g. +237600000000");
    gtk_editable_set_text(GTK_EDITABLE(fab_w.entry_momo_phone), MOMO_PHONE);
    gtk_widget_set_hexpand(fab_w.entry_momo_phone, TRUE);
    gtk_grid_attach(GTK_GRID(pay_grid), fab_w.entry_momo_phone, 1, 1, 1, 1);

    /* Bank name */
    gtk_grid_attach(GTK_GRID(pay_grid),
                    gtk_label_new("Bank Name:"), 0, 2, 1, 1);
    fab_w.entry_bank_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(fab_w.entry_bank_name),
                                   "e.g. Afriland First Bank");
    gtk_editable_set_text(GTK_EDITABLE(fab_w.entry_bank_name), BANK_NAME);
    gtk_widget_set_hexpand(fab_w.entry_bank_name, TRUE);
    gtk_grid_attach(GTK_GRID(pay_grid), fab_w.entry_bank_name, 1, 2, 1, 1);

    /* Bank account number */
    gtk_grid_attach(GTK_GRID(pay_grid),
                    gtk_label_new("Bank Account Number:"), 0, 3, 1, 1);
    fab_w.entry_bank_account = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(fab_w.entry_bank_account),
                                   "e.g. 12345-67890-XYZ");
    gtk_editable_set_text(GTK_EDITABLE(fab_w.entry_bank_account), BANK_ACCOUNT);
    gtk_widget_set_hexpand(fab_w.entry_bank_account, TRUE);
    gtk_grid_attach(GTK_GRID(pay_grid), fab_w.entry_bank_account, 1, 3, 1, 1);

    /* Bank account holder name */
    gtk_grid_attach(GTK_GRID(pay_grid),
                    gtk_label_new("Account Holder Name:"), 0, 4, 1, 1);
    fab_w.entry_bank_account_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(fab_w.entry_bank_account_name),
                                   "e.g. Group3 Wedding Fund");
    gtk_editable_set_text(GTK_EDITABLE(fab_w.entry_bank_account_name), BANK_ACCOUNT_NAME);
    gtk_widget_set_hexpand(fab_w.entry_bank_account_name, TRUE);
    gtk_grid_attach(GTK_GRID(pay_grid), fab_w.entry_bank_account_name, 1, 4, 1, 1);

    /* Save button */
    GtkWidget *save_btn = gtk_button_new_with_label("💾  Save Settings & Activate");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_fab_admin_save), NULL);
    gtk_grid_attach(GTK_GRID(pay_grid), save_btn, 1, 5, 1, 1);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Status / sold info ---- */
    SECTION_LBL(box, "Current Status");

    fab_w.lbl_sold_info = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(fab_w.lbl_sold_info), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(fab_w.lbl_sold_info), TRUE);
    gtk_box_append(GTK_BOX(box), fab_w.lbl_sold_info);
    fabric_refresh_admin_info();   /* populate immediately from current state */

    fab_w.lbl_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(fab_w.lbl_status), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(fab_w.lbl_status), TRUE);
    gtk_box_append(GTK_BOX(box), fab_w.lbl_status);

    return box;
}

/* =========================================================
 * Wedding Fabrics – Guest tab
 * =========================================================
 *
 * fabric_refresh_guest_panel is called:
 *   - When the guest fabric tab is first built.
 *   - After the admin saves fabric settings (on_fab_admin_save).
 *   - After a guest successfully records fabric interest (on_fab_guest_buy).
 *
 * It shows or hides the entire content box based on fabric_cfg.active.
 */

/*
 * fabric_refresh_guest_panel
 * --------------------------
 * Synchronise the guest fabric tab's widgets with the current fabric_cfg.
 *   - Hides the entire guest_fabric_box when the offer is inactive.
 *   - Updates the fabric name label, image, and availability message when active.
 */
static void fabric_refresh_guest_panel(void) {
    if (!fab_w.guest_fabric_box) return;

    /* Show or hide the whole panel based on active flag */
    gtk_widget_set_visible(fab_w.guest_fabric_box, fabric_cfg.active ? TRUE : FALSE);
    if (!fabric_cfg.active) return;

    /* Fabric name */
    gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_name),
                       FABRIC_NAMES[fabric_cfg.fabric_index]);

    /* Fabric image */
    if (fab_w.guest_img)
        fab_load_image(fab_w.guest_img, fabric_cfg.fabric_index);

    /* Availability status (no stock tracking; always available) */
    (void)fabric_remaining();   /* kept to silence unused-function warning */
    int sold = fabric_units_sold();
    char stock_buf[160];
    snprintf(stock_buf, sizeof(stock_buf),
             "✔ Available — %d interest%s registered. "
             "Click below to go to the online shop and purchase.",
             sold, sold == 1 ? "" : "s");
    gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_stock), stock_buf);
}

/*
 * on_fab_guest_verify
 * -------------------
 * "Verify Identity" callback for the guest fabric tab.
 * Identical logic to other identity-verify callbacks.
 */
static void on_fab_guest_verify(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    const char *nm = gtk_editable_get_text(GTK_EDITABLE(fab_w.guest_entry_name));
    if (strlen(nm) < 1) {
        gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_verify),
                           "Please enter your full name.");
        return;
    }
    if (find_guest_by_name(nm, &fab_w.guest_verified)) {
        fab_w.guest_ok = 1;
        char msg[160];
        snprintf(msg, sizeof(msg), "✔ Verified: %s", fab_w.guest_verified.name);
        gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_verify), msg);
    } else {
        fab_w.guest_ok = 0;
        gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_verify),
            "No guest found with that name. Please re-enter exactly as registered.");
    }
}

/*
 * on_fab_guest_buy  ("🛒 Go to Online Shop & Buy Fabric" button)
 * ----------------
 * Records the guest's fabric interest as a GiftRecord with item_id = -1,
 * opens the real online shop URL in the default browser, resets the form,
 * and refreshes both panels.
 *
 * item_id == -1 is the sentinel for fabric interest (distinct from regular
 * gifts with positive IDs and financial contributions with item_id == 0).
 */
static void on_fab_guest_buy(GtkButton *b, gpointer d) {
    (void)b; (void)d;

    if (!fabric_cfg.active) {
        gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_result),
                           "Fabric offer is currently not active.");
        return;
    }
    if (!fab_w.guest_ok) {
        gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_result),
                           "Please verify your identity first.");
        return;
    }

    int fidx = fabric_cfg.fabric_index;

    /* Record interest in gifts.csv (item_id = -1) */
    GiftRecord g;
    memset(&g, 0, sizeof(g));
    g.record_id  = get_next_gift_id();
    g.guest_id   = fab_w.guest_verified.id;
    strncpy(g.guest_name, fab_w.guest_verified.name, sizeof(g.guest_name) - 1);
    find_category_for_guest(g.guest_id, g.category, sizeof(g.category));
    g.item_id    = -1;   /* -1 = fabric purchase */
    snprintf(g.item_name, sizeof(g.item_name),
             "Wedding Fabric [%s]", FABRIC_NAMES[fidx]);
    g.quantity   = 1;
    g.total_fcfa = 0.0;   /* price set on shop side */
    g.total_eur  = 0.0;
    strncpy(g.thanked_by, fab_w.guest_verified.name, sizeof(g.thanked_by) - 1);

    FILE *f = fopen(GIFT_CSV, "a");
    if (!f) {
        gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_result),
                           "Error: could not open gifts.csv");
        return;
    }
    write_gift_line(f, &g);
    fclose(f);

    /* Compose the confirmation message */
    char msg[400];
    snprintf(msg, sizeof(msg),
             "✔ Interest recorded (Record ID: %d).\n"
             "Fabric: %s\n\n"
             "🛒 Opening the online shop now — complete your purchase there.\n"
             "Shop URL: %s",
             g.record_id,
             FABRIC_NAMES[fidx],
             FABRIC_SHOP_URLS[fidx]);
    gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_result), msg);

    /* Open real online shop URL in default browser */
    g_app_info_launch_default_for_uri(FABRIC_SHOP_URLS[fidx], NULL, NULL);

    /* Reset form */
    gtk_editable_set_text(GTK_EDITABLE(fab_w.guest_entry_name), "");
    gtk_label_set_text(GTK_LABEL(fab_w.guest_lbl_verify), "");
    fab_w.guest_ok = 0;

    /* Refresh info: admin sold-count and guest availability message */
    fabric_refresh_guest_panel();
    fabric_refresh_admin_info();
}

/*
 * build_guest_fabric_tab
 * ----------------------
 * Construct the "🧵 Wedding Fabric" guest tab widget.
 *
 * Two-layer layout:
 *   - An "inactive" notice label (always present, shown when offer is off).
 *   - The "active content box" (guest_fabric_box, shown only when active).
 *
 * fabric_refresh_guest_panel() at the end of the function performs the
 * initial show/hide based on the current fabric_cfg.active value.
 *
 * Active content layout:
 *   Intro text
 *   ── separator ──
 *   Selected Fabric: name label + large image (300×300) + availability label
 *   ── separator ──
 *   Step 1: Identity verification
 *   ── separator ──
 *   Step 2: [🛒 Go to Online Shop & Buy Fabric] button + result label
 */
static GtkWidget *build_guest_fabric_tab(void) {
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(outer, 24);
    gtk_widget_set_margin_end(outer, 24);
    gtk_widget_set_margin_top(outer, 16);
    gtk_widget_set_margin_bottom(outer, 16);

    /* Inactive notice — shown when admin has not activated a fabric */
    GtkWidget *inactive_lbl = gtk_label_new(
        "No fabric offer is currently available. "
        "Please check back later or ask an organiser.");
    gtk_label_set_xalign(GTK_LABEL(inactive_lbl), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(inactive_lbl), TRUE);
    gtk_box_append(GTK_BOX(outer), inactive_lbl);

    /* Active content box — hidden when inactive */
    fab_w.guest_fabric_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_append(GTK_BOX(outer), fab_w.guest_fabric_box);
    GtkWidget *box = fab_w.guest_fabric_box;

    GtkWidget *intro = gtk_label_new(
        "The couple has selected a wedding fabric for the City Council ceremony. "
        "Verify your identity, then click the button below to be taken to the "
        "official online shop where you can purchase the fabric directly.");
    gtk_label_set_xalign(GTK_LABEL(intro), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(intro), TRUE);
    gtk_box_append(GTK_BOX(box), intro);
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Fabric image & name ---- */
    SECTION_LBL(box, "Selected Fabric");

    fab_w.guest_lbl_name = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(fab_w.guest_lbl_name), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(fab_w.guest_lbl_name), TRUE);
    gtk_box_append(GTK_BOX(box), fab_w.guest_lbl_name);

    /* Fabric image – large preview centred, GtkPicture scales to fill */
    fab_w.guest_img = gtk_picture_new();
    gtk_picture_set_content_fit(GTK_PICTURE(fab_w.guest_img), GTK_CONTENT_FIT_COVER);
    gtk_widget_set_size_request(fab_w.guest_img, 300, 300);
    gtk_widget_set_halign(fab_w.guest_img, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(fab_w.guest_img, 8);
    gtk_widget_set_margin_bottom(fab_w.guest_img, 8);
    gtk_box_append(GTK_BOX(box), fab_w.guest_img);

    fab_w.guest_lbl_stock = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(fab_w.guest_lbl_stock), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(fab_w.guest_lbl_stock), TRUE);
    gtk_box_append(GTK_BOX(box), fab_w.guest_lbl_stock);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Step 1: Identity verification ---- */
    SECTION_LBL(box, "Step 1 : Verify your identity");
    GtkWidget *id_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(id_grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(id_grid), 8);
    gtk_box_append(GTK_BOX(box), id_grid);

    gtk_grid_attach(GTK_GRID(id_grid), gtk_label_new("Full Name:"), 0, 0, 1, 1);
    fab_w.guest_entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(fab_w.guest_entry_name),
                                   "Exactly as registered");
    gtk_widget_set_hexpand(fab_w.guest_entry_name, TRUE);
    gtk_grid_attach(GTK_GRID(id_grid), fab_w.guest_entry_name, 1, 0, 1, 1);

    GtkWidget *bv = gtk_button_new_with_label("Verify Identity");
    g_signal_connect(bv, "clicked", G_CALLBACK(on_fab_guest_verify), NULL);
    gtk_grid_attach(GTK_GRID(id_grid), bv, 1, 1, 1, 1);

    fab_w.guest_lbl_verify = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(fab_w.guest_lbl_verify), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(fab_w.guest_lbl_verify), TRUE);
    gtk_box_append(GTK_BOX(box), fab_w.guest_lbl_verify);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* ---- Step 2: Go to shop ---- */
    SECTION_LBL(box, "Step 2 : Go to the Online Shop to Buy");

    GtkWidget *shop_note = gtk_label_new(
        "Clicking the button below will open the official shop page for this fabric "
        "in your web browser. Your interest will be recorded in the system.");
    gtk_label_set_xalign(GTK_LABEL(shop_note), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(shop_note), TRUE);
    gtk_box_append(GTK_BOX(box), shop_note);

    GtkWidget *buy_btn = gtk_button_new_with_label("🛒  Go to Online Shop & Buy Fabric");
    gtk_widget_set_halign(buy_btn, GTK_ALIGN_START);
    g_signal_connect(buy_btn, "clicked", G_CALLBACK(on_fab_guest_buy), NULL);
    gtk_box_append(GTK_BOX(box), buy_btn);

    fab_w.guest_lbl_result = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(fab_w.guest_lbl_result), TRUE);
    gtk_label_set_xalign(GTK_LABEL(fab_w.guest_lbl_result), 0.0f);
    gtk_box_append(GTK_BOX(box), fab_w.guest_lbl_result);

    /* Populate everything from current config */
    fabric_refresh_guest_panel();

    return outer;
}

/*
 * build_catalogue_tab
 * -------------------
 * Construct the "Manage Catalogue" admin tab widget.
 *
 * Layout:
 *   Current Gift Catalogue section (scrolled GtkTextView, read-only)
 *   ── separator ──
 *   Add a New Gift section (name entry, price entry, [＋ Add Gift] button)
 *   Status label
 */
static GtkWidget *build_catalogue_tab(void) {
    memset(&cat_w, 0, sizeof(cat_w));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    SECTION_LBL(box, "Current Gift Catalogue");

    /* Read-only monospace text view for the current catalogue */
    cat_w.list_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(cat_w.list_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(cat_w.list_view), TRUE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(cat_w.list_view), FALSE);
    gtk_widget_set_vexpand(cat_w.list_view, TRUE);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), cat_w.list_view);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_size_request(scroll, -1, 200);
    gtk_box_append(GTK_BOX(box), scroll);

    cat_refresh_list();   /* populate immediately */

    gtk_box_append(GTK_BOX(box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    SECTION_LBL(box, "Add a New Gift to the Catalogue");

    GtkWidget *note = gtk_label_new(
        "The new item will appear immediately in all guest / admin dropdowns.");
    gtk_label_set_xalign(GTK_LABEL(note), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(note), TRUE);
    gtk_box_append(GTK_BOX(box), note);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_box_append(GTK_BOX(box), grid);

    gtk_grid_attach(GTK_GRID(grid),
                    gtk_label_new("Gift Name:"), 0, 0, 1, 1);
    cat_w.entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(cat_w.entry_name),
                                   "e.g. Coffee Machine");
    gtk_widget_set_hexpand(cat_w.entry_name, TRUE);
    gtk_grid_attach(GTK_GRID(grid), cat_w.entry_name, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(grid),
                    gtk_label_new("Price (FCFA):"), 0, 1, 1, 1);
    cat_w.entry_price = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(cat_w.entry_price),
                                   "e.g. 45000");
    gtk_widget_set_hexpand(cat_w.entry_price, TRUE);
    gtk_grid_attach(GTK_GRID(grid), cat_w.entry_price, 1, 1, 1, 1);

    GtkWidget *add_btn = gtk_button_new_with_label("  ＋ Add Gift to Catalogue");
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_cat_add), NULL);
    gtk_grid_attach(GTK_GRID(grid), add_btn, 1, 2, 1, 1);

    cat_w.lbl_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(cat_w.lbl_status), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(cat_w.lbl_status), TRUE);
    gtk_box_append(GTK_BOX(box), cat_w.lbl_status);

    return box;
}

/*
 * build_admin_page
 * ----------------
 * Construct the full "Admin Panel" page widget.
 *
 * The page uses a GtkNotebook with 5 tabs:
 *   1. Display Gifts       – do_refresh() (password-gated)
 *   2. Delete Gift         – temp-file rewrite
 *   3. Update Gift         – temp-file rewrite + auto-refresh display
 *   4. Manage Catalogue    – add items to cat[]
 *   5. 🧵 Wedding Fabrics – fabric admin + payment config
 *
 * Tabs 2-5 are each wrapped in a GtkScrolledWindow.
 * upd_w.disp_w_ptr is set to &disp_w so the Update tab can trigger a
 * display refresh without a hard dependency.
 */
static GtkWidget *build_admin_page(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* Top bar: [← Back] title */
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(top_bar, 12);
    gtk_widget_set_margin_end(top_bar, 12);
    gtk_widget_set_margin_top(top_bar, 8);
    gtk_widget_set_margin_bottom(top_bar, 8);
    gtk_box_append(GTK_BOX(vbox), top_bar);

    GtkWidget *back = gtk_button_new_with_label("← Back");
    g_signal_connect(back, "clicked", G_CALLBACK(on_back_home_admin), NULL);
    gtk_box_append(GTK_BOX(top_bar), back);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
                         "<b>Admin Panel</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_widget_set_hexpand(title, TRUE);
    gtk_box_append(GTK_BOX(top_bar), title);

    gtk_box_append(GTK_BOX(vbox),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *nb = gtk_notebook_new();
    gtk_widget_set_vexpand(nb, TRUE);
    gtk_box_append(GTK_BOX(vbox), nb);

    /* Tab 1: Display Gifts (no scroll needed; GtkTextView has its own scroll) */
    gtk_notebook_append_page(GTK_NOTEBOOK(nb),
                             build_display_tab(&disp_w),
                             gtk_label_new("Display Gifts"));

    /* Tab 2: Delete Gift */
    GtkWidget *s3 = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(s3),
                                  build_delete_tab(&del_w));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), s3,
                             gtk_label_new("Delete Gift"));

    /* Tab 3: Update Gift – wire disp_w_ptr for auto-refresh */
    GtkWidget *s4 = gtk_scrolled_window_new();
    upd_w.disp_w_ptr = &disp_w;
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(s4),
                                  build_update_tab(&upd_w));
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), s4,
                             gtk_label_new("Update Gift"));

    /* Tab 4: Manage Catalogue */
    GtkWidget *s5 = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(s5),
                                  build_catalogue_tab());
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), s5,
                             gtk_label_new("Manage Catalogue"));

    /* Tab 5: Wedding Fabrics */
    GtkWidget *s6 = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(s6),
                                  build_admin_fabric_tab());
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), s6,
                             gtk_label_new("🧵 Wedding Fabrics"));

    return vbox;
}

/*
 * on_activate
 * -----------
 * GtkApplication "activate" signal handler – the application entry point.
 *
 * Responsibilities:
 *   1. Install application-wide CSS (taken-item styling + dropdown width).
 *   2. Create the main GtkApplicationWindow.
 *   3. Set a custom header bar with the application title.
 *   4. Create a GtkStack and add all four top-level pages to it.
 *   5. Show the home page and present the window.
 *
 * The GtkStack uses a slide-left/right transition (250 ms) for navigation.
 */
static void on_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    _suppress_unused();   /* prevent "unused function" warning for show_alert */

    /* Install CSS for taken-item highlighting and dropdown minimum width */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        ".taken-item {"
        "  background-color: #1a1a1a;"
        "  color: #888888;"
        "}"
        ".taken-item > * {"
        "  color: #888888;"
        "}"

        "popover > contents > scrolledwindow > listview {"
        "  min-width: 620px;"
        "}"
        "popover > contents > scrolledwindow > listview > row {"
        "  padding: 2px 4px;"
        "  min-height: 28px;"
        "}");
    GdkDisplay *display = gdk_display_get_default();
    if (display)
        gtk_style_context_add_provider_for_display(
            display,
            GTK_STYLE_PROVIDER(css),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    /* Main window */
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window),
                         "Wedding Gift Manager - group3wed");
    gtk_window_set_default_size(GTK_WINDOW(window), 920, 660);

    /* Custom header bar with the application title as a markup label */
    GtkWidget *header = gtk_header_bar_new();
    GtkWidget *title_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_lbl),
                         "<b>Wedding Gift Manager</b>  -  group3wed");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title_lbl);
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    /* GtkStack with slide transition holds all top-level pages */
    app_state.stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app_state.stack),
                                  GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(app_state.stack), 250);
    gtk_window_set_child(GTK_WINDOW(window), app_state.stack);

    /* Add all pages; the names are used by show_page() for navigation */
    gtk_stack_add_named(GTK_STACK(app_state.stack),
                        build_home_page(),       "home");
    gtk_stack_add_named(GTK_STACK(app_state.stack),
                        build_guest_page(),      "guest");
    gtk_stack_add_named(GTK_STACK(app_state.stack),
                        build_admin_login_page(),"admin_login");
    gtk_stack_add_named(GTK_STACK(app_state.stack),
                        build_admin_page(),      "admin");

    /* Start on the home page */
    gtk_stack_set_visible_child_name(GTK_STACK(app_state.stack), "home");

    gtk_window_present(GTK_WINDOW(window));
}

/*
 * main
 * ----
 * Application entry point.
 *
 * 1. init_csv_paths() – resolve all CSV file paths relative to the binary.
 * 2. fabric_load()    – restore fabric/payment settings from fabric.csv.
 * 3. Create and run a GtkApplication with app ID "com.group3wed.giftmanager".
 *    G_APPLICATION_NON_UNIQUE allows multiple instances (useful for testing).
 * 4. Return the GTK run status (0 = normal exit).
 */
int main(int argc, char **argv) {
    init_csv_paths();   /* resolve CSV paths to the binary's own directory */
    fabric_load();      /* restore fabric settings from previous session    */
    GtkApplication *app = gtk_application_new(
        "com.group3wed.giftmanager",
        G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
