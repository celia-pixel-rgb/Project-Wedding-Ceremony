#include "parking.h"

static void read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) {
        buf[0] = '\0';
        return;
    }

    /* Strip trailing newline if present */
    int len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';

    /* If the line was too long, drain the rest of the input buffer */
    else {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * INTERNAL HELPER  –  case-insensitive string comparison
 *
 * Returns 0 if a and b are equal ignoring case, non-zero otherwise.
 * Portable replacement for strcasecmp (not always available on Windows).
 * ───────────────────────────────────────────────────────────────────────────── */
static int str_iequal(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (*a + 32) : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (*b + 32) : *b;
        if (ca != cb) return 1;
        a++; b++;
    }
    return (*a != '\0' || *b != '\0');   /* 0 only if both strings ended */
}

/* =============================================================================
 * TIME UTILITIES
 * ============================================================================= */

/*
 * to_minutes()
 * Convert hour and minute to total minutes since midnight.
 * Example: 9:30  →  570 minutes
 */
int to_minutes(int h, int m) {
    return h * 60 + m;
}

/*
 * add_hours()
 * Add dur_h whole hours to start time sh:sm.
 * The result wraps around midnight (modulo 24 h = 1440 minutes).
 * Outputs are written to *eh (end hour) and *em (end minute).
 */
void add_hours(int sh, int sm, int dur_h, int *eh, int *em) {
    int total = to_minutes(sh, sm) + dur_h * 60;
    total %= (24 * 60);          /* wrap around midnight */
    *eh = total / 60;
    *em = total % 60;
}

/*
 * intervals_overlap()
 * Determine whether two half-open time intervals [as, ae) and [bs, be) overlap.
 * All values are in minutes since midnight.
 *
 * Midnight wrap-around is handled by extending the interval that crosses
 * midnight by adding 24*60 to its end point before comparing.
 *
 * Returns 1 if they overlap, 0 if they are disjoint.
 */
int intervals_overlap(int as, int ae, int bs, int be) {
    if (ae <= as) ae += 24 * 60;   /* interval A crosses midnight */
    if (be <= bs) be += 24 * 60;   /* interval B crosses midnight */
    return (as < be) && (bs < ae);
}

/* =============================================================================
 * CSV PERSISTENCE  –  parking_spot.csv
 * ============================================================================= */

/*
 * save_spot_config()
 * Write the admin's spot configuration to parking_spot.csv as a single CSV row:
 *   num_spots,max_hours
 * Overwrites any previous configuration.
 */
void save_spot_config(int num_spots, int max_hours) {
    FILE *f = fopen(SPOTS_FILE, "w");
    if (!f) {
        printf("  [ERROR] Cannot write to %s\n", SPOTS_FILE);
        return;
    }
    fprintf(f, "%d,%d\n", num_spots, max_hours);
    fclose(f);
}

/*
 * load_spot_config()
 * Read num_spots and max_hours from parking_spot.csv.
 * Returns 1 on success, 0 if the file is missing or cannot be parsed.
 */
int load_spot_config(int *num_spots, int *max_hours) {
    FILE *f = fopen(SPOTS_FILE, "r");
    if (!f) return 0;
    int r = fscanf(f, "%d,%d", num_spots, max_hours);
    fclose(f);
    return (r == 2) ? 1 : 0;
}

/* =============================================================================
 * CSV PERSISTENCE  –  parking_booking.csv
 * ============================================================================= */

/*
 * next_booking_id()
 * Scan every row in parking_booking.csv and return the highest booking_id + 1.
 * Returns 1 if the file does not exist or contains no valid rows.
 * This ensures booking IDs are always unique and monotonically increasing.
 */
int next_booking_id(void) {
    FILE *f = fopen(BOOKINGS_FILE, "r");
    if (!f) return 1;

    int max_id = 0, id;
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), f))
        if (sscanf(line, "%d,", &id) == 1 && id > max_id)
            max_id = id;

    fclose(f);
    return max_id + 1;
}

/*
 * append_booking()
 * Append one booking record to parking_booking.csv in CSV format:
 *   booking_id, spot_id, guest_name, start_hour, start_min,
 *   duration_hours, end_hour, end_min
 */
void append_booking(const Booking *b) {
    FILE *f = fopen(BOOKINGS_FILE, "a");
    if (!f) {
        printf("  [ERROR] Cannot write to %s\n", BOOKINGS_FILE);
        return;
    }
    fprintf(f, "%d,%d,%s,%d,%d,%d,%d,%d\n",
            b->booking_id, b->spot_id, b->guest_name,
            b->start_hour, b->start_min,
            b->duration_hours,
            b->end_hour, b->end_min);
    fclose(f);
}

/*
 * load_bookings()
 * Allocate and fill an array with every valid booking from parking_booking.csv.
 * *out_count is set to the number of bookings loaded.
 * Returns NULL if the file does not exist or is empty.
 * IMPORTANT: the caller must free() the returned pointer when done.
 */
Booking *load_bookings(int *out_count) {
    *out_count = 0;

    FILE *f = fopen(BOOKINGS_FILE, "r");
    if (!f) return NULL;

    /* First pass: count rows to allocate exactly the right amount of memory */
    int count = 0;
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) count++;
    rewind(f);

    Booking *arr = malloc(sizeof(Booking) * (count + 1));
    if (!arr) { fclose(f); return NULL; }

    /* Second pass: parse each row into the array */
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

/*
 * cancel_booking_by_id()
 * Rewrite parking_booking.csv, omitting the row whose booking_id matches
 * the given target_id.  All other rows are preserved exactly as-is.
 * Returns 1 if the booking was found and removed, 0 if it was not found.
 */
int cancel_booking_by_id(int target_id) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    if (!bookings || bcount == 0) { free(bookings); return 0; }

    /* Check if the target ID actually exists before touching the file */
    int found = 0;
    int i;
    for (i = 0; i < bcount; i++)
        if (bookings[i].booking_id == target_id) { found = 1; break; }

    if (!found) { free(bookings); return 0; }

    /* Rewrite file keeping every row except the cancelled one */
    FILE *f = fopen(BOOKINGS_FILE, "w");
    if (!f) {
        printf("  [ERROR] Cannot write to %s\n", BOOKINGS_FILE);
        free(bookings);
        return 0;
    }
    for (i = 0; i < bcount; i++) {
        if (bookings[i].booking_id == target_id)
            continue;   /* skip the cancelled row */
        fprintf(f, "%d,%d,%s,%d,%d,%d,%d,%d\n",
                bookings[i].booking_id, bookings[i].spot_id,
                bookings[i].guest_name,
                bookings[i].start_hour, bookings[i].start_min,
                bookings[i].duration_hours,
                bookings[i].end_hour, bookings[i].end_min);
    }
    fclose(f);
    free(bookings);
    return 1;
}

/*
 * cancel_bookings_for_guest()
 * Rewrite parking_booking.csv, omitting every row whose guest_name matches
 * the given name (case-insensitive).  Used when a guest's parking is revoked
 * from the person module (parking changed "Yes" → "No").
 * Returns the number of rows removed.
 */
int cancel_bookings_for_guest(const char *guest_name) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    if (!bookings || bcount == 0) { free(bookings); return 0; }

    /* Count how many rows belong to this guest */
    int removed = 0;
    int i;
    for (i = 0; i < bcount; i++)
        if (str_iequal(bookings[i].guest_name, guest_name) == 0)
            removed++;

    if (removed == 0) { free(bookings); return 0; }

    /* Rewrite file keeping only rows for other guests */
    FILE *f = fopen(BOOKINGS_FILE, "w");
    if (!f) {
        printf("  [ERROR] Cannot write to %s\n", BOOKINGS_FILE);
        free(bookings);
        return 0;
    }
    for (i = 0; i < bcount; i++) {
        if (str_iequal(bookings[i].guest_name, guest_name) == 0)
            continue;   /* skip – this guest's booking is being cancelled */
        fprintf(f, "%d,%d,%s,%d,%d,%d,%d,%d\n",
                bookings[i].booking_id, bookings[i].spot_id,
                bookings[i].guest_name,
                bookings[i].start_hour, bookings[i].start_min,
                bookings[i].duration_hours,
                bookings[i].end_hour, bookings[i].end_min);
    }
    fclose(f);
    free(bookings);
    return removed;
}

/* =============================================================================
 * persons.csv LOOKUP
 * ============================================================================= */

/*
 * check_person_parking()
 * Search persons.csv for a guest whose name matches 'name' (case-insensitive).
 *
 * CSV format (written by gtkperson.c):
 *   id, name, age, status, phone, side, parking, email
 *
 * Returns:
 *   PERSON_NOT_FOUND   (0) – name not in the file
 *   PERSON_NO_PARKING  (1) – found but parking field = "No"
 *   PERSON_HAS_PARKING (2) – found and parking field = "Yes"
 */
int check_person_parking(const char *name) {
    FILE *f = fopen(PERSONS_FILE, "r");
    if (!f) return PERSON_NOT_FOUND;   /* no registry file at all */

    char line[MAX_LINE_LEN];
    int  id, age, side;
    char pname[100], status[50], phone[20], parking[10];

    while (fgets(line, sizeof(line), f)) {
        /* Parse at least 7 fields; an optional email field after parking is ignored */
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9[^,\r\n]",
                   &id, pname, &age, status, phone, &side, parking) != 7)
            continue;

        if (str_iequal(pname, name) == 0) {
            fclose(f);
            /* Strip any trailing whitespace or newline from the parking field */
            int plen = (int)strlen(parking);
            while (plen > 0 && (parking[plen-1] == ' '  ||
                                 parking[plen-1] == '\t' ||
                                 parking[plen-1] == '\r' ||
                                 parking[plen-1] == '\n'))
                parking[--plen] = '\0';

            return (str_iequal(parking, "Yes") == 0)
                   ? PERSON_HAS_PARKING
                   : PERSON_NO_PARKING;
        }
    }

    fclose(f);
    return PERSON_NOT_FOUND;
}

/* =============================================================================
 * AVAILABILITY LOGIC
 * ============================================================================= */

/*
 * find_available_spot()
 * Iterate through spots 1 … num_spots and return the first one whose
 * existing bookings do not conflict with the requested window
 * [req_start, req_end) (both in minutes since midnight).
 *
 * Returns the spot number (1-based) if found, -1 if all spots are busy.
 */
int find_available_spot(int req_start, int req_end, int num_spots) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
int spot;
    for (spot = 1; spot <= num_spots; spot++) {
        int conflict = 0;
int i;
        for (i = 0; i < bcount; i++) {
            if (bookings[i].spot_id != spot) continue;

            int bs = to_minutes(bookings[i].start_hour, bookings[i].start_min);
            int be = to_minutes(bookings[i].end_hour,   bookings[i].end_min);

            if (intervals_overlap(req_start, req_end, bs, be)) {
                conflict = 1;
                break;
            }
        }

        if (!conflict) {
            free(bookings);
            return spot;   /* first free spot found */
        }
    }

    free(bookings);
    return -1;   /* every spot has a conflicting booking */
}

/*
 * guest_has_overlapping_booking()
 * Return 1 if the guest named 'name' already has any booking that overlaps
 * the requested window [req_start, req_end) (minutes since midnight).
 *
 * If a conflict is found and out != NULL, *out is filled with the conflicting
 * booking so the caller can show details to the user.
 *
 * A guest may hold multiple non-overlapping bookings simultaneously;
 * this function only blocks if the new window clashes with an existing one.
 */
int guest_has_overlapping_booking(const char *name, Booking *out,
                                  int req_start, int req_end) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
int i;
    for (i = 0; i < bcount; i++) {
        if (str_iequal(bookings[i].guest_name, name) != 0) continue;

        int bs = to_minutes(bookings[i].start_hour, bookings[i].start_min);
        int be = to_minutes(bookings[i].end_hour,   bookings[i].end_min);

        if (intervals_overlap(req_start, req_end, bs, be)) {
            if (out) *out = bookings[i];
            free(bookings);
            return 1;
        }
    }

    free(bookings);
    return 0;
}

/*
 * guest_has_any_booking()
 * Return 1 if the guest named 'name' already has ANY booking in the system,
 * regardless of time window.  Enforces the one-booking-per-guest rule.
 *
 * If a booking is found and out != NULL, *out is filled with that booking
 * so the caller can display it to the user.
 */
int guest_has_any_booking(const char *name, Booking *out) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    int i;
    for (i = 0; i < bcount; i++) {
        if (str_iequal(bookings[i].guest_name, name) == 0) {
            if (out) *out = bookings[i];
            free(bookings);
            return 1;
        }
    }
    free(bookings);
    return 0;
}

/* =============================================================================
 * DISPLAY HELPERS
 * ============================================================================= */

/*
 * print_separator()
 * Print a horizontal divider line for visual clarity in the terminal.
 */
void print_separator(void) {
    printf("  -------------------------------------------------------\n");
}

/*
 * print_all_spots()
 * Display the current occupation status of every configured spot.
 * For each spot, all bookings (guest name + time window) are listed.
 * Spots with no bookings are shown as AVAILABLE.
 */
void print_all_spots(int num_spots) {
    if (num_spots == 0) {
        printf("  [!] No spots configured yet. Use Admin > Configure to set them up.\n");
        return;
    }

    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    int free_count = 0, occupied_count = 0;

    print_separator();
    printf("  %-6s  %-12s  %-20s  %s\n",
           "SPOT", "STATUS", "GUEST", "TIME WINDOW");
    print_separator();
int spot;
    for (spot = 1; spot <= num_spots; spot++) {
        int has_booking = 0;

        /* Print every booking that occupies this spot */
        int i;
        for (i = 0; i < bcount; i++) {
            if (bookings[i].spot_id != spot) continue;

            if (!has_booking) {
                /* First booking for this spot – print spot number */
                printf("  P%-5d  %-12s  %-20s  %02d:%02d - %02d:%02d  (%dh)\n",
                       spot,
                       "OCCUPIED",
                       bookings[i].guest_name,
                       bookings[i].start_hour, bookings[i].start_min,
                       bookings[i].end_hour,   bookings[i].end_min,
                       bookings[i].duration_hours);
                has_booking = 1;
            } else {
                /* Subsequent bookings on the same spot – indent, no spot number */
                printf("  %-7s  %-12s  %-20s  %02d:%02d - %02d:%02d  (%dh)\n",
                       "",
                       "",
                       bookings[i].guest_name,
                       bookings[i].start_hour, bookings[i].start_min,
                       bookings[i].end_hour,   bookings[i].end_min,
                       bookings[i].duration_hours);
            }
        }

        if (!has_booking) {
            printf("  P%-5d  %-12s\n", spot, "AVAILABLE");
            free_count++;
        } else {
            occupied_count++;
        }
    }

    print_separator();
    printf("  Free: %d   |   Occupied: %d   |   Total: %d\n",
           free_count, occupied_count, num_spots);
    print_separator();

    free(bookings);
}

/*
 * print_guest_bookings()
 * List all bookings belonging to guest_name with their booking IDs,
 * spot numbers, and time windows.  Shows a message if none exist.
 */
void print_guest_bookings(const char *guest_name) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    int found = 0;
    print_separator();
    printf("  Bookings for: %s\n", guest_name);
    print_separator();
    printf("  %-6s  %-6s  %-20s\n", "ID", "SPOT", "TIME WINDOW");
    print_separator();

int i;
    for (i = 0; i < bcount; i++) {
        if (str_iequal(bookings[i].guest_name, guest_name) != 0) continue;
        found++;
        printf("  %-6d  P%-5d  %02d:%02d - %02d:%02d  (%d hour(s))\n",
               bookings[i].booking_id,
               bookings[i].spot_id,
               bookings[i].start_hour, bookings[i].start_min,
               bookings[i].end_hour,   bookings[i].end_min,
               bookings[i].duration_hours);
    }

    if (found == 0)
        printf("  No bookings found for your name.\n");

    print_separator();
    free(bookings);
}

/* =============================================================================
 * GUEST MENU
 * ============================================================================= */

/*
 * run_guest_menu()
 * Interactive menu for a verified guest (parking = "Yes" confirmed before entry).
 *
 * Options:
 *   1. Book a parking slot  – check availability then confirm
 *   2. View my bookings     – list all personal reservations
 *   3. Cancel a booking     – remove one reservation by ID
 *   4. Back / Exit
 */
void run_guest_menu(const char *guest_name, int num_spots, int max_hours) {
    char input[64];
    int  choice;

    printf("\n  Welcome, %s!\n", guest_name);

    while (1) {
        printf("\n");
        print_separator();
        printf("  GUEST MENU  –  %s\n", guest_name);
        print_separator();
        printf("  1. Book a parking slot\n");
        printf("  2. View my bookings\n");
        printf("  3. Cancel a booking\n");
        printf("  4. Back\n");
        print_separator();
        printf("  Choice: ");
        read_line(input, sizeof(input));
        choice = atoi(input);

        /* ── Option 1: Book a parking slot ─────────────────────────────── */
        if (choice == 1) {
            /* Reload config in case admin changed it since login */
            load_spot_config(&num_spots, &max_hours);

            if (num_spots == 0) {
                printf("  [!] The parking lot is not configured yet.\n"
                       "      Please check back later.\n");
                continue;
            }

            /* ---- Collect start time ---- */
            int sh, sm, dur;

            printf("  Start hour   (0-23): ");
            read_line(input, sizeof(input));
            sh = atoi(input);
            if (sh < 0 || sh > 23) {
                printf("  [!] Invalid hour. Must be 0–23.\n");
                continue;
            }

            printf("  Start minute (0-59): ");
            read_line(input, sizeof(input));
            sm = atoi(input);
            if (sm < 0 || sm > 59) {
                printf("  [!] Invalid minute. Must be 0–59.\n");
                continue;
            }

            printf("  Duration (hours):    ");
            read_line(input, sizeof(input));
            dur = atoi(input);
            if (dur <= 0) {
                printf("  [!] Duration must be at least 1 hour.\n");
                continue;
            }

            /* ---- Enforce max-hours limit ---- */
            if (max_hours > 0 && dur > max_hours) {
                printf("  [!] Duration (%d h) exceeds the maximum allowed (%d h).\n",
                       dur, max_hours);
                continue;
            }

            /* ---- Compute end time ---- */
            int eh, em;
            add_hours(sh, sm, dur, &eh, &em);
            int req_start = to_minutes(sh, sm);
            int req_end   = to_minutes(eh, em);

            /* ---- Enforce one-booking-per-guest rule ---- */
            Booking conflict;
            if (guest_has_any_booking(guest_name, &conflict)) {
                printf("  [!] You already have an active booking:\n");
                printf("      Booking #%d  |  Spot P%d  |  %02d:%02d - %02d:%02d\n",
                       conflict.booking_id,
                       conflict.spot_id,
                       conflict.start_hour, conflict.start_min,
                       conflict.end_hour,   conflict.end_min);
                printf("      Please cancel your existing booking before making a new one.\n");
                continue;
            }

            /* ---- Find a free spot ---- */
            int spot = find_available_spot(req_start, req_end, num_spots);
            if (spot == -1) {
                printf("  [!] No spot is available for %02d:%02d - %02d:%02d.\n",
                       sh, sm, eh, em);
                printf("      Please try a different time window.\n");
                continue;
            }

            /* ---- Show booking summary and ask for confirmation ---- */
            printf("\n");
            print_separator();
            printf("  BOOKING SUMMARY\n");
            print_separator();
            printf("  Spot      :  P%d\n", spot);
            printf("  From      :  %02d:%02d\n", sh, sm);
            printf("  To        :  %02d:%02d\n", eh, em);
            printf("  Duration  :  %d hour(s)\n", dur);
            print_separator();
            printf("  Confirm booking? (y/n): ");
            read_line(input, sizeof(input));

            if (input[0] != 'y' && input[0] != 'Y') {
                printf("  Booking cancelled. No changes made.\n");
                continue;
            }

            /* ---- Save the booking ---- */
            Booking b;
            b.booking_id    = next_booking_id();
            b.spot_id       = spot;
            b.start_hour    = sh;
            b.start_min     = sm;
            b.duration_hours = dur;
            b.end_hour      = eh;
            b.end_min       = em;
            strncpy(b.guest_name, guest_name, MAX_NAME_LEN - 1);
            b.guest_name[MAX_NAME_LEN - 1] = '\0';

            append_booking(&b);
            printf("  [OK] Booking confirmed! Spot P%d reserved from %02d:%02d to %02d:%02d.\n",
                   spot, sh, sm, eh, em);
        }

        /* ── Option 2: View my bookings ─────────────────────────────────── */
        else if (choice == 2) {
            print_guest_bookings(guest_name);
        }

        /* ── Option 3: Cancel a booking ──────────────────────────────────── */
        else if (choice == 3) {
            /* First show the guest's bookings so they can pick an ID */
            print_guest_bookings(guest_name);

            int bcount = 0;
            Booking *bookings = load_bookings(&bcount);
            int has_any = 0;
            int i;
            for (i = 0; i < bcount; i++)
                if (str_iequal(bookings[i].guest_name, guest_name) == 0)
                    { has_any = 1; break; }
            free(bookings);

            if (!has_any) {
                printf("  No bookings to cancel.\n");
                continue;
            }

            printf("  Enter booking ID to cancel (0 to go back): ");
            read_line(input, sizeof(input));
            int target_id = atoi(input);

            if (target_id == 0) continue;

            /* Verify the ID belongs to this guest before cancelling */
            int bcount2 = 0;
            Booking *bookings2 = load_bookings(&bcount2);
            int owner = 0;
            for (i = 0; i < bcount2; i++) {
                if (bookings2[i].booking_id == target_id &&
                    str_iequal(bookings2[i].guest_name, guest_name) == 0) {
                    owner = 1;
                    break;
                }
            }
            free(bookings2);

            if (!owner) {
                printf("  [!] Booking ID %d not found for your name.\n", target_id);
                continue;
            }

            if (cancel_booking_by_id(target_id))
                printf("  [OK] Booking #%d cancelled. The spot is now free.\n",
                       target_id);
            else
                printf("  [!] Could not cancel booking #%d.\n", target_id);
        }

        /* ── Option 4: Back ──────────────────────────────────────────────── */
        else if (choice == 4) {
            break;
        }

        else {
            printf("  [!] Invalid choice. Please enter 1, 2, 3, or 4.\n");
        }
    }
}

/* =============================================================================
 * ADMIN MENU
 * ============================================================================= */

/*
 * run_admin_menu()
 * Password-protected interactive menu for the parking administrator.
 *
 * Options:
 *   1. Configure spots    – set number of spots and maximum hours
 *   2. View spot status   – show all spots and their current bookings
 *   3. Cancel any booking – remove a booking by its ID (admin override)
 *   4. Exit admin menu
 */
void run_admin_menu(void) {
    char input[64];

    /* ---- Password verification ---- */
    printf("  Admin password: ");
    read_line(input, sizeof(input));
    if (strcmp(input, PASSWORD) != 0) {
        printf("  [!] Incorrect password. Access denied.\n");
        return;
    }
    printf("  [OK] Access granted.\n");

    /* Load current configuration */
    int num_spots = 0, max_hours = 0;
    load_spot_config(&num_spots, &max_hours);

    int choice;

    while (1) {
        printf("\n");
        print_separator();
        printf("  ADMIN MENU\n");
        if (num_spots > 0)
            printf("  Config: %d spots  |  max %d hour(s) per booking\n",
                   num_spots, max_hours);
        else
            printf("  Config: not set yet\n");
        print_separator();
        printf("  1. Configure spots\n");
        printf("  2. View spot status\n");
        printf("  3. Cancel any booking by ID\n");
        printf("  4. Back\n");
        print_separator();
        printf("  Choice: ");
        read_line(input, sizeof(input));
        choice = atoi(input);

        /* ── Option 1: Configure ─────────────────────────────────────────── */
        if (choice == 1) {
            printf("  Number of parking spots (1-%d): ", MAX_SPOTS);
            read_line(input, sizeof(input));
            int spots = atoi(input);
            if (spots <= 0 || spots > MAX_SPOTS) {
                printf("  [!] Must be between 1 and %d.\n", MAX_SPOTS);
                continue;
            }

            printf("  Maximum hours per booking: ");
            read_line(input, sizeof(input));
            int hours = atoi(input);
            if (hours <= 0) {
                printf("  [!] Must be a positive number.\n");
                continue;
            }

            num_spots = spots;
            max_hours = hours;
            save_spot_config(num_spots, max_hours);
            printf("  [OK] Saved: %d spots, max %d hour(s).\n",
                   num_spots, max_hours);
        }

        /* ── Option 2: View spot status ─────────────────────────────────── */
        else if (choice == 2) {
            print_all_spots(num_spots);
        }

        /* ── Option 3: Cancel any booking ───────────────────────────────── */
        else if (choice == 3) {
            /* Show all bookings across all guests so admin can pick an ID */
            int bcount = 0;
            Booking *bookings = load_bookings(&bcount);

            if (bcount == 0) {
                printf("  No bookings currently in the system.\n");
                free(bookings);
                continue;
            }

            print_separator();
            printf("  %-6s  %-6s  %-20s  %s\n",
                   "ID", "SPOT", "GUEST", "TIME WINDOW");
            print_separator();
            int i;
            for (i = 0; i < bcount; i++) {
                printf("  %-6d  P%-5d  %-20s  %02d:%02d - %02d:%02d  (%dh)\n",
                       bookings[i].booking_id,
                       bookings[i].spot_id,
                       bookings[i].guest_name,
                       bookings[i].start_hour, bookings[i].start_min,
                       bookings[i].end_hour,   bookings[i].end_min,
                       bookings[i].duration_hours);
            }
            print_separator();
            free(bookings);

            printf("  Enter booking ID to cancel (0 to go back): ");
            read_line(input, sizeof(input));
            int target_id = atoi(input);
            if (target_id == 0) continue;

            if (cancel_booking_by_id(target_id))
                printf("  [OK] Booking #%d cancelled. Spot is now free.\n",
                       target_id);
            else
                printf("  [!] Booking ID %d not found.\n", target_id);
        }

        /* ── Option 4: Back ──────────────────────────────────────────────── */
        else if (choice == 4) {
            break;
        }

        else {
            printf("  [!] Invalid choice. Please enter 1, 2, 3, or 4.\n");
        }
    }
}

/* =============================================================================
 * MAIN  –  top-level menu
 * ============================================================================= */

int main(void) {
    char input[MAX_NAME_LEN];
    int  choice;

    printf("\n");
    print_separator();
    printf("  PARKING MANAGEMENT SYSTEM\n");
    print_separator();

    while (1) {
        printf("\n");
        print_separator();
        printf("  MAIN MENU\n");
        print_separator();
        printf("  1. Admin panel\n");
        printf("  2. Guest panel\n");
        printf("  3. Exit\n");
        print_separator();
        printf("  Choice: ");
        read_line(input, sizeof(input));
        choice = atoi(input);

        /* ── Admin panel ─────────────────────────────────────────────────── */
        if (choice == 1) {
            run_admin_menu();
        }

        /* ── Guest panel ─────────────────────────────────────────────────── */
        else if (choice == 2) {
            printf("  Enter your full name: ");
            read_line(input, sizeof(input));

            if (!strlen(input)) {
                printf("  [!] Name cannot be empty.\n");
                continue;
            }

            /* Verify name and parking status against persons.csv */
            int status = check_person_parking(input);

            if (status == PERSON_NOT_FOUND) {
                printf("  [!] Name not found in the guest registry.\n"
                       "      Please register with the event manager first.\n");
                continue;
            }
            if (status == PERSON_NO_PARKING) {
                printf("  [!] You are registered but parking was not requested.\n"
                       "      Contact the event manager to enable parking.\n");
                continue;
            }

            /* status == PERSON_HAS_PARKING – proceed to guest menu */
            int num_spots = 0, max_hours = 0;
            load_spot_config(&num_spots, &max_hours);
            run_guest_menu(input, num_spots, max_hours);
        }

        /* ── Exit ────────────────────────────────────────────────────────── */
        else if (choice == 3) {
            printf("  Goodbye!\n\n");
            break;
        }

        else {
            printf("  [!] Invalid choice. Please enter 1, 2, or 3.\n");
        }
    }

    return 0;
}
