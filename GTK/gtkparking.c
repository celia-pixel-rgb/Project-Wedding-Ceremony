/* parking.c
 * Parking Management System
 * Follows the same GTK4 patterns as gtkperson-1.c
 *
 * Two panels:
 *   Admin  – password-protected: set spots / max hours / view status
 *   Guest  – open: register a parking booking
 *
 * Storage: parking_spots.csv   (spot configurations set by admin)
 *          parking_bookings.csv (guest reservations)
 *
 * Overlap logic: many guests may share one spot as long as their
 * time ranges do not overlap.  If every spot is taken for the
 * requested window, the guest is notified.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/*  Constants                                                           */
/* ------------------------------------------------------------------ */
static const char *PASSWORD          = "group3wed!";
static const char *SPOTS_FILE        = "parking_spot.csv";
static const char *BOOKINGS_FILE     = "parking_booking.csv";
static const char *PERSONS_FILE      = "persons.csv";

#define MAX_SPOTS    200   /* hard upper limit */
#define MAX_NAME_LEN  64

/* ------------------------------------------------------------------ */
/*  Data types                                                          */
/* ------------------------------------------------------------------ */
typedef struct {
    int  spot_id;          /* 1-based */
    int  max_hours;        /* maximum allowed hours for this spot     */
} SpotConfig;

typedef struct {
    int  booking_id;
    int  spot_id;
    char guest_name[MAX_NAME_LEN];
    int  start_hour;       /* 0-23 */
    int  start_min;        /* 0-59 */
    int  duration_hours;
    int  end_hour;
    int  end_min;
} Booking;

/* ------------------------------------------------------------------ */
/*  Forward declarations                                               */
/* ------------------------------------------------------------------ */
static int       to_minutes(int h, int m);
static void      add_hours(int sh, int sm, int dur_h, int *eh, int *em);
static gboolean  intervals_overlap(int as, int ae, int bs, int be);
static void      save_spot_config(void);
static gboolean  load_spot_config(void);
static void      append_booking(const Booking *b);
static int       next_booking_id(void);
static Booking  *load_bookings(int *out_count);
static int       check_person_parking(const char *name);
static gboolean  guest_has_existing_booking(const char *name, Booking *out,
                                            int req_start, int req_end);
static int       find_available_spot(int req_start, int req_end);
static gboolean  is_spot_available(int spot, int req_start, int req_end);
static gboolean  admin_auto_refresh(gpointer data);
static gboolean  cancel_booking_by_id(int target_id);
static void      on_confirm_cancel_booking(GtkButton *btn, gpointer id_ptr);
static void      on_guest_cancel(GtkButton *btn, gpointer data);

/* ------------------------------------------------------------------ */
/*  Global admin settings (loaded from / saved to SPOTS_FILE)          */
/* ------------------------------------------------------------------ */
static int g_num_spots    = 0;
static int g_max_hours    = 0;

/* ------------------------------------------------------------------ */
/*  GTK4 dialog helper (replaces deprecated gtk_message_dialog_new)   */
/* ------------------------------------------------------------------ */
static void show_dialog(const char *title, const char *message) {
    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), title);
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 320, -1);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *lbl = gtk_label_new(message);
    gtk_label_set_wrap(GTK_LABEL(lbl), TRUE);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_box_append(GTK_BOX(box), lbl);

    GtkWidget *ok_btn = gtk_button_new_with_label("OK");
    gtk_widget_set_halign(ok_btn, GTK_ALIGN_END);
    g_signal_connect_swapped(ok_btn, "clicked", G_CALLBACK(gtk_window_destroy), win);
    gtk_box_append(GTK_BOX(box), ok_btn);

    gtk_window_set_child(GTK_WINDOW(win), box);
    gtk_window_present(GTK_WINDOW(win));
}

/* ------------------------------------------------------------------ */
/*  Widget handles – Admin panel                                        */
/* ------------------------------------------------------------------ */
static GtkWidget *admin_password_entry;
static GtkWidget *admin_spots_entry;
static GtkWidget *admin_maxhours_entry;
static GtkWidget *admin_settings_box;      /* shown after correct pw  */
static GtkWidget *admin_status_grid;       /* spot-card flow box      */
static GtkWidget *admin_free_label;
static GtkWidget *admin_occupied_label;

/* ------------------------------------------------------------------ */
/*  Widget handles – Navigation stack (home / admin / guest)           */
/* ------------------------------------------------------------------ */
static GtkWidget *g_main_stack;   /* the top-level GtkStack           */

/* ------------------------------------------------------------------ */
/*  Widget handles – Guest panel                                        */
/* ------------------------------------------------------------------ */
static GtkWidget *guest_name_entry;        /* login screen name entry  */
static GtkWidget *guest_login_box;         /* login phase container    */
static GtkWidget *guest_login_error;       /* login error label        */
static GtkWidget *guest_lookup_label;      /* booking lookup result    */
static GtkWidget *guest_cancel_label;      /* cancel result label      */
static GtkWidget *guest_cancel_list_box;   /* box listing bookings to cancel */

static GtkWidget *guest_booking_box;       /* booking phase container  */
static GtkWidget *guest_locked_name_label; /* shows verified name      */
static GtkWidget *guest_start_hour_entry;
static GtkWidget *guest_start_min_entry;
static GtkWidget *guest_duration_entry;
static GtkWidget *guest_spot_entry;        /* guest-chosen spot number  */
static GtkWidget *guest_preview_box;       /* shown after "Check"      */
static GtkWidget *guest_preview_label;     /* spot / from / to / dur   */
static GtkWidget *guest_confirm_btn;       /* final confirm button     */
static GtkWidget *guest_result_label;      /* post-confirm message     */
static GtkWidget *guest_spot_grid;         /* flow-box of spot cards (guest view) */
static GtkWidget *guest_spot_grid_box;     /* wrapper shown/hidden around grid    */
static GtkWidget *guest_free_label;        /* 🟢 Free: N counter       */
static GtkWidget *guest_occupied_label;    /* 🔴 Occupied: N counter   */

/* Pending booking filled by on_guest_check, consumed by on_guest_confirm */
static Booking   g_pending_booking;
static gboolean  g_has_pending = FALSE;

/* ===================================================================
 *  GUEST PANEL CALLBACKS
 * =================================================================== */

/*
 * ONE-BOOKING-PER-GUEST RULE
 * ==========================
 * Policy: each guest is permitted AT MOST ONE active parking booking at any
 * time.  It does not matter whether a prospective new booking would overlap
 * the existing one in time — the guest simply cannot hold more than one
 * reservation simultaneously.
 *
 * Returns TRUE if 'name' already has ANY booking recorded in the bookings
 * CSV, regardless of time window.  When TRUE is returned, *out (if non-NULL)
 * is filled with that existing booking so the caller can display its details
 * in the error message shown to the guest.
 *
 * The parameters req_start and req_end (the requested window in minutes since
 * midnight) are intentionally IGNORED here.  They are retained in the
 * signature only so that call sites do not need to be changed; passing them
 * keeps the rest of the code untouched.
 *
 * Previous behaviour (now superseded):
 *   The function used to check only for time-window OVERLAP, allowing a guest
 *   to accumulate multiple bookings as long as none of them clashed in time.
 *   The new policy is stricter: one guest, one spot, period.
 */
static gboolean guest_has_existing_booking(const char *name, Booking *out,
                                           int req_start, int req_end) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    /* Iterate every booking in the CSV.
     * As soon as we find ANY row belonging to this guest we return TRUE —
     * no time-overlap check is performed (see policy note above).          */
    for (int i = 0; i < bcount; i++) {

        /* Skip rows that belong to a different guest (case-insensitive). */
        if (g_ascii_strcasecmp(bookings[i].guest_name, name) != 0) continue;

        /* Found an existing booking for this guest.
         * Copy it into *out so the caller can show the guest which booking
         * is blocking the new request, then bail out immediately.          */
        if (out) *out = bookings[i];
        free(bookings);
        return TRUE;   /* guest already has a booking → deny new request   */
    }

    /* No booking found for this guest — they are allowed to book. */
    free(bookings);
    return FALSE;
}

/* Lookup – show existing bookings for a verified guest, stay on login screen */
static void on_guest_lookup(GtkButton *btn, gpointer data) {
    gtk_label_set_text(GTK_LABEL(guest_login_error), "");
    gtk_label_set_text(GTK_LABEL(guest_lookup_label), "");
    gtk_label_set_text(GTK_LABEL(guest_cancel_label), "");

    const char *name = gtk_editable_get_text(GTK_EDITABLE(guest_name_entry));
    if (!strlen(name)) {
        gtk_label_set_text(GTK_LABEL(guest_lookup_label),
            "⚠  Please enter your name first.");
        return;
    }

    /* Must still be a registered parking guest */
    int status = check_person_parking(name);
    if (status == 0) {
        gtk_label_set_text(GTK_LABEL(guest_lookup_label),
            "⚠  Name not found in the guest registry.");
        return;
    }
    if (status == 1) {
        gtk_label_set_text(GTK_LABEL(guest_lookup_label),
            "⚠  You do not have parking enabled in the registry.");
        return;
    }

    /* Scan bookings CSV for this guest */
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    GString *result = g_string_new(NULL);
    int found = 0;

    for (int i = 0; i < bcount; i++) {
        if (g_ascii_strcasecmp(bookings[i].guest_name, name) != 0) continue;
        found++;
        g_string_append_printf(result,
            "🅿  Spot %d   |   🕐 %02d:%02d – %02d:%02d   |   ⏱ %d hour(s)\n",
            bookings[i].spot_id,
            bookings[i].start_hour, bookings[i].start_min,
            bookings[i].end_hour,   bookings[i].end_min,
            bookings[i].duration_hours);
    }
    free(bookings);

    if (found == 0) {
        gtk_label_set_text(GTK_LABEL(guest_lookup_label),
            "ℹ  No bookings found for your name.");
    } else {
        /* Prepend a header */
        GString *full = g_string_new(NULL);
        g_string_append_printf(full, "📋  Bookings for %s:\n\n", name);
        g_string_append(full, result->str);
        gtk_label_set_text(GTK_LABEL(guest_lookup_label), full->str);
        g_string_free(full, TRUE);
    }
    g_string_free(result, TRUE);
}

/* ===================================================================
 *  CANCEL BOOKING – select and remove a specific booking by ID
 * =================================================================== */

/*
 * Remove one booking row (matching booking_id) from the CSV.
 * All other rows are preserved unchanged.
 * Returns TRUE on success, FALSE if the ID was not found or write failed.
 */
static gboolean cancel_booking_by_id(int target_id) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    if (!bookings || bcount == 0) { free(bookings); return FALSE; }

    /* Verify the ID actually exists */
    gboolean found = FALSE;
    for (int i = 0; i < bcount; i++)
        if (bookings[i].booking_id == target_id) { found = TRUE; break; }

    if (!found) { free(bookings); return FALSE; }

    /* Rewrite file, skipping only the target row */
    FILE *f = fopen(BOOKINGS_FILE, "w");
    if (!f) {
        g_print("Cannot write %s\n", BOOKINGS_FILE);
        free(bookings);
        return FALSE;
    }
    for (int i = 0; i < bcount; i++) {
        if (bookings[i].booking_id == target_id)
            continue;   /* this is the row being cancelled */
        fprintf(f, "%d,%d,%s,%d,%d,%d,%d,%d\n",
                bookings[i].booking_id, bookings[i].spot_id,
                bookings[i].guest_name,
                bookings[i].start_hour, bookings[i].start_min,
                bookings[i].duration_hours,
                bookings[i].end_hour,   bookings[i].end_min);
    }
    fclose(f);
    free(bookings);
    return TRUE;
}

/*
 * Callback attached to each per-booking "Cancel this" button.
 * The booking_id is passed as an integer cast to gpointer.
 */
static void on_confirm_cancel_booking(GtkButton *btn, gpointer id_ptr) {
    int booking_id = GPOINTER_TO_INT(id_ptr);

    gtk_label_set_text(GTK_LABEL(guest_cancel_label), "");

    if (cancel_booking_by_id(booking_id)) {
        gtk_label_set_text(GTK_LABEL(guest_cancel_label),
            "✅  Booking cancelled. The spot is now free for other guests.");
    } else {
        gtk_label_set_text(GTK_LABEL(guest_cancel_label),
            "⚠  Could not cancel booking. It may have already been removed.");
    }

    /* Rebuild the booking list to reflect the removal */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(guest_cancel_list_box)) != NULL)
        gtk_box_remove(GTK_BOX(guest_cancel_list_box), child);

    /* Re-read remaining bookings for this guest and re-populate */
    const char *name = gtk_editable_get_text(GTK_EDITABLE(guest_name_entry));
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    int found = 0;
    for (int i = 0; i < bcount; i++) {
        if (g_ascii_strcasecmp(bookings[i].guest_name, name) != 0) continue;
        found++;

        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_top(row, 2);

        char info[180];
        snprintf(info, sizeof(info),
            "🅿 Spot %-3d  |  🕐 %02d:%02d – %02d:%02d  |  ⏱ %d hr(s)",
            bookings[i].spot_id,
            bookings[i].start_hour, bookings[i].start_min,
            bookings[i].end_hour,   bookings[i].end_min,
            bookings[i].duration_hours);

        GtkWidget *lbl = gtk_label_new(info);
        gtk_widget_set_hexpand(lbl, TRUE);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);

        GtkWidget *del_btn = gtk_button_new_with_label("❌ Cancel this");
        g_signal_connect(del_btn, "clicked",
                         G_CALLBACK(on_confirm_cancel_booking),
                         GINT_TO_POINTER(bookings[i].booking_id));

        gtk_box_append(GTK_BOX(row), lbl);
        gtk_box_append(GTK_BOX(row), del_btn);
        gtk_box_append(GTK_BOX(guest_cancel_list_box), row);
    }
    free(bookings);

    if (found == 0) {
        GtkWidget *none_lbl = gtk_label_new("  (no remaining bookings)");
        gtk_widget_set_halign(none_lbl, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(guest_cancel_list_box), none_lbl);
    }
}

/*
 * "❌ Cancel My Booking" button on the login screen.
 * Loads this guest's bookings from parking_booking.csv and displays each
 * one with its own "Cancel this" button so the guest chooses which to remove.
 */
static void on_guest_cancel(GtkButton *btn, gpointer data) {
    gtk_label_set_text(GTK_LABEL(guest_login_error), "");
    gtk_label_set_text(GTK_LABEL(guest_lookup_label), "");
    gtk_label_set_text(GTK_LABEL(guest_cancel_label), "");

    /* Clear any previously shown booking rows */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(guest_cancel_list_box)) != NULL)
        gtk_box_remove(GTK_BOX(guest_cancel_list_box), child);

    const char *name = gtk_editable_get_text(GTK_EDITABLE(guest_name_entry));
    if (!strlen(name)) {
        gtk_label_set_text(GTK_LABEL(guest_cancel_label),
            "⚠  Please enter your name first.");
        return;
    }

    int status = check_person_parking(name);
    if (status == 0) {
        gtk_label_set_text(GTK_LABEL(guest_cancel_label),
            "⚠  Name not found in the guest registry.");
        return;
    }
    if (status == 1) {
        gtk_label_set_text(GTK_LABEL(guest_cancel_label),
            "⚠  You do not have parking enabled in the registry.");
        return;
    }

    /* Load bookings from CSV and show only this guest's rows */
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    int found = 0;

    for (int i = 0; i < bcount; i++) {
        if (g_ascii_strcasecmp(bookings[i].guest_name, name) != 0) continue;
        found++;

        /* One row per booking: details label + "Cancel this" button */
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_top(row, 2);

        char info[180];
        snprintf(info, sizeof(info),
            "🅿 Spot %-3d  |  🕐 %02d:%02d – %02d:%02d  |  ⏱ %d hr(s)",
            bookings[i].spot_id,
            bookings[i].start_hour, bookings[i].start_min,
            bookings[i].end_hour,   bookings[i].end_min,
            bookings[i].duration_hours);

        GtkWidget *lbl = gtk_label_new(info);
        gtk_widget_set_hexpand(lbl, TRUE);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);

        GtkWidget *del_btn = gtk_button_new_with_label("❌ Cancel this");
        /* Pass the booking_id so we cancel exactly that row in the CSV */
        g_signal_connect(del_btn, "clicked",
                         G_CALLBACK(on_confirm_cancel_booking),
                         GINT_TO_POINTER(bookings[i].booking_id));

        gtk_box_append(GTK_BOX(row), lbl);
        gtk_box_append(GTK_BOX(row), del_btn);
        gtk_box_append(GTK_BOX(guest_cancel_list_box), row);
    }
    free(bookings);

    if (found == 0) {
        gtk_label_set_text(GTK_LABEL(guest_cancel_label),
            "ℹ  No active bookings found for your name.");
    } else {
        gtk_label_set_text(GTK_LABEL(guest_cancel_label),
            "Select a booking below to cancel it:");
    }
}

/* Phase 1 – verify name against persons.csv, switch to booking view */
static void on_guest_login(GtkButton *btn, gpointer data) {
    const char *name = gtk_editable_get_text(GTK_EDITABLE(guest_name_entry));

    if (!strlen(name)) {
        gtk_label_set_text(GTK_LABEL(guest_login_error),
            "⚠  Please enter your name.");
        return;
    }

    int status = check_person_parking(name);
    if (status == 0) {
        gtk_label_set_text(GTK_LABEL(guest_login_error),
            "⚠  Name not found in the guest registry.\n"
            "   Please register with the event manager first.");
        return;
    }
    if (status == 1) {
        gtk_label_set_text(GTK_LABEL(guest_login_error),
            "⚠  You are registered but parking was not requested.\n"
            "   Contact the event manager to enable parking.");
        return;
    }

    load_spot_config();

    /* ONE-BOOKING-PER-GUEST GATE
     * ---------------------------
     * If this guest already holds any booking, deny entry to the booking
     * panel entirely and show the details of their existing reservation.
     * They must cancel it first before a new booking can be made.           */
    {
        Booking existing;
        if (guest_has_existing_booking(name, &existing, 0, 0)) {
            char msg[320];
            snprintf(msg, sizeof(msg),
                "🚫  You already have a parking booking:\n"
                "    Spot %d  |  %02d:%02d – %02d:%02d  |  %d hour(s)\n\n"
                "    Only one booking per guest is allowed.\n"
                "    Please cancel your current booking first.",
                existing.spot_id,
                existing.start_hour, existing.start_min,
                existing.end_hour,   existing.end_min,
                existing.duration_hours);
            gtk_label_set_text(GTK_LABEL(guest_login_error), msg);
            return;   /* stay on the login screen – do NOT open booking panel */
        }
    }

    /* All clear – switch to booking panel */
    char markup[128];
    snprintf(markup, sizeof(markup),
        "Welcome, <b>%s</b>  ✔", name);
    gtk_label_set_markup(GTK_LABEL(guest_locked_name_label), markup);

    /* Reset booking fields and hide preview */
    gtk_editable_set_text(GTK_EDITABLE(guest_start_hour_entry), "");
    gtk_editable_set_text(GTK_EDITABLE(guest_start_min_entry),  "");
    gtk_editable_set_text(GTK_EDITABLE(guest_duration_entry),   "");
    gtk_editable_set_text(GTK_EDITABLE(guest_spot_entry),       "");
    gtk_widget_set_visible(guest_spot_grid_box, FALSE);
    gtk_widget_set_visible(guest_preview_box, FALSE);
    gtk_label_set_text(GTK_LABEL(guest_result_label), "");
    g_has_pending = FALSE;

    gtk_widget_set_visible(guest_login_box,   FALSE);
    gtk_widget_set_visible(guest_booking_box, TRUE);
}

/* -----------------------------------------------------------------------
 *  Build / refresh the guest-facing spot grid.
 *  Same card layout as the admin panel, but guest names are hidden:
 *    - Free card  : green, "AVAILABLE", "Ready to book"
 *    - Occupied   : red,   "OCCUPIED",  booked time slot(s) only (no name)
 * --------------------------------------------------------------------- */
static void on_guest_view_spots(GtkButton *btn, gpointer data) {
    load_spot_config();

    /* Clear any previously built cards */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(guest_spot_grid)) != NULL)
        gtk_flow_box_remove(GTK_FLOW_BOX(guest_spot_grid), child);

    if (g_num_spots == 0) {
        GtkWidget *lbl = gtk_label_new(
            "The parking lot is not configured yet. Please check back later.");
        gtk_flow_box_insert(GTK_FLOW_BOX(guest_spot_grid), lbl, -1);
        gtk_label_set_markup(GTK_LABEL(guest_free_label),
            "<span foreground='#16a34a' weight='bold'>🟢  Free: —</span>");
        gtk_label_set_markup(GTK_LABEL(guest_occupied_label),
            "<span foreground='#dc2626' weight='bold'>🔴  Occupied: —</span>");
        gtk_widget_set_visible(guest_spot_grid_box, TRUE);
        return;
    }

    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    int occupied_spots = 0;

    for (int spot = 1; spot <= g_num_spots; spot++) {
        gboolean has_booking = FALSE;
        for (int i = 0; i < bcount; i++)
            if (bookings[i].spot_id == spot) { has_booking = TRUE; break; }

        if (has_booking) occupied_spots++;

        /* card frame */
        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_widget_set_size_request(frame, 155, -1);
        gtk_widget_add_css_class(frame, "spot-card-frame");

        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(card, 10);
        gtk_widget_set_margin_end(card, 10);
        gtk_widget_set_margin_top(card, 10);
        gtk_widget_set_margin_bottom(card, 10);
        gtk_widget_add_css_class(card,
            has_booking ? "spot-card-occupied" : "spot-card-free");

        /* top row: status dot + spot number */
        GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(top_row, GTK_ALIGN_CENTER);

        GtkWidget *icon_lbl = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(icon_lbl),
            has_booking ? "<span font='16'>🔴</span>"
                        : "<span font='16'>🟢</span>");

        char spot_str[64];
        snprintf(spot_str, sizeof(spot_str),
            "<span font='14' weight='bold' foreground='%s'>P%d</span>",
            has_booking ? "#c62828" : "#1b5e20", spot);
        GtkWidget *num_lbl = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(num_lbl), spot_str);

        gtk_box_append(GTK_BOX(top_row), icon_lbl);
        gtk_box_append(GTK_BOX(top_row), num_lbl);

        /* status badge pill */
        GtkWidget *badge = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(badge),
            has_booking
                ? "<span font='7.5' weight='bold' foreground='#ffffff'>OCCUPIED</span>"
                : "<span font='7.5' weight='bold' foreground='#ffffff'>AVAILABLE</span>");
        gtk_widget_set_halign(badge, GTK_ALIGN_CENTER);
        gtk_widget_add_css_class(badge,
            has_booking ? "badge-occupied" : "badge-free");

        /* detail: time slots only — NO guest names */
        GtkWidget *detail_lbl = gtk_label_new(NULL);
        gtk_label_set_wrap(GTK_LABEL(detail_lbl), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(detail_lbl), 20);
        gtk_widget_set_halign(detail_lbl, GTK_ALIGN_CENTER);

        if (has_booking) {
            GString *detail = g_string_new(NULL);
            for (int j = 0; j < bcount; j++) {
                if (bookings[j].spot_id != spot) continue;
                if (detail->len > 0) g_string_append(detail, "\n");
                /* Show time range only — name deliberately omitted */
                g_string_append_printf(detail,
                    "<span font='7.5' foreground='#7f0000'>%02d:%02d – %02d:%02d</span>",
                    bookings[j].start_hour, bookings[j].start_min,
                    bookings[j].end_hour,   bookings[j].end_min);
            }
            gtk_label_set_markup(GTK_LABEL(detail_lbl), detail->str);
            g_string_free(detail, TRUE);
        } else {
            gtk_label_set_markup(GTK_LABEL(detail_lbl),
                "<span font='8' foreground='#2e7d32'>Ready to book</span>");
        }

        gtk_box_append(GTK_BOX(card), top_row);
        gtk_box_append(GTK_BOX(card), badge);
        gtk_box_append(GTK_BOX(card), detail_lbl);
        gtk_frame_set_child(GTK_FRAME(frame), card);
        gtk_flow_box_insert(GTK_FLOW_BOX(guest_spot_grid), frame, -1);
    }

    free(bookings);

    /* update counters */
    char cnt[128];
    snprintf(cnt, sizeof(cnt),
        "<span foreground='#16a34a' weight='bold'>🟢  Free: %d</span>",
        g_num_spots - occupied_spots);
    gtk_label_set_markup(GTK_LABEL(guest_free_label), cnt);
    snprintf(cnt, sizeof(cnt),
        "<span foreground='#dc2626' weight='bold'>🔴  Occupied: %d</span>",
        occupied_spots);
    gtk_label_set_markup(GTK_LABEL(guest_occupied_label), cnt);

    gtk_widget_set_visible(guest_spot_grid_box, TRUE);
}

/* Phase 2a – check availability and show a preview; do NOT book yet */
static void on_guest_check(GtkButton *btn, gpointer data) {
    gtk_label_set_text(GTK_LABEL(guest_result_label), "");
    g_has_pending = FALSE;
    gtk_widget_set_visible(guest_preview_box, FALSE);

    /* read locked name from label markup — easier to store separately */
    const char *name = gtk_editable_get_text(GTK_EDITABLE(guest_name_entry));

    const char *sh_str   = gtk_editable_get_text(GTK_EDITABLE(guest_start_hour_entry));
    const char *sm_str   = gtk_editable_get_text(GTK_EDITABLE(guest_start_min_entry));
    const char *dur_str  = gtk_editable_get_text(GTK_EDITABLE(guest_duration_entry));
    const char *spot_str = gtk_editable_get_text(GTK_EDITABLE(guest_spot_entry));

    if (!strlen(sh_str) || !strlen(sm_str) || !strlen(dur_str) || !strlen(spot_str)) {
        gtk_label_set_text(GTK_LABEL(guest_result_label),
            "⚠  Please fill in all fields, including your preferred spot number.");
        return;
    }

    int sh   = atoi(sh_str);
    int sm   = atoi(sm_str);
    int dur  = atoi(dur_str);
    int spot = atoi(spot_str);

    if (sh < 0 || sh > 23 || sm < 0 || sm > 59) {
        gtk_label_set_text(GTK_LABEL(guest_result_label),
            "⚠  Invalid start time. Hour: 0–23, Minute: 0–59.");
        return;
    }
    if (dur <= 0) {
        gtk_label_set_text(GTK_LABEL(guest_result_label),
            "⚠  Duration must be at least 1 hour.");
        return;
    }

    load_spot_config();
    if (g_num_spots == 0) {
        gtk_label_set_text(GTK_LABEL(guest_result_label),
            "⚠  The parking lot is not configured yet. Please check back later.");
        return;
    }

    if (spot < 1 || spot > g_num_spots) {
        char msg[200];
        snprintf(msg, sizeof(msg),
            "⚠  Spot %d does not exist. Please choose a spot between 1 and %d.",
            spot, g_num_spots);
        gtk_label_set_text(GTK_LABEL(guest_result_label), msg);
        return;
    }

    if (g_max_hours > 0 && dur > g_max_hours) {
        char msg[200];
        snprintf(msg, sizeof(msg),
            "⚠  Duration (%d h) exceeds the maximum allowed (%d h).",
            dur, g_max_hours);
        gtk_label_set_text(GTK_LABEL(guest_result_label), msg);
        return;
    }

    int eh, em;
    add_hours(sh, sm, dur, &eh, &em);
    int req_start = to_minutes(sh, sm);
    int req_end   = to_minutes(eh, em);

    /* Check whether the guest's chosen spot is free for the requested window */
    if (!is_spot_available(spot, req_start, req_end)) {
        char msg[300];
        snprintf(msg, sizeof(msg),
            "🚫  Spot P%d is occupied during %02d:%02d – %02d:%02d.\n"
            "   Someone has already reserved that spot for this time.\n"
            "   Please choose a different spot number or a different time.",
            spot, sh, sm, eh, em);
        gtk_label_set_text(GTK_LABEL(guest_result_label), msg);
        return;
    }

    /* Spot is free — fill pending booking (not saved yet).
     * NOTE: the one-booking-per-guest check is intentionally deferred to
     * on_guest_confirm() rather than performed here.  This prevents a
     * misleading error when the real issue is an existing booking.          */
    g_pending_booking.booking_id     = next_booking_id();
    g_pending_booking.spot_id        = spot;
    g_pending_booking.start_hour     = sh;
    g_pending_booking.start_min      = sm;
    g_pending_booking.duration_hours = dur;
    g_pending_booking.end_hour       = eh;
    g_pending_booking.end_min        = em;
    strncpy(g_pending_booking.guest_name, name, MAX_NAME_LEN - 1);
    g_pending_booking.guest_name[MAX_NAME_LEN - 1] = '\0';
    g_has_pending = TRUE;

    /* Show preview */
    char preview[300];
    snprintf(preview, sizeof(preview),
        "🅿  Spot       :  <b>%d</b>\n"
        "🕐  From       :  <b>%02d:%02d</b>\n"
        "🕑  To         :  <b>%02d:%02d</b>\n"
        "⏱  Duration   :  <b>%d hour(s)</b>",
        spot, sh, sm, eh, em, dur);
    gtk_label_set_markup(GTK_LABEL(guest_preview_label), preview);
    gtk_widget_set_visible(guest_preview_box, TRUE);
}

/* Phase 2b – confirm and actually save the booking */
static void on_guest_confirm(GtkButton *btn, gpointer data) {
    if (!g_has_pending) {
        gtk_label_set_text(GTK_LABEL(guest_result_label),
            "⚠  Please check availability first.");
        return;
    }

    /*
     * ONE-BOOKING-PER-GUEST ENFORCEMENT (confirm stage)
     * --------------------------------------------------
     * Before writing the new booking to disk, perform a final check to
     * ensure the guest does not already hold ANY existing booking.
     *
     * guest_has_existing_booking() now returns TRUE for ANY pre-existing
     * booking by this guest, regardless of time window (see its definition
     * above).  The req_start / req_end values are still computed and passed
     * to keep the call signature unchanged, but they are not used by the
     * function under the new policy.
     *
     * If a conflict is found, the pending booking is discarded and the guest
     * is shown the details of their existing reservation so they know what
     * is blocking the new request.  To make a new booking they must first
     * cancel the existing one via the "❌ Cancel My Booking" button.
     */
    {
        int req_start = to_minutes(g_pending_booking.start_hour,
                                   g_pending_booking.start_min);
        int req_end   = to_minutes(g_pending_booking.end_hour,
                                   g_pending_booking.end_min);
        Booking existing;
        if (guest_has_existing_booking(g_pending_booking.guest_name, &existing,
                                       req_start, req_end)) {
            /* Guest already has a booking — reject the new request and
             * display the details of their current reservation.            */
            char msg[300];
            snprintf(msg, sizeof(msg),
                "🚫  You already have an existing booking:\n"
                "    Spot %d  |  %02d:%02d – %02d:%02d\n"
                "    Only one booking per guest is allowed.\n"
                "    Please cancel your current booking first.",
                existing.spot_id,
                existing.start_hour, existing.start_min,
                existing.end_hour,   existing.end_min);
            gtk_label_set_text(GTK_LABEL(guest_result_label), msg);
            g_has_pending = FALSE;                /* discard pending data   */
            gtk_widget_set_visible(guest_preview_box, FALSE);
            return;
        }
    }

    append_booking(&g_pending_booking);
    g_has_pending = FALSE;

    /* Hide everything — booking phase disappears */
    gtk_widget_set_visible(guest_booking_box, FALSE);
    gtk_widget_set_visible(guest_preview_box, FALSE);

    /* Show login screen again with a success message,
       clear the name so next guest starts fresh */
    gtk_editable_set_text(GTK_EDITABLE(guest_name_entry), "");
    gtk_label_set_text(GTK_LABEL(guest_login_error),
        "✅  Booking confirmed! You may now close this panel.");
    gtk_label_set_text(GTK_LABEL(guest_lookup_label), "");
    gtk_widget_set_visible(guest_login_box, TRUE);
}

/* ===================================================================
 *  PAGE BUILDERS
 * =================================================================== */

/* ---- Admin page ---- */

/* Convert hh:mm to minutes-since-midnight */
static int to_minutes(int h, int m) { return h * 60 + m; }

/* Add duration (hours) to a start time, wrapping at midnight */
static void add_hours(int sh, int sm, int dur_h, int *eh, int *em) {
    int total = to_minutes(sh, sm) + dur_h * 60;
    total %= (24 * 60);
    *eh = total / 60;
    *em = total % 60;
}

/* Returns TRUE if [as,ae) overlaps [bs,be)  (all in minutes) */
static gboolean intervals_overlap(int as, int ae, int bs, int be) {
    /* handle wrap-around by splitting into two sub-intervals */
    if (ae <= as) ae += 24 * 60;   /* wraps midnight */
    if (be <= bs) be += 24 * 60;
    return (as < be) && (bs < ae);
}

/* ===================================================================
 *  UTILITY – CSV persistence
 * =================================================================== */

/* Save global spot settings (num_spots, max_hours) */
static void save_spot_config(void) {
    FILE *f = fopen(SPOTS_FILE, "w");
    if (!f) { g_print("Cannot write %s\n", SPOTS_FILE); return; }
    fprintf(f, "%d,%d\n", g_num_spots, g_max_hours);
    fclose(f);
}

/* Load global spot settings; returns FALSE if file missing */
static gboolean load_spot_config(void) {
    FILE *f = fopen(SPOTS_FILE, "r");
    if (!f) return FALSE;
    int r = fscanf(f, "%d,%d", &g_num_spots, &g_max_hours);
    fclose(f);
    return (r == 2);
}

/* Append one booking to the bookings CSV */
static void append_booking(const Booking *b) {
    FILE *f = fopen(BOOKINGS_FILE, "a");
    if (!f) { g_print("Cannot write %s\n", BOOKINGS_FILE); return; }
    fprintf(f, "%d,%d,%s,%d,%d,%d,%d,%d\n",
            b->booking_id, b->spot_id, b->guest_name,
            b->start_hour, b->start_min,
            b->duration_hours,
            b->end_hour, b->end_min);
    fclose(f);
}

/* Return next booking ID */
static int next_booking_id(void) {
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

/* Load all bookings into a dynamically-allocated array; caller frees */
static Booking *load_bookings(int *out_count) {
    *out_count = 0;
    FILE *f = fopen(BOOKINGS_FILE, "r");
    if (!f) return NULL;

    /* count lines first */
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

/* ===================================================================
 *  UTILITY – persons.csv lookup
 * =================================================================== */

/*
 * Check whether 'name' exists in persons.csv AND has parking="Yes".
 * CSV format (from gtkperson-1.c):
 *   id, name, age, status, phone, side(int), parking
 * Returns:
 *   0  – not found in persons.csv
 *   1  – found but parking = "No"
 *   2  – found and parking = "Yes"  (allowed to book)
 */
static int check_person_parking(const char *name) {
    FILE *f = fopen(PERSONS_FILE, "r");
    if (!f) return 0;   /* no file → treat as not registered */

    char line[512];
    int  id, age, side;
    char pname[100], status[50], phone[20], parking[10];

    while (fgets(line, sizeof(line), f)) {
        /* Use %[^,] for parking so it stops at the comma, not whitespace.
         * This correctly handles extra fields after parking (e.g. email).
         * We require at least 7 fields; any extra fields are ignored. */
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9[^,\r\n]",
                   &id, pname, &age, status, phone, &side, parking) != 7)
            continue;

        /* case-insensitive name comparison */
        if (g_ascii_strcasecmp(pname, name) == 0) {
            fclose(f);
            /* strip any stray whitespace/newline from parking field */
            parking[strcspn(parking, " \t\r\n")] = '\0';
            return (g_ascii_strcasecmp(parking, "Yes") == 0) ? 2 : 1;
        }
    }
    fclose(f);
    return 0;
}

/* ===================================================================
 *  OVERLAP / SPOT ASSIGNMENT LOGIC
 * =================================================================== */

/*
 * Find the first spot (1 .. g_num_spots) whose existing bookings do NOT
 * overlap with [start_min, end_min).
 * Returns the spot number, or -1 if all spots are occupied for that window.
 */
static int find_available_spot(int req_start, int req_end) {
    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    for (int spot = 1; spot <= g_num_spots; spot++) {
        gboolean conflict = FALSE;
        for (int i = 0; i < bcount; i++) {
            if (bookings[i].spot_id != spot) continue;
            int bs = to_minutes(bookings[i].start_hour, bookings[i].start_min);
            int be = to_minutes(bookings[i].end_hour,   bookings[i].end_min);
            if (intervals_overlap(req_start, req_end, bs, be)) {
                conflict = TRUE;
                break;
            }
        }
        if (!conflict) {
            free(bookings);
            return spot;
        }
    }
    free(bookings);
    return -1;   /* all spots busy */
}

/*
 * Check whether a specific spot is free for [req_start, req_end).
 * Returns TRUE if the spot exists (1 .. g_num_spots) AND has no
 * conflicting booking; FALSE otherwise.
 */
static gboolean is_spot_available(int spot, int req_start, int req_end) {
    if (spot < 1 || spot > g_num_spots) return FALSE;

    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);

    for (int i = 0; i < bcount; i++) {
        if (bookings[i].spot_id != spot) continue;
        int bs = to_minutes(bookings[i].start_hour, bookings[i].start_min);
        int be = to_minutes(bookings[i].end_hour,   bookings[i].end_min);
        if (intervals_overlap(req_start, req_end, bs, be)) {
            free(bookings);
            return FALSE;   /* conflict found */
        }
    }
    free(bookings);
    return TRUE;
}

/* ===================================================================
 *  ADMIN PANEL CALLBACKS
 * =================================================================== */

/* Verify password and reveal settings controls */
static void on_admin_unlock(GtkButton *btn, gpointer data) {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(admin_password_entry));
    if (strcmp(pw, PASSWORD) != 0) {
        show_dialog("Error", "Incorrect password!");
        return;
    }
    load_spot_config();
    /* pre-fill current values */
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", g_num_spots);
    gtk_editable_set_text(GTK_EDITABLE(admin_spots_entry), buf);
    snprintf(buf, sizeof(buf), "%d", g_max_hours);
    gtk_editable_set_text(GTK_EDITABLE(admin_maxhours_entry), buf);

    gtk_widget_set_visible(admin_settings_box, TRUE);
}

/* Save spot / max-hour settings */
static void on_admin_save_settings(GtkButton *btn, gpointer data) {
    const char *spots_str = gtk_editable_get_text(GTK_EDITABLE(admin_spots_entry));
    const char *hours_str = gtk_editable_get_text(GTK_EDITABLE(admin_maxhours_entry));

    int spots = atoi(spots_str);
    int hours = atoi(hours_str);

    if (spots <= 0 || spots > MAX_SPOTS) {
        char msg[128];
        snprintf(msg, sizeof(msg),
            "Number of spots must be between 1 and %d.", MAX_SPOTS);
        show_dialog("Warning", msg);
        return;
    }
    if (hours <= 0) {
        show_dialog("Warning", "Maximum hours must be a positive number.");
        return;
    }

    g_num_spots = spots;
    g_max_hours = hours;
    save_spot_config();

    char info[200];
    snprintf(info, sizeof(info),
        "Settings saved!\n%d spots  |  max %d hour(s)",
        g_num_spots, g_max_hours);
    show_dialog("Settings Saved", info);
}

/* Rebuild the visual spot-card grid */
static void on_admin_refresh_status(GtkButton *btn, gpointer data) {

    /* Remove all existing cards */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(admin_status_grid)) != NULL)
        gtk_flow_box_remove(GTK_FLOW_BOX(admin_status_grid), child);

    if (g_num_spots == 0) {
        GtkWidget *lbl = gtk_label_new("No spots configured yet. Save settings first.");
        gtk_flow_box_insert(GTK_FLOW_BOX(admin_status_grid), lbl, -1);
        gtk_label_set_markup(GTK_LABEL(admin_free_label),
            "<span foreground='#16a34a' weight='bold'>🟢  Free: —</span>");
        gtk_label_set_markup(GTK_LABEL(admin_occupied_label),
            "<span foreground='#dc2626' weight='bold'>🔴  Occupied: —</span>");
        return;
    }

    int bcount = 0;
    Booking *bookings = load_bookings(&bcount);
    int occupied_spots = 0;

    for (int spot = 1; spot <= g_num_spots; spot++) {
        gboolean has_booking = FALSE;

        for (int i = 0; i < bcount; i++) {
            if (bookings[i].spot_id != spot) continue;
            has_booking = TRUE;
        }
        if (has_booking) occupied_spots++;

        /* ---- card frame ---- */
        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_widget_set_size_request(frame, 155, -1);
        gtk_widget_add_css_class(frame, "spot-card-frame");

        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(card, 10);
        gtk_widget_set_margin_end(card, 10);
        gtk_widget_set_margin_top(card, 10);
        gtk_widget_set_margin_bottom(card, 10);
        gtk_widget_add_css_class(card, has_booking ? "spot-card-occupied" : "spot-card-free");

        /* top row: status dot + spot number */
        GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(top_row, GTK_ALIGN_CENTER);

        GtkWidget *icon_lbl = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(icon_lbl),
            has_booking ? "<span font='16'>🔴</span>" : "<span font='16'>🟢</span>");

        char spot_str[64];
        snprintf(spot_str, sizeof(spot_str),
            "<span font='14' weight='bold' foreground='%s'>P%d</span>",
            has_booking ? "#c62828" : "#1b5e20", spot);
        GtkWidget *num_lbl = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(num_lbl), spot_str);

        gtk_box_append(GTK_BOX(top_row), icon_lbl);
        gtk_box_append(GTK_BOX(top_row), num_lbl);

        /* status badge pill */
        GtkWidget *badge = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(badge),
            has_booking
                ? "<span font='7.5' weight='bold' foreground='#ffffff'>OCCUPIED</span>"
                : "<span font='7.5' weight='bold' foreground='#ffffff'>AVAILABLE</span>");
        gtk_widget_set_halign(badge, GTK_ALIGN_CENTER);
        gtk_widget_add_css_class(badge, has_booking ? "badge-occupied" : "badge-free");

        /* booking detail or ready message */
        GtkWidget *detail_lbl = gtk_label_new(NULL);
        gtk_label_set_wrap(GTK_LABEL(detail_lbl), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(detail_lbl), 20);
        gtk_widget_set_halign(detail_lbl, GTK_ALIGN_CENTER);

        if (has_booking) {
            GString *detail = g_string_new(NULL);
            for (int j = 0; j < bcount; j++) {
                if (bookings[j].spot_id != spot) continue;
                if (detail->len > 0)
                    g_string_append(detail, "\n");
                g_string_append_printf(detail,
                    "<span font='7.5' foreground='#7f0000'>%s\n%02d:%02d – %02d:%02d</span>",
                    bookings[j].guest_name,
                    bookings[j].start_hour, bookings[j].start_min,
                    bookings[j].end_hour,   bookings[j].end_min);
            }
            gtk_label_set_markup(GTK_LABEL(detail_lbl), detail->str);
            g_string_free(detail, TRUE);
        } else {
            gtk_label_set_markup(GTK_LABEL(detail_lbl),
                "<span font='8' foreground='#2e7d32'>Ready to book</span>");
        }

        gtk_box_append(GTK_BOX(card), top_row);
        gtk_box_append(GTK_BOX(card), badge);
        gtk_box_append(GTK_BOX(card), detail_lbl);
        gtk_frame_set_child(GTK_FRAME(frame), card);
        gtk_flow_box_insert(GTK_FLOW_BOX(admin_status_grid), frame, -1);
    }

    free(bookings);

    /* update summary counters */
    char cnt[128];
    snprintf(cnt, sizeof(cnt),
        "<span foreground='#16a34a' weight='bold'>🟢  Free: %d</span>",
        g_num_spots - occupied_spots);
    gtk_label_set_markup(GTK_LABEL(admin_free_label), cnt);
    snprintf(cnt, sizeof(cnt),
        "<span foreground='#dc2626' weight='bold'>🔴  Occupied: %d</span>",
        occupied_spots);
    gtk_label_set_markup(GTK_LABEL(admin_occupied_label), cnt);
}

/* Auto-refresh every 3 s when the admin panel is visible */
static gboolean admin_auto_refresh(gpointer data) {
    on_admin_refresh_status(NULL, NULL);
    return TRUE;   /* keep the timeout alive */
}

/* ===================================================================
 *  PAGE BUILDERS
 * =================================================================== */

/* ---- Admin page ---- */
static GtkWidget *create_admin_page(void) {
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(outer, 12);
    gtk_widget_set_margin_end(outer, 12);
    gtk_widget_set_margin_top(outer, 12);
    gtk_widget_set_margin_bottom(outer, 12);

    /* -- password row -- */
    GtkWidget *pw_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *pw_lbl = gtk_label_new("Admin Password:");
    admin_password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(admin_password_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(admin_password_entry),
                                   "Enter admin password");
    gtk_widget_set_hexpand(admin_password_entry, TRUE);

    GtkWidget *unlock_btn = gtk_button_new_with_label("Unlock");
    g_signal_connect(unlock_btn, "clicked", G_CALLBACK(on_admin_unlock), NULL);

    gtk_box_append(GTK_BOX(pw_row), pw_lbl);
    gtk_box_append(GTK_BOX(pw_row), admin_password_entry);
    gtk_box_append(GTK_BOX(pw_row), unlock_btn);
    gtk_box_append(GTK_BOX(outer), pw_row);

    /* -- settings box (hidden until unlocked) -- */
    admin_settings_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_visible(admin_settings_box, FALSE);

    /* spots entry */
    GtkWidget *spots_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(spots_row), gtk_label_new("Number of Parking Spots:"));
    admin_spots_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(admin_spots_entry), "e.g. 20");
    gtk_widget_set_hexpand(admin_spots_entry, TRUE);
    gtk_box_append(GTK_BOX(spots_row), admin_spots_entry);
    gtk_box_append(GTK_BOX(admin_settings_box), spots_row);

    /* max hours entry */
    GtkWidget *hours_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(hours_row), gtk_label_new("Maximum Parking Hours:  "));
    admin_maxhours_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(admin_maxhours_entry), "e.g. 4");
    gtk_widget_set_hexpand(admin_maxhours_entry, TRUE);
    gtk_box_append(GTK_BOX(hours_row), admin_maxhours_entry);
    gtk_box_append(GTK_BOX(admin_settings_box), hours_row);

    /* save settings button */
    GtkWidget *save_btn = gtk_button_new_with_label("Save Settings");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_admin_save_settings), NULL);
    gtk_box_append(GTK_BOX(admin_settings_box), save_btn);

    gtk_box_append(GTK_BOX(outer), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* -- status section -- */
    GtkWidget *status_hdr = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(status_hdr),
        "<span font='13' weight='bold'>🅿  Spot Occupation Status</span>");
    gtk_widget_set_halign(status_hdr, GTK_ALIGN_START);
    gtk_widget_set_margin_top(status_hdr, 6);
    gtk_box_append(GTK_BOX(admin_settings_box), status_hdr);

    /* counters */
    GtkWidget *counters = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_margin_top(counters, 4);
    gtk_widget_set_margin_bottom(counters, 2);
    admin_free_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(admin_free_label),
        "<span foreground='#16a34a' weight='bold'>🟢  Free: —</span>");
    admin_occupied_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(admin_occupied_label),
        "<span foreground='#dc2626' weight='bold'>🔴  Occupied: —</span>");
    gtk_box_append(GTK_BOX(counters), admin_free_label);
    gtk_box_append(GTK_BOX(counters), admin_occupied_label);
    gtk_box_append(GTK_BOX(admin_settings_box), counters);

    /* refresh button */
    GtkWidget *ref_btn = gtk_button_new_with_label("⟳  Refresh");
    g_signal_connect(ref_btn, "clicked", G_CALLBACK(on_admin_refresh_status), NULL);
    gtk_box_append(GTK_BOX(admin_settings_box), ref_btn);

    /* CSS for spot cards */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        /* card outer frame – no border, just a soft shadow via margin */
        ".spot-card-frame {"
        "  border-radius: 12px;"
        "  border: none;"
        "}"
        /* free card: clean white-green gradient */
        ".spot-card-free {"
        "  background: linear-gradient(160deg, #f0fff4 0%, #dcfce7 100%);"
        "  border-radius: 10px;"
        "  border: 2px solid #86efac;"
        "}"
        /* occupied card: warm red gradient */
        ".spot-card-occupied {"
        "  background: linear-gradient(160deg, #fff1f2 0%, #fecdd3 100%);"
        "  border-radius: 10px;"
        "  border: 2px solid #fca5a5;"
        "}"
        /* Available pill badge */
        ".badge-free {"
        "  background-color: #16a34a;"
        "  border-radius: 999px;"
        "  padding: 2px 10px;"
        "  color: white;"
        "}"
        /* Occupied pill badge */
        ".badge-occupied {"
        "  background-color: #dc2626;"
        "  border-radius: 999px;"
        "  padding: 2px 10px;"
        "  color: white;"
        "}"
    );
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    /* flow box grid of spot cards */
    admin_status_grid = gtk_flow_box_new();
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(admin_status_grid), TRUE);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(admin_status_grid), 3);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(admin_status_grid), 8);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(admin_status_grid), 8);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(admin_status_grid), 8);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(admin_status_grid), GTK_SELECTION_NONE);
    gtk_widget_set_margin_start(admin_status_grid, 6);
    gtk_widget_set_margin_end(admin_status_grid, 6);
    gtk_widget_set_margin_top(admin_status_grid, 6);
    gtk_widget_set_margin_bottom(admin_status_grid, 6);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 220);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), admin_status_grid);
    gtk_box_append(GTK_BOX(admin_settings_box), scroll);

    gtk_box_append(GTK_BOX(outer), admin_settings_box);

    /* auto-refresh every 3 s */
    g_timeout_add(3000, admin_auto_refresh, NULL);

    return outer;
}

/* ---- Guest page ---- */
static GtkWidget *create_guest_page(void) {
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(outer, TRUE);
    gtk_widget_set_vexpand(outer, TRUE);

    /* ================================================================
     *  PHASE 1 – Login box
     * ================================================================ */
    guest_login_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_halign(guest_login_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(guest_login_box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(guest_login_box, TRUE);
    gtk_widget_set_margin_start(guest_login_box, 40);
    gtk_widget_set_margin_end(guest_login_box, 40);

    GtkWidget *login_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(login_title),
        "<span font='16' weight='bold'>🚗  Guest Parking</span>");
    gtk_widget_set_halign(login_title, GTK_ALIGN_CENTER);

    GtkWidget *login_sub = gtk_label_new(
        "Enter your registered name to access parking booking.");
    gtk_label_set_wrap(GTK_LABEL(login_sub), TRUE);
    gtk_widget_set_halign(login_sub, GTK_ALIGN_CENTER);

    GtkWidget *name_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(name_row, GTK_ALIGN_CENTER);
    GtkWidget *name_lbl = gtk_label_new("Full Name:");
    guest_name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(guest_name_entry),
                                   "As registered in the system");
    gtk_widget_set_size_request(guest_name_entry, 260, -1);
    gtk_box_append(GTK_BOX(name_row), name_lbl);
    gtk_box_append(GTK_BOX(name_row), guest_name_entry);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_CENTER);

    GtkWidget *login_btn = gtk_button_new_with_label("  Access Parking  ");
    g_signal_connect(login_btn, "clicked", G_CALLBACK(on_guest_login), NULL);

    GtkWidget *lookup_btn = gtk_button_new_with_label("🔍  View My Booking");
    g_signal_connect(lookup_btn, "clicked", G_CALLBACK(on_guest_lookup), NULL);

    GtkWidget *cancel_btn = gtk_button_new_with_label("❌  Cancel My Booking");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_guest_cancel), NULL);

    gtk_box_append(GTK_BOX(btn_row), login_btn);
    gtk_box_append(GTK_BOX(btn_row), lookup_btn);
    gtk_box_append(GTK_BOX(btn_row), cancel_btn);

    guest_login_error = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(guest_login_error), TRUE);
    gtk_widget_set_halign(guest_login_error, GTK_ALIGN_CENTER);

    guest_lookup_label = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(guest_lookup_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(guest_lookup_label), 0.0f);
    gtk_widget_set_margin_top(guest_lookup_label, 6);

    /* Label that shows the result of a cancellation attempt */
    guest_cancel_label = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(guest_cancel_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(guest_cancel_label), 0.0f);
    gtk_widget_set_margin_top(guest_cancel_label, 4);

    /* Box populated dynamically with one row per booking found in CSV */
    guest_cancel_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top(guest_cancel_list_box, 4);
    gtk_widget_set_margin_start(guest_cancel_list_box, 8);

    gtk_box_append(GTK_BOX(guest_login_box), login_title);
    gtk_box_append(GTK_BOX(guest_login_box), login_sub);
    gtk_box_append(GTK_BOX(guest_login_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(guest_login_box), name_row);
    gtk_box_append(GTK_BOX(guest_login_box), btn_row);
    gtk_box_append(GTK_BOX(guest_login_box), guest_login_error);
    gtk_box_append(GTK_BOX(guest_login_box), guest_lookup_label);
    gtk_box_append(GTK_BOX(guest_login_box), guest_cancel_label);
    gtk_box_append(GTK_BOX(guest_login_box), guest_cancel_list_box);
    gtk_box_append(GTK_BOX(outer), guest_login_box);

    /* ================================================================
     *  PHASE 2 – Booking box (hidden until login verified)
     * ================================================================ */
    guest_booking_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(guest_booking_box, 16);
    gtk_widget_set_margin_end(guest_booking_box, 16);
    gtk_widget_set_margin_top(guest_booking_box, 12);
    gtk_widget_set_margin_bottom(guest_booking_box, 12);
    gtk_widget_set_visible(guest_booking_box, FALSE);

    /* Welcome / locked name */
    guest_locked_name_label = gtk_label_new(NULL);
    gtk_widget_set_halign(guest_locked_name_label, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(guest_booking_box), guest_locked_name_label);
    gtk_box_append(GTK_BOX(guest_booking_box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *booking_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(booking_title), "<b>Request a Parking Slot</b>");
    gtk_widget_set_halign(booking_title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(guest_booking_box), booking_title);

    GtkWidget *row;

    /* start hour */
    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Start Hour (0–23):   "));
    guest_start_hour_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(guest_start_hour_entry), "e.g. 9");
    gtk_widget_set_hexpand(guest_start_hour_entry, TRUE);
    gtk_box_append(GTK_BOX(row), guest_start_hour_entry);
    gtk_box_append(GTK_BOX(guest_booking_box), row);

    /* start minute */
    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Start Minute (0–59): "));
    guest_start_min_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(guest_start_min_entry), "e.g. 30");
    gtk_widget_set_hexpand(guest_start_min_entry, TRUE);
    gtk_box_append(GTK_BOX(row), guest_start_min_entry);
    gtk_box_append(GTK_BOX(guest_booking_box), row);

    /* duration */
    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Duration (hours):    "));
    guest_duration_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(guest_duration_entry), "e.g. 2");
    gtk_widget_set_hexpand(guest_duration_entry, TRUE);
    gtk_box_append(GTK_BOX(row), guest_duration_entry);
    gtk_box_append(GTK_BOX(guest_booking_box), row);

    /* "View Available Spots" button */
    GtkWidget *view_spots_btn =
        gtk_button_new_with_label("🅿  View Available Spots");
    gtk_widget_set_halign(view_spots_btn, GTK_ALIGN_START);
    g_signal_connect(view_spots_btn, "clicked",
                     G_CALLBACK(on_guest_view_spots), NULL);
    gtk_box_append(GTK_BOX(guest_booking_box), view_spots_btn);

    /* ---- Spot grid box (hidden until "View" is clicked) ---- */
    guest_spot_grid_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_visible(guest_spot_grid_box, FALSE);
    gtk_widget_set_margin_top(guest_spot_grid_box, 4);

    /* counter row */
    GtkWidget *gcounters = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_margin_bottom(gcounters, 2);
    guest_free_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(guest_free_label),
        "<span foreground='#16a34a' weight='bold'>🟢  Free: —</span>");
    guest_occupied_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(guest_occupied_label),
        "<span foreground='#dc2626' weight='bold'>🔴  Occupied: —</span>");
    gtk_box_append(GTK_BOX(gcounters), guest_free_label);
    gtk_box_append(GTK_BOX(gcounters), guest_occupied_label);
    gtk_box_append(GTK_BOX(guest_spot_grid_box), gcounters);

    /* flow box of spot cards */
    guest_spot_grid = gtk_flow_box_new();
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(guest_spot_grid), TRUE);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(guest_spot_grid), 3);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(guest_spot_grid), 8);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(guest_spot_grid), 8);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(guest_spot_grid), 8);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(guest_spot_grid),
                                    GTK_SELECTION_NONE);
    gtk_widget_set_margin_start(guest_spot_grid, 4);
    gtk_widget_set_margin_end(guest_spot_grid, 4);

    GtkWidget *gscroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(gscroll), 180);
    gtk_widget_set_vexpand(gscroll, FALSE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(gscroll), guest_spot_grid);
    gtk_box_append(GTK_BOX(guest_spot_grid_box), gscroll);

    gtk_box_append(GTK_BOX(guest_booking_box), guest_spot_grid_box);

    /* preferred spot number */
    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Preferred Spot No.:  "));
    guest_spot_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(guest_spot_entry), "e.g. 3");
    gtk_widget_set_hexpand(guest_spot_entry, TRUE);
    gtk_box_append(GTK_BOX(row), guest_spot_entry);
    gtk_box_append(GTK_BOX(guest_booking_box), row);

    /* Check button */
    GtkWidget *check_btn = gtk_button_new_with_label("🔍  Check Availability");
    g_signal_connect(check_btn, "clicked", G_CALLBACK(on_guest_check), NULL);
    gtk_box_append(GTK_BOX(guest_booking_box), check_btn);

    /* error/info label (shown below Check) */
    guest_result_label = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(guest_result_label), TRUE);
    gtk_widget_set_halign(guest_result_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(guest_booking_box), guest_result_label);

    /* ---- Preview box (hidden until availability found) ---- */
    guest_preview_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_visible(guest_preview_box, FALSE);
    gtk_widget_set_margin_top(guest_preview_box, 8);

    gtk_box_append(GTK_BOX(guest_preview_box),
                   gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *prev_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(prev_title),
        "<b>Your Parking Summary</b>");
    gtk_widget_set_halign(prev_title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(guest_preview_box), prev_title);

    guest_preview_label = gtk_label_new(NULL);
    gtk_label_set_wrap(GTK_LABEL(guest_preview_label), TRUE);
    gtk_widget_set_halign(guest_preview_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(guest_preview_label, 12);
    gtk_box_append(GTK_BOX(guest_preview_box), guest_preview_label);

    guest_confirm_btn = gtk_button_new_with_label("✅  Confirm Booking");
    gtk_widget_set_halign(guest_confirm_btn, GTK_ALIGN_START);
    g_signal_connect(guest_confirm_btn, "clicked",
                     G_CALLBACK(on_guest_confirm), NULL);
    gtk_box_append(GTK_BOX(guest_preview_box), guest_confirm_btn);

    gtk_box_append(GTK_BOX(guest_booking_box), guest_preview_box);
    gtk_box_append(GTK_BOX(outer), guest_booking_box);

    return outer;
}

/* ===================================================================
 *  APPLICATION ENTRY POINT
 * =================================================================== */

/* -----------------------------------------------------------------------
 *  Reset the entire guest panel back to the login screen.
 *
 *  Called whenever the user navigates away from or back to the guest page.
 *  Guarantees every visit starts from a completely blank slate.
 * --------------------------------------------------------------------- */
static void reset_guest_panel(void) {
    /* Clear every text field */
    gtk_editable_set_text(GTK_EDITABLE(guest_name_entry),        "");
    gtk_editable_set_text(GTK_EDITABLE(guest_start_hour_entry),  "");
    gtk_editable_set_text(GTK_EDITABLE(guest_start_min_entry),   "");
    gtk_editable_set_text(GTK_EDITABLE(guest_duration_entry),    "");
    gtk_editable_set_text(GTK_EDITABLE(guest_spot_entry),        "");

    /* Clear all status / result labels */
    gtk_label_set_text(GTK_LABEL(guest_login_error),   "");
    gtk_label_set_text(GTK_LABEL(guest_lookup_label),  "");
    gtk_label_set_text(GTK_LABEL(guest_cancel_label),  "");
    gtk_label_set_text(GTK_LABEL(guest_result_label),  "");

    /* Remove any dynamic cancel-list rows built at runtime */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(guest_cancel_list_box)) != NULL)
        gtk_box_remove(GTK_BOX(guest_cancel_list_box), child);

    /* Hide booking/preview phases; restore the login phase */
    gtk_widget_set_visible(guest_spot_grid_box, FALSE);
    gtk_widget_set_visible(guest_booking_box, FALSE);
    gtk_widget_set_visible(guest_preview_box, FALSE);
    gtk_widget_set_visible(guest_login_box,   TRUE);

    /* Reset spot-grid counters to placeholder dashes */
    gtk_label_set_markup(GTK_LABEL(guest_free_label),
        "<span foreground='#16a34a' weight='bold'>🟢  Free: —</span>");
    gtk_label_set_markup(GTK_LABEL(guest_occupied_label),
        "<span foreground='#dc2626' weight='bold'>🔴  Occupied: —</span>");

    /* Discard any pending (unconfirmed) booking */
    g_has_pending = FALSE;
}

/* Navigate to a named page in the main stack.
 * Resets the guest panel on every leave/enter so each visit is fresh.   */
static void go_to_page(GtkButton *btn, gpointer page_name) {
    const char *dest = (const char *)page_name;

    /* Leaving the guest page → wipe state for the next visitor */
    const char *current =
        gtk_stack_get_visible_child_name(GTK_STACK(g_main_stack));
    if (current && g_strcmp0(current, "guest") == 0)
        reset_guest_panel();

    gtk_stack_set_visible_child_name(GTK_STACK(g_main_stack), dest);

    /* Arriving at the guest page → also reset (Home → Guest path) */
    if (g_strcmp0(dest, "guest") == 0)
        reset_guest_panel();
}

/* Build the Home panel */
static GtkWidget *create_home_page(void) {
    /* parking_bg.jpg fills the background; two panel buttons overlay it */
    GtkWidget *overlay = gtk_overlay_new();

    GtkWidget *bg = gtk_picture_new_for_filename("parking_bg.jpg");
    gtk_picture_set_content_fit(GTK_PICTURE(bg), GTK_CONTENT_FIT_COVER);
    gtk_widget_set_hexpand(bg, TRUE);
    gtk_widget_set_vexpand(bg, TRUE);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), bg);

    /* Card with title + buttons */
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(card, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(card, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(card, 380, -1);
    gtk_widget_add_css_class(card, "park-home-card");

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span font='22' weight='bold' foreground='#c8a03c'>"
        "Parking Management</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);

    GtkWidget *sub = gtk_label_new("Smooth Arrivals, Organized Parking");
    gtk_widget_add_css_class(sub, "park-home-sub");
    gtk_widget_set_halign(sub, GTK_ALIGN_CENTER);

    /* Button row */
    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 30);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_CENTER);

    /* Guest button */
    GtkWidget *guest_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(guest_col, GTK_ALIGN_CENTER);
    GtkWidget *guest_icon = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(guest_icon), "<span font='36'>ð</span>");
    GtkWidget *guest_btn = gtk_button_new_with_label("  Guest Panel  ");
    gtk_widget_add_css_class(guest_btn, "park-panel-btn");
    g_signal_connect(guest_btn, "clicked", G_CALLBACK(go_to_page), "guest");
    gtk_box_append(GTK_BOX(guest_col), guest_icon);
    gtk_box_append(GTK_BOX(guest_col), guest_btn);

    /* Admin button */
    GtkWidget *admin_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(admin_col, GTK_ALIGN_CENTER);
    GtkWidget *admin_icon = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(admin_icon), "<span font='36'>ð§</span>");
    GtkWidget *admin_btn = gtk_button_new_with_label("  Admin Panel  ");
    gtk_widget_add_css_class(admin_btn, "park-panel-btn");
    g_signal_connect(admin_btn, "clicked", G_CALLBACK(go_to_page), "admin");
    gtk_box_append(GTK_BOX(admin_col), admin_icon);
    gtk_box_append(GTK_BOX(admin_col), admin_btn);

    gtk_box_append(GTK_BOX(btn_row), guest_col);
    gtk_box_append(GTK_BOX(btn_row), admin_col);

    gtk_box_append(GTK_BOX(card), title);
    gtk_box_append(GTK_BOX(card), sub);
    gtk_box_append(GTK_BOX(card), btn_row);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), card);
    return overlay;
}

/* Stored window handle so fullscreen works from both panels */
static GtkWidget *g_park_win = NULL;

/* Wrap a panel page with "â Home" and "â¶" fullscreen buttons at the top */
static GtkWidget *wrap_with_back_button(GtkWidget *page) {
    GtkWidget *wrapper = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(top_bar, 8);
    gtk_widget_set_margin_top(top_bar, 6);
    gtk_widget_set_margin_bottom(top_bar, 4);

    GtkWidget *back_btn = gtk_button_new_with_label("â Home");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(go_to_page), "home");
    gtk_box_append(GTK_BOX(top_bar), back_btn);

    /* Spacer pushes fullscreen btn to the right */
    GtkWidget *spacer = gtk_label_new("");
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(top_bar), spacer);

    GtkWidget *fs_btn = gtk_button_new_with_label("â¶");
    gtk_widget_set_tooltip_text(fs_btn, "Toggle Fullscreen");
    g_signal_connect_swapped(fs_btn, "clicked",
        G_CALLBACK(gtk_window_fullscreen), g_park_win);
    gtk_box_append(GTK_BOX(top_bar), fs_btn);

    gtk_box_append(GTK_BOX(wrapper), top_bar);
    gtk_box_append(GTK_BOX(wrapper), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_widget_set_vexpand(page, TRUE);
    gtk_box_append(GTK_BOX(wrapper), page);

    return wrapper;
}

static void activate(GtkApplication *app, gpointer data) {
    GtkWidget *win = gtk_application_window_new(app);
    g_park_win = win;   /* store for fullscreen callbacks */
    gtk_window_set_title(GTK_WINDOW(win), "Parking Management System");
    gtk_window_set_default_size(GTK_WINDOW(win), 800, 560);

    /* CSS for home card and panel buttons */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        ".park-home-card {"
        "  background-color: rgba(10,15,40,0.82);"
        "  border-radius: 16px;"
        "  padding: 36px 40px;"
        "  border: 1px solid rgba(200,160,60,0.5);"
        "}"
        ".park-home-sub {"
        "  font-size: 13px;"
        "  color: #d4c89a;"
        "}"
        ".park-panel-btn {"
        "  background-color: #c8a03c;"
        "  color: #0a0f28;"
        "  font-weight: bold;"
        "  border-radius: 8px;"
        "  padding: 10px 20px;"
        "}"
        ".park-panel-btn:hover { background-color: #e0b84a; }",
        -1);
    gtk_style_context_add_provider_for_display(
        gtk_widget_get_display(win),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    g_main_stack = gtk_stack_new();
    gtk_widget_set_hexpand(g_main_stack, TRUE);
    gtk_widget_set_vexpand(g_main_stack, TRUE);

    gtk_stack_add_named(GTK_STACK(g_main_stack),
                        create_home_page(), "home");
    gtk_stack_add_named(GTK_STACK(g_main_stack),
                        wrap_with_back_button(create_guest_page()), "guest");
    gtk_stack_add_named(GTK_STACK(g_main_stack),
                        wrap_with_back_button(create_admin_page()), "admin");

    gtk_stack_set_visible_child_name(GTK_STACK(g_main_stack), "home");

    gtk_window_set_child(GTK_WINDOW(win), g_main_stack);
    gtk_window_present(GTK_WINDOW(win));
}

int main(int argc, char **argv) {
    GtkApplication *app =
        gtk_application_new("org.example.parkingmanager",
                            G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
