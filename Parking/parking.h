/* =============================================================================
 * parking.h  –  Parking Management System  (Command-Line Version)
 *
 * This header declares all constants, data types, and function prototypes
 * used by parking.c.  It is the single shared interface between the
 * application logic and any future module that needs to interact with the
 * parking system (e.g. the person module for auto-cancellation).
 *
 * CSV files used (shared with the GTK person module):
 *   parking_spot.csv    – admin configuration  (num_spots, max_hours)
 *   parking_booking.csv – guest reservations
 *   persons.csv         – guest registry  (read-only from this module)
 * ============================================================================= */

#ifndef PARKING_H
#define PARKING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------------
 * FILE PATHS
 * These must match the filenames used by gtkperson.c so both programs
 * operate on the same data.
 * ----------------------------------------------------------------------------- */
#define SPOTS_FILE        "parking_spot.csv"
#define BOOKINGS_FILE     "parking_booking.csv"
#define PERSONS_FILE      "persons.csv"

/* -----------------------------------------------------------------------------
 * SYSTEM LIMITS
 * ----------------------------------------------------------------------------- */
#define MAX_SPOTS        200   /* hard upper limit on parking spots             */
#define MAX_NAME_LEN      64   /* maximum characters in a guest name            */
#define MAX_LINE_LEN     512   /* maximum characters in a CSV line              */

/* -----------------------------------------------------------------------------
 * ADMIN PASSWORD
 * Required to access admin functions (configure spots, view status).
 * Must match the password used in gtkperson.c.
 * ----------------------------------------------------------------------------- */
#define PASSWORD  "group3wed!"

/* -----------------------------------------------------------------------------
 * RETURN CODES  –  used by check_person_parking()
 * ----------------------------------------------------------------------------- */
#define PERSON_NOT_FOUND       0   /* name not in persons.csv                   */
#define PERSON_NO_PARKING      1   /* registered but parking = "No"             */
#define PERSON_HAS_PARKING     2   /* registered and parking = "Yes"            */

/* =============================================================================
 * DATA TYPES
 * ============================================================================= */

/*
 * SpotConfig  –  global parking lot settings saved by the admin.
 * Stored as a single row in parking_spot.csv: num_spots,max_hours
 */
typedef struct {
    int spot_id;    /* 1-based spot number (1 … num_spots)                      */
    int max_hours;  /* maximum booking duration allowed for this spot           */
} SpotConfig;

/*
 * Booking  –  one guest reservation stored in parking_booking.csv.
 *
 * CSV row format:
 *   booking_id, spot_id, guest_name, start_hour, start_min,
 *   duration_hours, end_hour, end_min
 */
typedef struct {
    int  booking_id;                  /* unique auto-incremented identifier      */
    int  spot_id;                     /* which spot this booking is for          */
    char guest_name[MAX_NAME_LEN];    /* full name as registered in persons.csv  */
    int  start_hour;                  /* booking start  – hour  (0-23)           */
    int  start_min;                   /* booking start  – minute (0-59)          */
    int  duration_hours;              /* length of the booking in whole hours    */
    int  end_hour;                    /* computed end   – hour  (0-23)           */
    int  end_min;                     /* computed end   – minute (0-59)          */
} Booking;

/* =============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================= */

/* --- Time utilities --------------------------------------------------------- */

/*
 * to_minutes()
 * Convert an hour:minute pair to a single integer (minutes since midnight).
 * Example: to_minutes(9, 30) == 570
 */
int to_minutes(int h, int m);

/*
 * add_hours()
 * Add dur_h hours to a start time (sh:sm), wrapping around midnight.
 * Results are written into *eh (end hour) and *em (end minute).
 */
void add_hours(int sh, int sm, int dur_h, int *eh, int *em);

/*
 * intervals_overlap()
 * Returns 1 if time interval [as, ae) overlaps [bs, be), 0 otherwise.
 * All values are in minutes since midnight.
 * Handles wrap-around past midnight correctly.
 */
int intervals_overlap(int as, int ae, int bs, int be);

/* --- CSV persistence -------------------------------------------------------- */

/*
 * save_spot_config()
 * Write the global spot count and max-hours setting to parking_spot.csv.
 * Called whenever the admin changes the configuration.
 */
void save_spot_config(int num_spots, int max_hours);

/*
 * load_spot_config()
 * Read num_spots and max_hours from parking_spot.csv.
 * Returns 1 on success, 0 if the file does not exist or cannot be parsed.
 * Results are written into *num_spots and *max_hours.
 */
int load_spot_config(int *num_spots, int *max_hours);

/*
 * append_booking()
 * Append one Booking record as a new CSV row to parking_booking.csv.
 */
void append_booking(const Booking *b);

/*
 * next_booking_id()
 * Scan parking_booking.csv and return max(booking_id) + 1.
 * Returns 1 if the file is empty or does not exist yet.
 */
int next_booking_id(void);

/*
 * load_bookings()
 * Read all rows from parking_booking.csv into a dynamically allocated array.
 * *out_count is set to the number of valid bookings read.
 * The caller is responsible for calling free() on the returned pointer.
 * Returns NULL if the file does not exist or is empty.
 */
Booking *load_bookings(int *out_count);

/*
 * cancel_booking_by_id()
 * Remove the booking row with the given booking_id from parking_booking.csv.
 * All other rows are preserved unchanged.
 * Returns 1 if the booking was found and removed, 0 otherwise.
 */
int cancel_booking_by_id(int booking_id);

/*
 * cancel_bookings_for_guest()
 * Remove ALL booking rows belonging to guest_name from parking_booking.csv.
 * Used when a guest's parking is revoked in the person module.
 * Returns the number of rows removed (0 = no booking found).
 */
int cancel_bookings_for_guest(const char *guest_name);

/* --- persons.csv lookup ----------------------------------------------------- */

/*
 * check_person_parking()
 * Look up 'name' in persons.csv (case-insensitive).
 * Returns:
 *   PERSON_NOT_FOUND   (0) – name not in the registry
 *   PERSON_NO_PARKING  (1) – registered but parking = "No"
 *   PERSON_HAS_PARKING (2) – registered and parking = "Yes"
 */
int check_person_parking(const char *name);

/* --- Availability logic ----------------------------------------------------- */

/*
 * find_available_spot()
 * Search spots 1 … num_spots for one whose existing bookings do NOT overlap
 * the requested window [req_start, req_end) (both in minutes since midnight).
 * Returns the spot number (1-based) if one is free, or -1 if all are taken.
 */
int find_available_spot(int req_start, int req_end, int num_spots);

/*
 * guest_has_overlapping_booking()
 * Returns 1 if 'name' already has a booking that overlaps [req_start, req_end).
 * If out != NULL and a conflict is found, *out is filled with that booking.
 * Returns 0 if no overlap exists (the guest may book the requested window).
 */
int guest_has_overlapping_booking(const char *name, Booking *out,
                                  int req_start, int req_end);

/* --- Display helpers -------------------------------------------------------- */

/*
 * print_separator()
 * Print a horizontal line of dashes for visual separation in the CLI.
 */
void print_separator(void);

/*
 * print_all_spots()
 * Display the status of every configured spot:
 *   - FREE  if no booking exists for that spot
 *   - OCCUPIED with guest name(s) and time window(s) if booked
 */
void print_all_spots(int num_spots);

/*
 * print_guest_bookings()
 * List all bookings in parking_booking.csv that belong to guest_name.
 * Prints a "no bookings" message if none are found.
 */
void print_guest_bookings(const char *guest_name);

/* --- Menu entry points ------------------------------------------------------ */

/*
 * run_admin_menu()
 * Password-protected interactive menu for the admin:
 *   1. Configure number of spots and max hours
 *   2. View spot occupation status
 *   3. Cancel any booking by ID
 *   4. Exit admin menu
 */
void run_admin_menu(void);

/*
 * run_guest_menu()
 * Open interactive menu for a verified guest:
 *   1. Book a parking slot
 *   2. View my bookings
 *   3. Cancel one of my bookings
 *   4. Exit guest menu
 */
void run_guest_menu(const char *guest_name, int num_spots, int max_hours);

#endif /* PARKING_H */
