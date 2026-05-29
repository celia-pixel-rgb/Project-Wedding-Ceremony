/* parking.h
 * Parking Management System – CLI version
 *
 * Header: constants, types, and function declarations.
 */

#ifndef PARKING_H
#define PARKING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Constants                                                           */
/* ------------------------------------------------------------------ */
#define PASSWORD          "group3wed!"
#define SPOTS_FILE        "parking_spot.csv"
#define BOOKINGS_FILE     "parking_booking.csv"
#define PERSONS_FILE      "persons.csv"

#define MAX_SPOTS         200   /* hard upper limit on number of spots  */
#define MAX_NAME_LEN       64   /* max characters in a guest name       */
#define MAX_BOOKINGS     1000   /* max rows loaded from bookings CSV    */

/* ------------------------------------------------------------------ */
/*  Data types                                                          */
/* ------------------------------------------------------------------ */

/* Spot configuration written/read by the admin */
typedef struct {
    int spot_id;       /* 1-based identifier  */
    int max_hours;     /* maximum booking duration for this spot */
} SpotConfig;

/* A single parking reservation */
typedef struct {
    int  booking_id;
    int  spot_id;
    char guest_name[MAX_NAME_LEN];
    int  start_hour;       /* 0–23 */
    int  start_min;        /* 0–59 */
    int  duration_hours;
    int  end_hour;
    int  end_min;
} Booking;

/* ------------------------------------------------------------------ */
/*  Global state (defined in parking.c)                                */
/* ------------------------------------------------------------------ */
extern int g_num_spots;   /* total spots configured by admin  */
extern int g_max_hours;   /* maximum booking duration (hours) */

/* ------------------------------------------------------------------ */
/*  Time utilities                                                      */
/* ------------------------------------------------------------------ */
int  to_minutes(int h, int m);
void add_hours(int sh, int sm, int dur_h, int *eh, int *em);
int  intervals_overlap(int as, int ae, int bs, int be);

/* ------------------------------------------------------------------ */
/*  CSV persistence                                                     */
/* ------------------------------------------------------------------ */
void     save_spot_config(void);
int      load_spot_config(void);           /* returns 1 on success, 0 on failure */
void     append_booking(const Booking *b);
int      next_booking_id(void);
Booking *load_bookings(int *out_count);    /* caller must free() the result       */

/* ------------------------------------------------------------------ */
/*  Business logic                                                      */
/* ------------------------------------------------------------------ */

/*
 * check_person_parking – look up a name in persons.csv
 * Returns:
 *   0  – name not found
 *   1  – found, but parking not enabled
 *   2  – found and parking enabled
 */
int check_person_parking(const char *name);

/*
 * guest_has_existing_booking – one-booking-per-guest enforcement.
 * Returns 1 if the guest already holds ANY booking; fills *out when found.
 * req_start / req_end are accepted for API compatibility but not used.
 */
int guest_has_existing_booking(const char *name, Booking *out,
                               int req_start, int req_end);

/*
 * find_available_spot – returns the first spot number free for the window,
 * or -1 if every spot is occupied.
 */
int find_available_spot(int req_start, int req_end);

/*
 * is_spot_available – checks one specific spot for the requested window.
 * Returns 1 if free, 0 if occupied or out of range.
 */
int is_spot_available(int spot, int req_start, int req_end);

/*
 * cancel_booking_by_id – removes the row with booking_id from the CSV.
 * Returns 1 on success, 0 if not found or write failed.
 */
int cancel_booking_by_id(int target_id);

/* ------------------------------------------------------------------ */
/*  Menu / panel entry points                                           */
/* ------------------------------------------------------------------ */
void run_admin_panel(void);
void run_guest_panel(void);

/*
 * guest_view_spots – prints the full spot status table for guests.
 * Same layout as the admin view but guest names are hidden;
 * occupied cards show only the booked time slot(s).
 */
void guest_view_spots(void);

#endif /* PARKING_H */
