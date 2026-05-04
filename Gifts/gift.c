/*
 * gift.c  --  Wedding Gift Management System (terminal back-end)
 *
 * Compile (terminal only, no GTK):
 *   gcc -Wall -Wextra -o gift gift.c -lm
 *
 * Compile with GTK 4 front-end:
 *   gcc -Wall -Wextra $(pkg-config --cflags gtk4) -o wedding_gift gift.c gift_gtk.c $(pkg-config --libs gtk4) -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "gift.h"

/* ================================================================== */
/*  Gift catalogue with moderate mid-range FCFA prices                 */
/*  (Central/West Africa market, 2024-2025 estimates)                  */
/*  1 EURO ≈ 655.957 FCFA                                               */
/* ================================================================== */
const GiftType GIFT_CATALOGUE[NUM_GIFT_TYPES] = {
    /* index  name                     unit price FCFA  (~EUR equiv)  */
    /*  0 */  { "House (contribution)",   5000000  },  /* ~7622 EUR  */
    /*  1 */  { "Car (contribution)",     3500000  },  /* ~5335 EUR  */
    /*  2 */  { "Pot Set",                  35000  },  /* ~53 EUR    */
    /*  3 */  { "Spoon Set",                 8000  },  /* ~12 EUR    */
    /*  4 */  { "Fork Set",                  8000  },  /* ~12 EUR    */
    /*  5 */  { "Dishes Set",               25000  },  /* ~38 EUR    */
    /*  6 */  { "Glass Set",                12000  },  /* ~18 EUR    */
    /*  7 */  { "Microwave",                65000  },  /* ~99 EUR    */
    /*  8 */  { "Customized Photo Frame",   15000  },  /* ~23 EUR    */
    /*  9 */  { "Travel Tickets (couple)", 400000  },  /* ~610 EUR   */
    /* 10 */  { "Hotel Reservation (week)",300000  },  /* ~457 EUR   */
    /* 11 */  { "Wine (bottle)",             8000  },  /* ~12 EUR    */
    /* 12 */  { "Smoothing Iron",           18000  },  /* ~27 EUR    */
    /* 13 */  { "Bedside Table",            45000  },  /* ~69 EUR    */
    /* 14 */  { "Cushion Set",              20000  },  /* ~30 EUR    */
    /* 15 */  { "Curtain Set",              30000  },  /* ~46 EUR    */
    /* 16 */  { "Television (43 inch)",    180000  },  /* ~274 EUR   */
    /* 17 */  { "Flower Pots (set of 3)",   12000  },  /* ~18 EUR    */
    /* 18 */  { "Kettle",                   15000  },  /* ~23 EUR    */
    /* 19 */  { "Cup Set",                  10000  },  /* ~15 EUR    */
    /* 20 */  { "Sheet Set",                22000  },  /* ~34 EUR    */
    /* 21 */  { "Jewelry Set",             120000  },  /* ~183 EUR   */
    /* 22 */  { "Couples Watches",          95000  },  /* ~145 EUR   */
    /* 23 */  { "Home Decor Set",           40000  },  /* ~61 EUR    */
    /* 24 */  { "Gas Stove",                55000  },  /* ~84 EUR    */
    /* 25 */  { "Money Donation (FCFA)",         0 },  /* user enters amount */
};

/* ================================================================== */
/*  Internal helpers                                                    */
/* ================================================================== */

static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void read_line(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, size, stdin)) {
        if (buf[strlen(buf) - 1] != '\n') flush_stdin();
        buf[strcspn(buf, "\n")] = '\0';
    }
}

static int read_int(const char *prompt) {
    char buf[32];
    read_line(prompt, buf, sizeof(buf));
    return atoi(buf);
}

static long read_long(const char *prompt) {
    char buf[32];
    read_line(prompt, buf, sizeof(buf));
    return atol(buf);
}

/* Case-insensitive string comparison */
static int str_iequal(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

/* ================================================================== */
/*  Password check                                                      */
/* ================================================================== */
int check_password_gift(void) {
    char pw[64];
    int tries = 3;
    while (tries-- > 0) {
        read_line("Password: ", pw, sizeof(pw));
        if (strcmp(pw, PASSWORD) == 0) return 1;
        if (tries > 0) printf("Incorrect password. %d attempt(s) remaining.\n", tries);
    }
    printf("Too many failed attempts.\n");
    return 0;
}

/* ================================================================== */
/*  Guest-name verification                                             */
/*  Checks guests.csv first, then categories.csv.                      */
/*  Fills out_id and out_category on success.                          */
/* ================================================================== */
int verify_guest_name(const char *name, int *out_id, char *out_category, int cat_size) {
    char line[512];

    /* --- Search guests.csv ---------------------------------------- */
    FILE *fg = fopen(GUEST_CSV_FILE, "r");
    if (fg) {
        while (fgets(line, sizeof(line), fg)) {
            /* Format: id,name,age,status,phone,side,parking */
            int  id; char gname[100]; char rest[256];
            if (sscanf(line, "%d,%99[^,],%255[^\n]", &id, gname, rest) >= 2) {
                if (str_iequal(gname, name)) {
                    /* Extract status (3rd field) as category */
                    char age_s[16], status[50];
                    sscanf(rest, "%15[^,],%49[^,]", age_s, status);
                    *out_id = id;
                    strncpy(out_category, status, cat_size - 1);
                    out_category[cat_size - 1] = '\0';
                    fclose(fg);
                    return 1;
                }
            }
        }
        fclose(fg);
    }

    /* --- Search categories.csv ------------------------------------ */
    /* Format assumed: cat_id,cat_code,guest_id,guest_name           */
    FILE *fc = fopen(CAT_CSV_FILE, "r");
    if (fc) {
        while (fgets(line, sizeof(line), fc)) {
            int cat_id, guest_id; char cat_code[50]; char gname[100];
            if (sscanf(line, "%d,%49[^,],%d,%99[^\n]", &cat_id, cat_code, &guest_id, gname) == 4) {
                /* strip trailing newline from gname */
                gname[strcspn(gname, "\r\n")] = '\0';
                if (str_iequal(gname, name)) {
                    *out_id = guest_id;
                    strncpy(out_category, cat_code, cat_size - 1);
                    out_category[cat_size - 1] = '\0';
                    fclose(fc);
                    return 1;
                }
            }
        }
        fclose(fc);
    }

    return 0;   /* not found in either file */
}

/* ================================================================== */
/*  CSV helpers                                                         */
/* ================================================================== */

static void write_gift_line(FILE *f, const Gift *g) {
    fprintf(f, "%d,%s,%d,%s,%s,%d,%ld,%ld\n",
            g->id, g->guest_name, g->guest_id, g->category,
            g->gift_name, g->quantity, g->unit_price_fcfa, g->total_fcfa);
}

static int parse_gift_line(const char *line, Gift *g) {
    return sscanf(line,
                  "%d,%99[^,],%d,%49[^,],%99[^,],%d,%ld,%ld",
                  &g->id, g->guest_name, &g->guest_id, g->category,
                  g->gift_name, &g->quantity, &g->unit_price_fcfa, &g->total_fcfa) == 8;
}

int gift_get_next_id(void) {
    FILE *f = fopen(GIFT_CSV_FILE, "r");
    if (!f) return 0;
    int max_id = -1, id;
    char line[512];
    while (fgets(line, sizeof(line), f))
        if (sscanf(line, "%d,", &id) == 1 && id > max_id) max_id = id;
    fclose(f);
    return max_id + 1;
}

int gift_load_all(Gift **out) {
    FILE *f = fopen(GIFT_CSV_FILE, "r");
    if (!f) { *out = NULL; return 0; }

    /* Count lines first */
    int count = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) count++;
    rewind(f);

    Gift *arr = (Gift *)malloc((count + 1) * sizeof(Gift));
    if (!arr) { fclose(f); *out = NULL; return 0; }

    int n = 0;
    while (fgets(line, sizeof(line), f))
        if (parse_gift_line(line, &arr[n])) n++;

    fclose(f);
    *out = arr;
    return n;
}

int gift_save_one(const Gift *g) {
    FILE *f = fopen(GIFT_CSV_FILE, "a");
    if (!f) { perror("Cannot open gifts.csv"); return 0; }
    write_gift_line(f, g);
    fclose(f);
    return 1;
}

int gift_delete_by_id(int id) {
    FILE *f = fopen(GIFT_CSV_FILE, "r");
    if (!f) return 0;
    FILE *tmp = fopen(GIFT_TMP_FILE, "w");
    if (!tmp) { fclose(f); return 0; }

    char line[512]; Gift g; int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!parse_gift_line(line, &g)) continue;
        if (g.id != id) write_gift_line(tmp, &g);
        else found = 1;
    }
    fclose(f); fclose(tmp);
    remove(GIFT_CSV_FILE);
    rename(GIFT_TMP_FILE, GIFT_CSV_FILE);
    return found;
}

int gift_update_by_id(int id, const Gift *updated) {
    FILE *f = fopen(GIFT_CSV_FILE, "r");
    if (!f) return 0;
    FILE *tmp = fopen(GIFT_TMP_FILE, "w");
    if (!tmp) { fclose(f); return 0; }

    char line[512]; Gift g; int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (!parse_gift_line(line, &g)) continue;
        if (g.id == id) { write_gift_line(tmp, updated); found = 1; }
        else              write_gift_line(tmp, &g);
    }
    fclose(f); fclose(tmp);
    remove(GIFT_CSV_FILE);
    rename(GIFT_TMP_FILE, GIFT_CSV_FILE);
    return found;
}

/* ================================================================== */
/*  Print gift catalogue                                                */
/* ================================================================== */
static void print_catalogue(void) {
    printf("\n%-4s %-32s %15s %12s\n", "No.", "Gift", "Price (FCFA)", "Price (EUR)");
    printf("%-4s %-32s %15s %12s\n", "---", "----", "------------", "-----------");
    for (int i = 0; i < NUM_GIFT_TYPES; i++) {
        if (i == NUM_GIFT_TYPES - 1) {
            /* Money donation – price entered by user */
            printf("%-4d %-32s %15s %12s\n", i + 1,
                   GIFT_CATALOGUE[i].name, "Your choice", "Your choice");
        } else {
            double eur = GIFT_CATALOGUE[i].price_fcfa / EUR_TO_FCFA;
            printf("%-4d %-32s %15ld %12.2f\n",
                   i + 1, GIFT_CATALOGUE[i].name,
                   GIFT_CATALOGUE[i].price_fcfa, eur);
        }
    }
    printf("\n");
}

/* ================================================================== */
/*  ACTION: Add / Register a gift                                       */
/* ================================================================== */
void action_add_gift(void) {
    printf("\n=== Register a Gift ===\n");

    /* Step 1 = verify guest name (up to 3 tries) */
    char guest_name[100];
    int  guest_id = -1;
    char category[50] = "Unknown";
    int  verified = 0;

    for (int attempt = 0; attempt < 3 && !verified; attempt++) {
        read_line("Enter your full name (as registered): ", guest_name, sizeof(guest_name));
        if (verify_guest_name(guest_name, &guest_id, category, sizeof(category))) {
            verified = 1;
        } else {
            printf("Name not found in the guest list. Please re-enter the correct name.\n");
        }
    }
    if (!verified) {
        printf("Could not verify your name. Gift registration cancelled.\n");
        return;
    }
    printf("Welcome, %s! (ID: %d | Category: %s)\n", guest_name, guest_id, category);

    /* Step 2 = show catalogue and let guest choose */
    print_catalogue();
    int choice = read_int("Enter gift number from the list above: ");
    if (choice < 1 || choice > NUM_GIFT_TYPES) {
        printf("Invalid choice.\n"); return;
    }
    int idx = choice - 1;

    Gift g;
    g.id             = gift_get_next_id();
    g.guest_id       = guest_id;
    strncpy(g.guest_name, guest_name, sizeof(g.guest_name) - 1);
    g.guest_name[sizeof(g.guest_name) - 1] = '\0';
    strncpy(g.category, category, sizeof(g.category) - 1);
    g.category[sizeof(g.category) - 1] = '\0';
    strncpy(g.gift_name, GIFT_CATALOGUE[idx].name, sizeof(g.gift_name) - 1);
    g.gift_name[sizeof(g.gift_name) - 1] = '\0';

    /* Step 3 quantity / amount */
    if (idx == NUM_GIFT_TYPES - 1) {
        /* Money donation */
        g.unit_price_fcfa = read_long("Enter the amount to donate (FCFA): ");
        g.quantity        = 1;
        g.total_fcfa      = g.unit_price_fcfa;
    } else {
        g.unit_price_fcfa = GIFT_CATALOGUE[idx].price_fcfa;
        printf("Unit price: %ld FCFA (%.2f EUR)\n",
               g.unit_price_fcfa, g.unit_price_fcfa / EUR_TO_FCFA);
        g.quantity   = read_int("How many would you like to offer? ");
        if (g.quantity < 1) { printf("Invalid quantity.\n"); return; }
        g.total_fcfa = g.unit_price_fcfa * (long)g.quantity;
    }

    printf("\n--- Order Summary ---\n");
    printf("Gift    : %s\n",   g.gift_name);
    printf("Quantity: %d\n",   g.quantity);
    printf("Total   : %ld FCFA  (%.2f EUR)\n",
           g.total_fcfa, g.total_fcfa / EUR_TO_FCFA);

    /* Step 4 – confirm giver name for thank-you message */
    char giver[100];
    read_line("Enter your name so the couple can thank you: ", giver, sizeof(giver));
    printf("\nThank you, %s! The bride and groom are grateful for your generous gift.\n", giver);

    gift_save_one(&g);
    printf("Gift recorded with ID: %d\n", g.id);
}

/* ================================================================== */
/*  ACTION: Display all gifts                                           */
/* ================================================================== */
void action_display_gifts(void) {
    printf("\n=== Display All Gifts ===\n");
    if (!check_password_gift()) return;

    Gift *arr = NULL;
    int n = gift_load_all(&arr);
    if (n == 0) {
        printf("No gifts recorded yet.\n");
        free(arr);
        return;
    }

    long grand_total = 0;
    printf("\n| %-4s | %-20s | %-4s | %-10s | %-28s | %-4s | %-12s | %-12s |\n",
           "ID", "Guest Name", "GID", "Category", "Gift", "Qty", "Unit(FCFA)", "Total(FCFA)");
    printf("|------|----------------------|------|------------|"
           "------------------------------|------|--------------|---------------|\n");

    for (int i = 0; i < n; i++) {
        printf("| %-4d | %-20s | %-4d | %-10s | %-28s | %-4d | %-12ld | %-12ld |\n",
               arr[i].id, arr[i].guest_name, arr[i].guest_id, arr[i].category,
               arr[i].gift_name, arr[i].quantity,
               arr[i].unit_price_fcfa, arr[i].total_fcfa);
        grand_total += arr[i].total_fcfa;
    }

    printf("\nTotal gifts: %d  |  Grand total value: %ld FCFA  (%.2f EUR)\n",
           n, grand_total, grand_total / EUR_TO_FCFA);
    free(arr);
}

/* ================================================================== */
/*  ACTION: Delete a gift                                               */
/* ================================================================== */
void action_delete_gift(void) {
    printf("\n=== Delete a Gift ===\n");
    if (!check_password_gift()) return;

    char guest_name[100];
    int  guest_id = -1; char category[50];
    int  verified = 0;

    for (int attempt = 0; attempt < 3 && !verified; attempt++) {
        read_line("Enter the guest name whose gift to delete: ", guest_name, sizeof(guest_name));
        if (verify_guest_name(guest_name, &guest_id, category, sizeof(category)))
            verified = 1;
        else
            printf("Name not found. Please re-enter the correct name.\n");
    }
    if (!verified) { printf("Deletion cancelled.\n"); return; }

    /* Show gifts belonging to this guest */
    Gift *arr = NULL;
    int n = gift_load_all(&arr);
    printf("\nGifts registered by %s:\n", guest_name);
    int any = 0;
    for (int i = 0; i < n; i++) {
        if (str_iequal(arr[i].guest_name, guest_name)) {
            printf("  ID %-4d | %-28s | Qty %-3d | %ld FCFA\n",
                   arr[i].id, arr[i].gift_name, arr[i].quantity, arr[i].total_fcfa);
            any = 1;
        }
    }
    free(arr);
    if (!any) { printf("No gifts found for this guest.\n"); return; }

    int del_id = read_int("Enter Gift ID to delete: ");
    if (gift_delete_by_id(del_id))
        printf("Gift ID %d deleted successfully.\n", del_id);
    else
        printf("Gift ID not found.\n");
}

/* ================================================================== */
/*  ACTION: Update a gift                                               */
/* ================================================================== */
void action_update_gift(void) {
    printf("\n=== Update a Gift ===\n");
    if (!check_password_gift()) return;

    char guest_name[100];
    int  guest_id = -1; char category[50];
    int  verified = 0;

    for (int attempt = 0; attempt < 3 && !verified; attempt++) {
        read_line("Enter the guest name whose gift to update: ", guest_name, sizeof(guest_name));
        if (verify_guest_name(guest_name, &guest_id, category, sizeof(category)))
            verified = 1;
        else
            printf("Name not found. Please re-enter the correct name.\n");
    }
    if (!verified) { printf("Update cancelled.\n"); return; }

    /* Show gifts for this guest */
    Gift *arr = NULL;
    int n = gift_load_all(&arr);
    printf("\nGifts registered by %s:\n", guest_name);
    int any = 0;
    for (int i = 0; i < n; i++) {
        if (str_iequal(arr[i].guest_name, guest_name)) {
            printf("  ID %-4d | %-28s | Qty %-3d | %ld FCFA\n",
                   arr[i].id, arr[i].gift_name, arr[i].quantity, arr[i].total_fcfa);
            any = 1;
        }
    }
    free(arr);
    if (!any) { printf("No gifts found for this guest.\n"); return; }

    int upd_id = read_int("Enter Gift ID to update: ");

    /* Load the specific record */
    arr = NULL; n = gift_load_all(&arr);
    Gift *target = NULL;
    for (int i = 0; i < n; i++)
        if (arr[i].id == upd_id && str_iequal(arr[i].guest_name, guest_name))
            { target = &arr[i]; break; }

    if (!target) {
        printf("Gift ID not found or does not belong to this guest.\n");
        free(arr); return;
    }

    printf("Current gift: %s  Qty: %d  Total: %ld FCFA\n",
           target->gift_name, target->quantity, target->total_fcfa);

    print_catalogue();
    int choice = read_int("Enter new gift number (0 to keep current): ");

    Gift updated = *target;

    if (choice >= 1 && choice <= NUM_GIFT_TYPES) {
        int idx = choice - 1;
        strncpy(updated.gift_name, GIFT_CATALOGUE[idx].name, sizeof(updated.gift_name) - 1);

        if (idx == NUM_GIFT_TYPES - 1) {
            updated.unit_price_fcfa = read_long("Enter donation amount (FCFA): ");
            updated.quantity        = 1;
        } else {
            updated.unit_price_fcfa = GIFT_CATALOGUE[idx].price_fcfa;
            printf("Unit price: %ld FCFA (%.2f EUR)\n",
                   updated.unit_price_fcfa, updated.unit_price_fcfa / EUR_TO_FCFA);
            updated.quantity = read_int("New quantity: ");
            if (updated.quantity < 1) updated.quantity = 1;
        }
    } else {
        /* Just update quantity */
        int new_qty = read_int("New quantity (0 to keep current): ");
        if (new_qty > 0) updated.quantity = new_qty;
    }

    updated.total_fcfa = updated.unit_price_fcfa * (long)updated.quantity;
    printf("New total: %ld FCFA  (%.2f EUR)\n",
           updated.total_fcfa, updated.total_fcfa / EUR_TO_FCFA);

    gift_update_by_id(upd_id, &updated);
    printf("Gift ID %d updated successfully.\n", upd_id);
    free(arr);
}

/* ================================================================== */
/*  main  (terminal mode)                                               */
/* ================================================================== */
int main(void) {
    int choice;
    do {
        printf("\n ===============================================================================\n");
        printf("******  Wedding Gift Management   ********\n");
        printf("===================================================================================\n");
        printf(" 1. Register a gift         \n");
        printf(" 2. Display all gifts       \n");
        printf(" 3. Delete a gift           \n");
        printf(" 4. Update a gift           \n");
        printf(" 0. Exit                    \n");
        printf("=====================================================================================�\n");

        choice = read_int("Choice: ");
        switch (choice) {
            case 1: action_add_gift();      break;
            case 2: action_display_gifts(); break;
            case 3: action_delete_gift();   break;
            case 4: action_update_gift();   break;
            case 0: printf("Goodbye!\n");   break;
            default: printf("Invalid choice.\n"); break;
        }
    } while (choice != 0);

    return 0;
}
