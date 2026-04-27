#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

const char *CSV_FILE = "guests.csv";
const char *PASSWORD = "group3wed!";

const char* side_to_string(Side s) {
    return (s == GROOM) ? "Groom" : (s == BRIDE) ? "Bride" : "Both";
}


static void read_line(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, size, stdin))
        buf[strcspn(buf, "\n")] = '\0';
}

static int read_int(const char *prompt) {
    char buf[32];
    read_line(prompt, buf, sizeof(buf));
    return atoi(buf);
}

static int check_password() {
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
    return (p[0]=='6' || p[0]=='7' || p[0]=='8' || p[0]=='9');
}

static int is_valid_age(const char *a) {
    if (!is_all_digits(a)) return 0;
    int v = atoi(a);
    return (v >= 1 && v <= 120);
}


int get_next_id() {
    FILE *file = fopen(CSV_FILE, "r");
    if (!file) return 0;
    int max_id = -1, id;
    char line[512];
    while (fgets(line, sizeof(line), file))
        if (sscanf(line, "%d,", &id) == 1 && id > max_id) max_id = id;
    fclose(file);
    return max_id + 1;
}

static Side prompt_side() {
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
    buf[size - 1] = '\0';
}


static void action_add_guest() {
    Guest g;

    char name[100], age_str[10], status[50], phone[20];

    read_line("Full Name: ", name, sizeof(name));
    if (strlen(name) < 2) { printf("Name too short.\n"); return; }

    read_line("Age: ", age_str, sizeof(age_str));
    if (!is_valid_age(age_str)) { printf("Invalid age (must be 1-120).\n"); return; }

    read_line("Status (VIP/Regular/Guest): ", status, sizeof(status));

    read_line("Phone (9 digits, start 6-9): ", phone, sizeof(phone));
    if (!is_valid_phone(phone)) {
        printf("Invalid phone (must be 9 digits starting with 6, 7, 8 or 9).\n");
        return;
    }

    g.id  = get_next_id();
    g.age = atoi(age_str);
    strncpy(g.name,   name,   sizeof(g.name)-1);   g.name[sizeof(g.name)-1]   = '\0';
    strncpy(g.status, status, sizeof(g.status)-1); g.status[sizeof(g.status)-1] = '\0';
    strncpy(g.phone,  phone,  sizeof(g.phone)-1);  g.phone[sizeof(g.phone)-1]  = '\0';
    g.side = prompt_side();
    prompt_parking(g.parking, sizeof(g.parking));

    FILE *f = fopen(CSV_FILE, "a");
    if (!f) { printf("File error.\n"); return; }
    fprintf(f, "%d,%s,%d,%s,%s,%d,%s\n",
            g.id, g.name, g.age, g.status, g.phone, (int)g.side, g.parking);
    fclose(f);

    printf("Saved guest ID: %d\n", g.id);
}


static void action_show_guests() {
    if (!check_password()) { printf("Incorrect password!\n"); return; }

    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { printf("No data.\n"); return; }

    printf("\n| ID | %-16s | Age | %-8s | %-10s | %-5s | Parking |\n",
           "Name", "Status", "Phone", "Side");
    printf("|-----|------------------|-----|----------|------------|-------|--------|\n");

    char line[512];
    Guest g;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &g.id, g.name, &g.age, g.status, g.phone,
                   (int*)&g.side, g.parking) != 7)
            continue;
        printf("| %-3d | %-16s | %-3d | %-8s | %-10s | %-5s | %-6s |\n",
               g.id, g.name, g.age, g.status, g.phone,
               side_to_string(g.side), g.parking);
    }
    fclose(f);
    printf("\n");
}




static int load_guest(int target, Guest *out) {
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &out->id, out->name, &out->age, out->status,
                   out->phone, (int*)&out->side, out->parking) != 7)
            continue;
        if (out->id == target) { fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}



static void action_delete_guest() {
    if (!check_password()) { printf("Incorrect password!\n"); return; }

    int target = read_int("Guest ID to delete: ");
    if (target < 0) { printf("Invalid ID.\n"); return; }

    FILE *f   = fopen(CSV_FILE,  "r"); if (!f)   return;
    FILE *tmp = fopen("temp.csv","w"); if (!tmp) { fclose(f); return; }

    char line[512];
    Guest g;
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &g.id, g.name, &g.age, g.status, g.phone,
                   (int*)&g.side, g.parking) != 7)
            continue;
        if (g.id != target)
            fprintf(tmp, "%d,%s,%d,%s,%s,%d,%s\n",
                    g.id, g.name, g.age, g.status, g.phone, (int)g.side, g.parking);
        else
            found = 1;
    }
    fclose(f); fclose(tmp);
    remove(CSV_FILE); rename("temp.csv", CSV_FILE);

    if (found) printf("Deleted guest ID: %d\n", target);
    else       printf("ID not found.\n");
}



static void action_update_guest() {
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
    if (strlen(buf) >= 2) strncpy(g.name, buf, sizeof(g.name)-1);
    else if (strlen(buf) > 0) printf("Name too short — kept original.\n");

    read_line("New age: ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (is_valid_age(buf)) g.age = atoi(buf);
        else printf("Invalid age — kept original.\n");
    }

    read_line("New status: ", buf, sizeof(buf));
    if (strlen(buf) > 0) strncpy(g.status, buf, sizeof(g.status)-1);

    read_line("New phone: ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (is_valid_phone(buf)) strncpy(g.phone, buf, sizeof(g.phone)-1);
        else printf("Invalid phone — kept original.\n");
    }

    printf("Change side? 0=No  1=Yes\n");
    if (read_int("Choice: ") == 1) g.side = prompt_side();

    printf("Change parking? 0=No  1=Yes\n");
    if (read_int("Choice: ") == 1) prompt_parking(g.parking, sizeof(g.parking));

   
    FILE *f   = fopen(CSV_FILE,  "r"); if (!f)   return;
    FILE *tmp = fopen("temp.csv","w"); if (!tmp) { fclose(f); return; }

    char line[512];
    Guest cur;
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &cur.id, cur.name, &cur.age, cur.status, cur.phone,
                   (int*)&cur.side, cur.parking) != 7)
            continue;
        if (cur.id == target) {
            fprintf(tmp, "%d,%s,%d,%s,%s,%d,%s\n",
                    g.id, g.name, g.age, g.status, g.phone, (int)g.side, g.parking);
            found = 1;
        } else {
            fprintf(tmp, "%d,%s,%d,%s,%s,%d,%s\n",
                    cur.id, cur.name, cur.age, cur.status, cur.phone,
                    (int)cur.side, cur.parking);
        }
    }
    fclose(f); fclose(tmp);
    remove(CSV_FILE); rename("temp.csv", CSV_FILE);

    if (found) printf("Updated guest ID: %d\n", target);
    else       printf("ID not found.\n");
}

int main(void) {
    int choice;
    do {
        printf("Wedding – Guest Manager \n");
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
