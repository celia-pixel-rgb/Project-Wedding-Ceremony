/*
 * gift.h  --  Wedding Gift Management System
 * Shared types, constants, and function declarations.
 *
 * CSV row format (gifts.csv):
 *   <id>,<guest_name>,<guest_id>,<category>,<gift_name>,<quantity>,<unit_price_fcfa>,<total_fcfa>
 *   Example: 0,John Smith,3,VIP,Television,1,150000,150000
 *
 * Monetary note:
 *   1 EUR = 655.957 FCFA  (fixed CFA franc rate)
 */

#ifndef GIFT_H
#define GIFT_H

/* ------------------------------------------------------------------ */
/*  Files                                                               */
/* ------------------------------------------------------------------ */
#define GIFT_CSV_FILE   "gifts.csv"
#define GIFT_TMP_FILE   "gifts_tmp.csv"
#define GUEST_CSV_FILE  "guests.csv"   /* written by newperson.c  */
#define CAT_CSV_FILE    "categories.csv" /* written by category.c */
#define PASSWORD        "group3wed!"

/* ------------------------------------------------------------------ */
/*  Currency conversion                                                 */
/* ------------------------------------------------------------------ */
#define EUR_TO_FCFA     655.957

/* ------------------------------------------------------------------ */
/*  Gift catalogue                                                      */
/*  Prices are moderate/mid-range estimates in FCFA (Central Africa).  */
/*  1 EUR ≈ 655.957 FCFA                                               */
/* ------------------------------------------------------------------ */
#define NUM_GIFT_TYPES  26   /* 25 physical gifts + 1 "money" option  */

typedef struct {
    const char *name;
    long        price_fcfa;   /* unit price */
} GiftType;

/* Catalogue – defined in gift.c */
extern const GiftType GIFT_CATALOGUE[NUM_GIFT_TYPES];

/* ------------------------------------------------------------------ */
/*  Gift record                                                         */
/* ------------------------------------------------------------------ */
typedef struct {
    int  id;
    char guest_name[100];
    int  guest_id;           /* id from guests.csv or categories.csv  */
    char category[50];       /* e.g. "VIP", "Regular", "Guest"        */
    char gift_name[100];
    int  quantity;
    long unit_price_fcfa;
    long total_fcfa;
} Gift;

/* ------------------------------------------------------------------ */
/*  Terminal-mode public API  (used by gift.c main + GTK layer)        */
/* ------------------------------------------------------------------ */
int  check_password_gift(void);      /* returns 1 if correct, 0 if not */
int  verify_guest_name(const char *name, int *out_id, char *out_category, int cat_size);

void action_add_gift(void);
void action_display_gifts(void);
void action_delete_gift(void);
void action_update_gift(void);

/* Low-level helpers used by the GTK layer */
int  gift_get_next_id(void);
int  gift_load_all(Gift **out_array);   /* caller must free() */
int  gift_save_one(const Gift *g);      /* append */
int  gift_delete_by_id(int id);
int  gift_update_by_id(int id, const Gift *updated);

#endif /* GIFT_H */
