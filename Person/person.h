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

/* Utility */
// Function Declarations (The Prototypes)
// We only tell the computer these exist here. 
// The actual logic goes into person.c
const char *side_to_string(Side s);

/* CRUD actions (called from person.c's own main) */
void action_add_guest(void);
void action_show_guests(void);
void action_delete_guest(void);
void action_update_guest(void);

/* Used by category.c to look up a guest record */
int get_next_id(void);

#endif /* PERSON_H */
