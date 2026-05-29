/* parking.c
 * Parking Management System – CLI version
 *
 * Two panels:
 *   Admin – password-protected: set number of spots / max hours / view status
 *   Guest – open: register a booking, view bookings, cancel a booking
 *
 * Storage: parking_spot.csv    (spot config written by admin)
 *          parking_booking.csv (guest reservations)
 *          persons.csv         (guest registry; must contain parking="Yes")
 *
 * Guest chooses their own spot number.  If the spot is occupied for the
 * requested time window the guest is told to pick another spot.
 *
 * One-booking-per-guest rule: a guest may not hold more than one active
 * reservation at a time regardless of time window.
 */

#include "parking.h"

/* ------------------------------------------------------------------ */
/*  Global state                                                        */
/* ------------------------------------------------------------------ */
int g_num_spots = 0;
int g_max_hours = 0;

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/* Read a trimmed line from stdin into buf (max len-1 chars) */
static void read_line(char *buf, int len) {
    if (!fgets(buf, len, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\r\n")] = '\0';
}

/* Print a horizontal rule */
static void print_rule(void) {
    puts("--------------------------------------------------");
}

/* Print a section title */
static void print_title(const char *t) {
    print_rule();
    printf("  %s\n", t);
    print_rule();
}

/* ================================================================== */
/*  Time utilities                                                      */
/* ================================================================== */

int to_minutes(int h, int m) {
    return h * 60 + m;
}

void add_hours(int sh, int sm, int dur_h, int *eh, int *em) {
    int total = to_minutes(sh, sm) + dur_h * 60;
    total %= (24 * 60);
    *eh = total / 60;
    *em = total % 60;
}

/* Returns 1 if [as,ae) overlaps [bs,be)  (values in minutes) */
int intervals_overlap(int as, int ae, int bs, int be) {
    if (ae <= as) ae += 24 * 60;   /* handle midnight wrap */
    if (be <= bs) be += 24 * 60;
    return (as < be) && (bs < ae);
}

/* ================================================================== */
/*  CSV persistence                                                     */
/* ================================================================== */

void save_spot_config(void) {
    FILE *f = fopen(SPOTS_FILE, "w");
    if (!f) { fprintf(stderr, "ERROR: Cannot write %s\n", SPOTS_FILE); return; }
    fprintf(f, "%d,%d\n", g_num_spots, g_max_hours);
    fclose(f);
}

/* Returns 1 on success, 0 if file missing or malformed */
int load_spot_config(void) {
    FILE *f = fopen(SPOTS_FILE, "r");
    if (!f) return 0;
    int r = fscanf(f, "%d,%d", &g_num_spots, &g_max_hours);
    fclose(f);
    return (r == 2);
}

void append_booking(const Booking *b) {
    FILE *f = fopen(BOOKINGS_FILE, "a");
    if (!f) { fprintf(stderr, "ERROR: Cannot write %s\n", BOOKINGS_FILE); return; }
    fprintf(f, "%d,%d,%s,%d,%d,%d,%d,%d\n",
            b->booking_id, b->spot_id, b->guest_name,
            b->start_hour, b->start_min,
            b->duration_hours,
            b->end_hour, b->end_min);
    fclose(f);
}

int next_booking_id(void) {
    FILE *f = fopen(BOOKINGS_FILE, "r");
    if (!f) return 1;
    int max_id = 0, id;
    char line[256];
    while (fgets(line, sizeof(line), f))
        if (sscanf(line, "%d,", &id) == 1 && id > max_id)
            max_id = id;
    fclose(f);
    return max_id + 1;
}

/* Caller must free() the returned array. *out_count set to number of rows. */
Booking *load_bookings(int *out_count) {
    *out_count = 0;
    FILE *f = fopen(BOOKINGS_FILE, "r");
    if (!f) return NULL;

    /* Count lines to allocate exactly enough memory */
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) count++;
    rewind(f);

    Booking *arr = malloc(sizeof(Booking) * (count + 1));
    if (!arr) { fclose(f); return NULL; }

    int idx = 0;
    while (fgets(line, sizeof(line), f)) {
        Booking b;
        if (sscanf(line, "%d,%d,%63[^,],%d,%d,%d,%d,%d",
                   &b.booking_id, &b.spot_id, b.guest_name,
                   &b.start_hour, &b.start_min,
                   &b.duration_hours,
                   &b.end_hour, &b.end_min) == 8)
            arr[idx++] = b;
    }
    fclose(f);
    *out_count = idx;
    return arr;
}

/* ================================================================== */
/*  persons.csv lookup                                                  */
/* ================================================================== */

/*
 * CSV format: id, name, age, status, phone, side(int), parking
 * Returns:
 *   0 – not found
 *   1 – found but parking = "No"
 *   2 – found and parking = "Yes"
 */
int check_person_parking(const char *name) {
    FILE *f = fopen(PERSONS_FILE, "r");
    if (!f) return 0;

    char line[512];
    int  id, age, side;
    char pname[100], status[50], phone[20], parking[10];

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9[^,\r\n]",
                   &id, pname, &age, status, phone, &side, parking) != 7)
            continue;

        if (strcasecmp(pname, name) == 0) {
            fclose(f);
            parking[strcspn(parking, " \t\r\n")] = '\0';
            return (strcasecmp(parking, "Yes") == 0) ? 2 : 1;
        }
    }
    fclose(f);
    return 0;
}

/* ================================================================== */
/*  Business logic                                                      */
/* ================================================================== */

int guest_has_existing_booking(const char *name, Booking *out,
                               int req_start, int req_end) {
    (void)req_start; (void)req_end;   /* intentionally unused – see policy note */

    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    for (int i = 0; i < bcount; i++) {
        if (strcasecmp(bookings[i].guest_name, name) != 0) continue;
        if (out) *out = bookings[i];
        free(bookings);
        return 1;
    }
    free(bookings);
    return 0;
}

int find_available_spot(int req_start, int req_end) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    for (int spot = 1; spot <= g_num_spots; spot++) {
        int conflict = 0;
        for (int i = 0; i < bcount; i++) {
            if (bookings[i].spot_id != spot) continue;
            int bs = to_minutes(bookings[i].start_hour, bookings[i].start_min);
            int be = to_minutes(bookings[i].end_hour,   bookings[i].end_min);
            if (intervals_overlap(req_start, req_end, bs, be)) {
                conflict = 1;
                break;
            }
        }
        if (!conflict) { free(bookings); return spot; }
    }
    free(bookings);
    return -1;
}

int is_spot_available(int spot, int req_start, int req_end) {
    if (spot < 1 || spot > g_num_spots) return 0;

    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    for (int i = 0; i < bcount; i++) {
        if (bookings[i].spot_id != spot) continue;
        int bs = to_minutes(bookings[i].start_hour, bookings[i].start_min);
        int be = to_minutes(bookings[i].end_hour,   bookings[i].end_min);
        if (intervals_overlap(req_start, req_end, bs, be)) {
            free(bookings);
            return 0;   /* occupied */
        }
    }
    free(bookings);
    return 1;   /* free */
}

int cancel_booking_by_id(int target_id) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    if (!bookings || bcount == 0) { free(bookings); return 0; }

    /* Verify ID exists */
    int found = 0;
    for (int i = 0; i < bcount; i++)
        if (bookings[i].booking_id == target_id) { found = 1; break; }

    if (!found) { free(bookings); return 0; }

    /* Rewrite file, skipping the cancelled row */
    FILE *f = fopen(BOOKINGS_FILE, "w");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot write %s\n", BOOKINGS_FILE);
        free(bookings);
        return 0;
    }
    for (int i = 0; i < bcount; i++) {
        if (bookings[i].booking_id == target_id) continue;
        fprintf(f, "%d,%d,%s,%d,%d,%d,%d,%d\n",
                bookings[i].booking_id, bookings[i].spot_id,
                bookings[i].guest_name,
                bookings[i].start_hour, bookings[i].start_min,
                bookings[i].duration_hours,
                bookings[i].end_hour,   bookings[i].end_min);
    }
    fclose(f);
    free(bookings);
    return 1;
}

/* ================================================================== */
/*  ADMIN PANEL                                                         */
/* ================================================================== */

/* Display current status of every configured spot */
static void admin_view_status(void) {
    load_spot_config();
    if (g_num_spots == 0) {
        puts("  No spots configured yet. Save settings first.");
        return;
    }

    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    int free_count = 0, occupied_count = 0;

    printf("\n  %-6s  %-10s  %s\n", "Spot", "Status", "Bookings");
    print_rule();

    for (int spot = 1; spot <= g_num_spots; spot++) {
        int has_booking = 0;
        for (int i = 0; i < bcount; i++)
            if (bookings[i].spot_id == spot) { has_booking = 1; break; }

        if (has_booking) {
            occupied_count++;
            printf("  P%-5d  %-10s  ", spot, "OCCUPIED");
            int first = 1;
            for (int i = 0; i < bcount; i++) {
                if (bookings[i].spot_id != spot) continue;
                if (!first) printf("%-20s  ", "");
                printf("%s  %02d:%02d-%02d:%02d  (%dh)\n",
                       bookings[i].guest_name,
                       bookings[i].start_hour, bookings[i].start_min,
                       bookings[i].end_hour,   bookings[i].end_min,
                       bookings[i].duration_hours);
                first = 0;
            }
        } else {
            free_count++;
            printf("  P%-5d  %-10s  Ready to book\n", spot, "FREE");
        }
    }

    print_rule();
    printf("  Free: %d   Occupied: %d\n\n", free_count, occupied_count);
    free(bookings);
}

/* ================================================================== */
/*  GUEST SPOT VIEW                                                     */
/*  Same table as admin_view_status but guest names are hidden.        */
/*  Occupied spots show only the booked time slot(s).                  */
/* ================================================================== */
void guest_view_spots(void) {
    load_spot_config();
    if (g_num_spots == 0) {
        puts("  The parking lot is not configured yet. Please check back later.");
        return;
    }

    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    int free_count = 0, occupied_count = 0;

    printf("\n  %-6s  %-10s  %s\n", "Spot", "Status", "Booked times");
    print_rule();

    for (int spot = 1; spot <= g_num_spots; spot++) {
        int has_booking = 0;
        for (int i = 0; i < bcount; i++)
            if (bookings[i].spot_id == spot) { has_booking = 1; break; }

        if (has_booking) {
            occupied_count++;
            printf("  P%-5d  %-10s  ", spot, "OCCUPIED");
            int first = 1;
            for (int i = 0; i < bcount; i++) {
                if (bookings[i].spot_id != spot) continue;
                /* time slot only — name deliberately omitted */
                if (!first) printf("%-20s  ", "");
                printf("%02d:%02d-%02d:%02d (%dh)\n",
                       bookings[i].start_hour, bookings[i].start_min,
                       bookings[i].end_hour,   bookings[i].end_min,
                       bookings[i].duration_hours);
                first = 0;
            }
        } else {
            free_count++;
            printf("  P%-5d  %-10s  Ready to book\n", spot, "FREE");
        }
    }

    print_rule();
    printf("  Free: %d   Occupied: %d\n\n", free_count, occupied_count);
    free(bookings);
}

/* Save number of spots and max hours */
static void admin_save_settings(void) {
    char buf[32];

    printf("  Number of spots (1-%d): ", MAX_SPOTS);
    fflush(stdout);
    read_line(buf, sizeof(buf));
    int spots = atoi(buf);

    if (spots < 1 || spots > MAX_SPOTS) {
        printf("  ERROR: Spots must be between 1 and %d.\n", MAX_SPOTS);
        return;
    }

    printf("  Maximum hours per booking: ");
    fflush(stdout);
    read_line(buf, sizeof(buf));
    int hours = atoi(buf);

    if (hours <= 0) {
        puts("  ERROR: Maximum hours must be a positive number.");
        return;
    }

    g_num_spots = spots;
    g_max_hours = hours;
    save_spot_config();
    printf("  Settings saved: %d spots, max %d hour(s) per booking.\n",
           g_num_spots, g_max_hours);
}

void run_admin_panel(void) {
    char pw[64];
    printf("  Admin password: ");
    fflush(stdout);
    read_line(pw, sizeof(pw));

    if (strcmp(pw, PASSWORD) != 0) {
        puts("  ERROR: Incorrect password.");
        return;
    }

    load_spot_config();
    puts("  Access granted.");

    int choice;
    do {
        print_title("Admin Panel");
        printf("  Current config: %d spot(s), max %d hour(s)\n\n",
               g_num_spots, g_max_hours);
        puts("  1. Save spot / max-hour settings");
        puts("  2. View parking status");
        puts("  0. Back to main menu");
        print_rule();
        printf("  Choice: ");
        fflush(stdout);

        char buf[8];
        read_line(buf, sizeof(buf));
        choice = atoi(buf);

        switch (choice) {
            case 1: admin_save_settings(); break;
            case 2: admin_view_status();   break;
            case 0: break;
            default: puts("  Invalid choice."); break;
        }
    } while (choice != 0);
}

/* ================================================================== */
/*  GUEST PANEL – view bookings                                         */
/* ================================================================== */

static void guest_view_bookings(const char *name) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    int found = 0;
    printf("\n  Bookings for %s:\n", name);
    print_rule();

    for (int i = 0; i < bcount; i++) {
        if (strcasecmp(bookings[i].guest_name, name) != 0) continue;
        found++;
        printf("  [ID %d]  Spot P%d  |  %02d:%02d - %02d:%02d  |  %d hour(s)\n",
               bookings[i].booking_id,
               bookings[i].spot_id,
               bookings[i].start_hour, bookings[i].start_min,
               bookings[i].end_hour,   bookings[i].end_min,
               bookings[i].duration_hours);
    }

    if (found == 0)
        puts("  No bookings found for your name.");

    print_rule();
    free(bookings);
}

/* ================================================================== */
/*  GUEST PANEL – cancel a booking                                      */
/* ================================================================== */

static void guest_cancel_booking(const char *name) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    /* Collect this guest's booking IDs */
    int ids[MAX_BOOKINGS];
    int count = 0;

    printf("\n  Your active bookings:\n");
    print_rule();

    for (int i = 0; i < bcount; i++) {
        if (strcasecmp(bookings[i].guest_name, name) != 0) continue;
        ids[count++] = bookings[i].booking_id;
        printf("  [ID %d]  Spot P%d  |  %02d:%02d - %02d:%02d  |  %d hour(s)\n",
               bookings[i].booking_id,
               bookings[i].spot_id,
               bookings[i].start_hour, bookings[i].start_min,
               bookings[i].end_hour,   bookings[i].end_min,
               bookings[i].duration_hours);
    }
    free(bookings);

    if (count == 0) {
        puts("  No active bookings to cancel.");
        print_rule();
        return;
    }

    print_rule();
    printf("  Enter booking ID to cancel (0 to go back): ");
    fflush(stdout);

    char buf[16];
    read_line(buf, sizeof(buf));
    int target_id = atoi(buf);

    if (target_id == 0) { puts("  Cancelled."); return; }

    /* Verify the ID belongs to this guest */
    int valid = 0;
    for (int i = 0; i < count; i++)
        if (ids[i] == target_id) { valid = 1; break; }

    if (!valid) {
        puts("  ERROR: That booking ID does not belong to you.");
        return;
    }

    if (cancel_booking_by_id(target_id))
        printf("  Booking %d cancelled. The spot is now free.\n", target_id);
    else
        puts("  ERROR: Could not cancel booking. It may have already been removed.");
}

/* ================================================================== */
/*  GUEST PANEL – make a new booking                                    */
/* ================================================================== */

static void guest_make_booking(const char *name) {
    load_spot_config();

    if (g_num_spots == 0) {
        puts("  The parking lot is not configured yet. Please check back later.");
        return;
    }

    /* One-booking-per-guest gate */
    Booking existing;
    if (guest_has_existing_booking(name, &existing, 0, 0)) {
        printf("  ERROR: You already have a parking booking:\n"
               "    Spot P%d  |  %02d:%02d - %02d:%02d  |  %d hour(s)\n"
               "  Only one booking per guest is allowed.\n"
               "  Please cancel your current booking first.\n",
               existing.spot_id,
               existing.start_hour, existing.start_min,
               existing.end_hour,   existing.end_min,
               existing.duration_hours);
        return;
    }

    char buf[32];
    int sh, sm, dur;

    /* Start hour */
    printf("  Start hour   (0-23): ");
    fflush(stdout);
    read_line(buf, sizeof(buf));
    sh = atoi(buf);

    /* Start minute */
    printf("  Start minute (0-59): ");
    fflush(stdout);
    read_line(buf, sizeof(buf));
    sm = atoi(buf);

    if (sh < 0 || sh > 23 || sm < 0 || sm > 59) {
        puts("  ERROR: Invalid start time. Hour must be 0-23, Minute 0-59.");
        return;
    }

    /* Duration */
    printf("  Duration (hours):    ");
    fflush(stdout);
    read_line(buf, sizeof(buf));
    dur = atoi(buf);

    if (dur <= 0) {
        puts("  ERROR: Duration must be at least 1 hour.");
        return;
    }

    if (g_max_hours > 0 && dur > g_max_hours) {
        printf("  ERROR: Duration (%d h) exceeds the maximum allowed (%d h).\n",
               dur, g_max_hours);
        return;
    }

    int eh, em;
    add_hours(sh, sm, dur, &eh, &em);
    int req_start = to_minutes(sh, sm);
    int req_end   = to_minutes(eh, em);

    /* Show all spots (free and occupied) so the guest can see the full picture.
     * Occupied cards display only booked time slots — no guest names.          */
    printf("\n  Parking lot status for your reference:\n");
    guest_view_spots();

    /* Spot selection loop: keep prompting until the guest picks a valid
     * free spot, or enters 0 to go back.                                  */
    int spot = 0;
    for (;;) {
        printf("  Enter your preferred spot number (1-%d, 0 to cancel): ",
               g_num_spots);
        fflush(stdout);
        read_line(buf, sizeof(buf));
        spot = atoi(buf);

        if (spot == 0) {
            puts("  Booking cancelled.");
            return;
        }

        if (spot < 1 || spot > g_num_spots) {
            printf("  ERROR: Spot %d does not exist. "
                   "Please choose between 1 and %d.\n",
                   spot, g_num_spots);
            continue;
        }

        if (!is_spot_available(spot, req_start, req_end)) {
            printf("  Someone is already occupying Spot P%d "
                   "during %02d:%02d - %02d:%02d.\n"
                   "  Please choose a different spot or a different time.\n",
                   spot, sh, sm, eh, em);
            continue;   /* let the guest try another spot */
        }

        break;   /* valid free spot chosen */
    }

    /* Show preview and ask for confirmation */
    print_rule();
    printf("  Parking Summary\n");
    print_rule();
    printf("  Spot      : P%d\n", spot);
    printf("  From      : %02d:%02d\n", sh, sm);
    printf("  To        : %02d:%02d\n", eh, em);
    printf("  Duration  : %d hour(s)\n", dur);
    print_rule();
    printf("  Confirm booking? (y/n): ");
    fflush(stdout);

    char confirm[4];
    read_line(confirm, sizeof(confirm));

    if (confirm[0] != 'y' && confirm[0] != 'Y') {
        puts("  Booking not saved.");
        return;
    }

    /* Final one-booking-per-guest check before writing to disk */
    if (guest_has_existing_booking(name, &existing, req_start, req_end)) {
        printf("  ERROR: You already have a booking (Spot P%d, %02d:%02d-%02d:%02d).\n"
               "  Cancel it first before making a new one.\n",
               existing.spot_id,
               existing.start_hour, existing.start_min,
               existing.end_hour,   existing.end_min);
        return;
    }

    Booking b;
    b.booking_id     = next_booking_id();
    b.spot_id        = spot;
    b.start_hour     = sh;
    b.start_min      = sm;
    b.duration_hours = dur;
    b.end_hour       = eh;
    b.end_min        = em;
    strncpy(b.guest_name, name, MAX_NAME_LEN - 1);
    b.guest_name[MAX_NAME_LEN - 1] = '\0';

    append_booking(&b);
    printf("  Booking confirmed! Spot P%d reserved for %02d:%02d - %02d:%02d.\n",
           spot, sh, sm, eh, em);
}

/* ================================================================== */
/*  GUEST PANEL – entry point                                           */
/* ================================================================== */

void run_guest_panel(void) {
    print_title("Guest Parking");

    char name[MAX_NAME_LEN];
    printf("  Enter your full name: ");
    fflush(stdout);
    read_line(name, sizeof(name));

    if (strlen(name) == 0) {
        puts("  ERROR: Name cannot be empty.");
        return;
    }

    int status = check_person_parking(name);
    if (status == 0) {
        puts("  ERROR: Name not found in the guest registry.\n"
             "  Please register with the event manager first.");
        return;
    }
    if (status == 1) {
        puts("  ERROR: You are registered but parking was not requested.\n"
             "  Contact the event manager to enable parking.");
        return;
    }

    printf("  Welcome, %s!\n\n", name);

    int choice;
    do {
        puts("  1. Make a booking");
        puts("  2. View my bookings");
        puts("  3. Cancel a booking");
        puts("  0. Back to main menu");
        print_rule();
        printf("  Choice: ");
        fflush(stdout);

        char buf[8];
        read_line(buf, sizeof(buf));
        choice = atoi(buf);

        switch (choice) {
            case 1: guest_make_booking(name);   break;
            case 2: guest_view_bookings(name);  break;
            case 3: guest_cancel_booking(name); break;
            case 0: break;
            default: puts("  Invalid choice."); break;
        }
    } while (choice != 0);
}

/* ================================================================== */
/*  MAIN                                                                */
/* ================================================================== */

int main(void) {
    int choice;
    do {
        print_title("Parking Management System");
        puts("  1. Guest Panel");
        puts("  2. Admin Panel");
        puts("  0. Exit");
        print_rule();
        printf("  Choice: ");
        fflush(stdout);

        char buf[8];
        read_line(buf, sizeof(buf));
        choice = atoi(buf);

        switch (choice) {
            case 1: run_guest_panel(); break;
            case 2: run_admin_panel(); break;
            case 0: puts("  Goodbye."); break;
            default: puts("  Invalid choice."); break;
        }
    } while (choice != 0);

    return 0;
}
