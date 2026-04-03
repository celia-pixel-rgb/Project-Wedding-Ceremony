/*
 * newperson.c  --  Wedding Guest Manager (improved version)
 * Original author: group3wed
 *
 * CSV row format:
 *   <id>,<name>,<age>,<status>,<phone>,<side>,<parking>
 *   Example: 0,John Smith,35,VIP,612345678,0,Yes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

static const char *CSV_FILE = "guests.csv";
static const char *TMP_FILE = "temp.csv";
static const char *PASSWORD = "group3wed!";

const char *side_to_string(Side s) {
    return (s == GROOM) ? "Groom" : (s == BRIDE) ? "Bride" : "Both";
}

/*
 * flush_stdin -- Discard leftover characters in the input buffer.
 * Called after a read to prevent stale input from corrupting the next read.
 */
static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void read_line(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    fflush(stdout);                      /* Force the prompt to appear before the user types */
    if (fgets(buf, size, stdin)) {
        if (buf[strlen(buf) - 1] != '\n') {
            /* The input was longer than the buffer -- flush the rest */
            flush_stdin();
        }
        buf[strcspn(buf, "\n")] = '\0';  /* strcspn finds the '\n' position and replaces it with '\0' */
    }
}

static int read_int(const char *prompt) {
    char buf[32];
    read_line(prompt, buf, sizeof(buf));
    return atoi(buf);
}

static int check_password(void) {
    char pw[64];
    read_line("Password: ", pw, sizeof(pw));
    return strcmp(pw, PASSWORD) == 0;
}

static int is_all_digits(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; s++) if (*s < '0' || *s > '9') return 0;
    return 1;
}

static int is_valid_phone(const char *p) {
    if (strlen(p) != 9) return 0;
    if (!is_all_digits(p)) return 0;
    return (p[0] == '6' || p[0] == '7' || p[0] == '8' || p[0] == '9');
}

static int is_valid_age(const char *a) {
    if (!is_all_digits(a)) return 0;
    int v = atoi(a);
    return (v >= 1 && v <= 120);
}

static int is_valid_status(const char *s) {
    char lower[50];
    int i;
    /* Convert to lowercase so the check works regardless of how the user typed it
       e.g. "VIP", "vip", "Vip" are all accepted */
    for (i = 0; s[i] && i < 49; i++)
        lower[i] = (char)tolower((unsigned char)s[i]);
    lower[i] = '\0';
    return (strcmp(lower, "vip")     == 0 ||
            strcmp(lower, "regular") == 0 ||
            strcmp(lower, "guest")   == 0);
}

static int get_next_id(void) {
    FILE *file = fopen(CSV_FILE, "r");
    if (!file) return 0;   /* File does not exist yet -- first guest gets ID 0 */

    int max_id = -1, id;
    char line[512];
    /* Scan every line and track the highest ID seen */
    while (fgets(line, sizeof(line), file))
        if (sscanf(line, "%d,", &id) == 1 && id > max_id)
            max_id = id;

    fclose(file);
    return max_id + 1;   /* Next ID is one above the current maximum */
}

/*
 * write_guest_line / parse_guest_line -- The CSV format is defined in one
 * place each so that if it ever changes, only these two functions need updating.
 */
static void write_guest_line(FILE *f, const Guest *g) {
    fprintf(f, "%d,%s,%d,%s,%s,%d,%s\n",
            g->id, g->name, g->age, g->status,
            g->phone, (int)g->side, g->parking);
}

static int parse_guest_line(const char *line, Guest *g) {
    /* [^,] means "read characters until a comma" -- handles fields that contain spaces */
    return sscanf(line,
                  "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                  &g->id, g->name, &g->age, g->status,
                  g->phone, (int *)&g->side, g->parking) == 7;
}

static Side prompt_side(void) {
    printf("Side: 1=Groom  2=Bride  3=Both\n");
    int s = read_int("Choice: ");
    if (s == 1) return GROOM;
    if (s == 2) return BRIDE;
    return BOTH;
}

static void prompt_parking(char *buf, int size) {
    printf("Parking? 1=Yes  2=No\n");
    int p = read_int("Choice: ");
    strncpy(buf, (p == 1) ? "Yes" : "No", size - 1);
    buf[size - 1] = '\0';  /* strncpy does not guarantee null-termination when truncating */
}

static void action_add_guest(void) {
    Guest g;
    /* [FIX] Increased age_str buffer from 10 to 32 to prevent overflow
       corrupting the next read_line call */
    char name[100], age_str[32], status[50], phone[20];

    read_line("Full Name: ", name, sizeof(name));
    if (strlen(name) < 2) { printf("Name too short.\n"); return; }

    read_line("Age: ", age_str, sizeof(age_str));
    if (!is_valid_age(age_str)) { printf("Invalid age (must be 1-120).\n"); return; }

    read_line("Status (VIP/Regular/Guest): ", status, sizeof(status));
    if (!is_valid_status(status)) {
        printf("Invalid status. Must be VIP, Regular or Guest.\n");
        return;
    }

    read_line("Phone (9 digits, start 6-9): ", phone, sizeof(phone));
    if (!is_valid_phone(phone)) {
        printf("Invalid phone (must be 9 digits starting with 6, 7, 8 or 9).\n");
        return;
    }

    g.id  = get_next_id();
    g.age = atoi(age_str);
    strncpy(g.name,   name,   sizeof(g.name)   - 1); g.name[sizeof(g.name)   - 1] = '\0';
    strncpy(g.status, status, sizeof(g.status) - 1); g.status[sizeof(g.status) - 1] = '\0';
    strncpy(g.phone,  phone,  sizeof(g.phone)  - 1); g.phone[sizeof(g.phone)  - 1] = '\0';
    g.side = prompt_side();
    prompt_parking(g.parking, sizeof(g.parking));

    FILE *f = fopen(CSV_FILE, "a");  /* "a" = append mode: adds to the end without overwriting */
    if (!f) {
        /* [FIX] Print the exact system error so the user knows the real cause */
        perror("Could not open guests.csv");
        return;
    }
    write_guest_line(f, &g);
    fclose(f);

    printf("Saved guest ID: %d\n", g.id);
}

static void action_show_guests(void) {
    if (!check_password()) { printf("Incorrect password!\n"); return; }

    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { printf("No guests found yet.\n"); return; }

    printf("\n| %-3s | %-20s | %-3s | %-8s | %-11s | %-6s | %-7s |\n",
           "ID", "Name", "Age", "Status", "Phone", "Side", "Parking");
    printf("|-----|----------------------|-----|----------|-------------|--------|--------|\n");

    char  line[512];
    Guest g;
    int   count = 0;

    while (fgets(line, sizeof(line), f)) {
        if (!parse_guest_line(line, &g)) continue;  /* Skip any malformed lines */
        printf("| %-3d | %-20s | %-3d | %-8s | %-11s | %-6s | %-7s |\n",
               g.id, g.name, g.age, g.status, g.phone,
               side_to_string(g.side), g.parking);
        count++;
    }
    fclose(f);
    printf("\nTotal guests: %d\n\n", count);
}

static int load_guest(int target, Guest *out) {
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (!parse_guest_line(line, out)) continue;
        if (out->id == target) { fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}

static void action_delete_guest(void) {
    if (!check_password()) { printf("Incorrect password!\n"); return; }

    int target = read_int("Guest ID to delete: ");
    if (target < 0) { printf("Invalid ID.\n"); return; }

    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { printf("No guests found yet.\n"); return; }

    FILE *tmp = fopen(TMP_FILE, "w");
    if (!tmp) {
        /* [FIX] Print the exact system error */
        perror("Could not open temp file");
        fclose(f);
        return;
    }

    char  line[512];
    Guest g;
    int   found = 0;

    /* Copy every row except the one to delete into the temp file */
    while (fgets(line, sizeof(line), f)) {
        if (!parse_guest_line(line, &g)) continue;
        if (g.id != target)
            write_guest_line(tmp, &g);
        else
            found = 1;
    }
    fclose(f);
    fclose(tmp);

    /* Replace the original file with the temp file atomically.
       This prevents data loss if the program crashes mid-write. */
    remove(CSV_FILE);
    if (rename(TMP_FILE, CSV_FILE) != 0)
        perror("Warning: could not rename temp file");

    if (found) printf("Deleted guest ID: %d\n", target);
    else       printf("ID not found.\n");
}

static void action_update_guest(void) {
    if (!check_password()) { printf("Incorrect password!\n"); return; }

    int target = read_int("Guest ID to update: ");
    Guest g;
    if (!load_guest(target, &g)) { printf("ID not found.\n"); return; }

    printf("Current: %s | Age %d | %s | %s | %s | Parking: %s\n",
           g.name, g.age, g.status, g.phone,
           side_to_string(g.side), g.parking);
    printf("(Leave blank to keep current value)\n");

    char buf[100];

    read_line("New name: ", buf, sizeof(buf));
    if (strlen(buf) >= 2)
        strncpy(g.name, buf, sizeof(g.name) - 1);
    else if (strlen(buf) > 0)
        printf("Name too short -- kept original.\n");
    /* strlen == 0 means the user pressed Enter: silently keep the original */

    read_line("New age: ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (is_valid_age(buf)) g.age = atoi(buf);
        else printf("Invalid age -- kept original.\n");
    }

    read_line("New status (VIP/Regular/Guest): ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (is_valid_status(buf))
            strncpy(g.status, buf, sizeof(g.status) - 1);
        else
            printf("Invalid status -- kept original.\n");
    }

    read_line("New phone: ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (is_valid_phone(buf))
            strncpy(g.phone, buf, sizeof(g.phone) - 1);
        else
            printf("Invalid phone -- kept original.\n");
    }

    printf("Change side? 0=No  1=Yes\n");
    if (read_int("Choice: ") == 1) g.side = prompt_side();

    printf("Change parking? 0=No  1=Yes\n");
    if (read_int("Choice: ") == 1) prompt_parking(g.parking, sizeof(g.parking));

    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { printf("No guests found yet.\n"); return; }

    FILE *tmp = fopen(TMP_FILE, "w");
    if (!tmp) {
        perror("Could not open temp file");
        fclose(f);
        return;
    }

    char  line[512];
    Guest cur;
    int   found = 0;

    /* Rewrite the CSV: replace the matching row with the updated record,
       copy all other rows unchanged */
    while (fgets(line, sizeof(line), f)) {
        if (!parse_guest_line(line, &cur)) continue;
        if (cur.id == target) {
            write_guest_line(tmp, &g);   /* Write updated guest */
            found = 1;
        } else {
            write_guest_line(tmp, &cur); /* Copy unchanged guest */
        }
    }
    fclose(f);
    fclose(tmp);

    /* Same safe-replace technique as in action_delete_guest */
    remove(CSV_FILE);
    if (rename(TMP_FILE, CSV_FILE) != 0)
        perror("Warning: could not rename temp file");

    if (found) printf("Updated guest ID: %d\n", target);
    else       printf("ID not found.\n");
}

int main(void) {
    int choice;
    do {
        printf("\nWedding Guest Manager\n");
        printf("1. Add guest\n");
        printf("2. Display all guests\n");
        printf("3. Delete guest\n");
        printf("4. Update guest\n");
        printf("0. Exit\n");
        choice = read_int("Choice: ");

        switch (choice) {
            case 1: action_add_guest();    break;
            case 2: action_show_guests();  break;
            case 3: action_delete_guest(); break;
            case 4: action_update_guest(); break;
            case 0: printf("Goodbye!\n");  break;
            default: printf("Invalid choice.\n"); break;
        }
    } while (choice != 0);

    return 0;
}
