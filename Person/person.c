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
#include "person.h"

// this is used to represent which side the guest belongs to
// from the person.h, we will have structure or parameters representing the guest
// these are main storage file and temporary filed. the temporary file will be used for updates/deletes
// ALL platforms (person.c, gtkperson.c) share this same file so IDs stay consistent across all of them
const char *CSV_FILE = "persons.csv";
// this is to insure the security and the confidentiality of the file  
const char *PASSWORD = "group3wed!";

//it converts enum to readable string 
const char* side_to_string(Side s) {
    return (s == GROOM) ? "Groom" : (s == BRIDE) ? "Bride" : "Both";
}


// reads a line of text from the user after printing a prompt
static void read_line(const char *prompt, char *buf, int size) {
    printf("%s", prompt);
    fflush(stdout);                      //to ensure that the prompt appears immediately.
    if (fgets(buf, size, stdin))
        buf[strcspn(buf, "\n")] = '\0';  // to remove the newline character
}

// reads a single integer from the user input
static int read_int(const char *prompt) {
    char buf[32];
    read_line(prompt, buf, sizeof(buf));
    return atoi(buf);
}

// asks the user for a password and returns 1 if it matches, 0 otherwise
static int check_password(void) {
    char pw[64];
    read_line("Password: ", pw, sizeof(pw));
    return strcmp(pw, PASSWORD) == 0;
}


// it is used to verify if string contains only digits
static int is_all_digits(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; s++) if (*s < '0' || *s > '9') return 0;
    return 1;
}

// validate phone number if it startes with 6-9. By convention, we chose the Camerooon format
static int is_valid_phone(const char *p) {
    if (strlen(p) != 9) return 0;
    if (!is_all_digits(p)) return 0;
    return (p[0]=='6' || p[0]=='7' || p[0]=='8' || p[0]=='9');
}

// validate age
static int is_valid_age(const char *a) {
    if (!is_all_digits(a)) return 0;
    int v = atoi(a);
    return (v >= 1 && v <= 120);
}


// scans the CSV file to find the highest existing ID and returns the next available one
// because all platforms write to the same persons.csv, this always gives a unique ID
// regardless of which platform registered the previous guest
int get_next_id(void) {
    FILE *file = fopen(CSV_FILE, "r");
    if (!file) return 0;   /* File does not exist yet -- first guest gets ID 0 */
    int max_id = -1, id;
    char line[512];
    while (fgets(line, sizeof(line), file))
        if (sscanf(line, "%d,", &id) == 1 && id > max_id) max_id = id;
    fclose(file);
    return max_id + 1;   /* Next ID is one above the current maximum */
}

// asks the user which side of the wedding the guest belongs to and returns the corresponding enum value
static Side prompt_side(void) {
    printf("Side: 1=Groom  2=Bride  3=Both\n");
    int s = read_int("Choice: ");
    if (s == 1) return GROOM;
    if (s == 2) return BRIDE;
    return BOTH;
}

// asks the user whether the guest needs a parking spot and stores "Yes" or "No" in buf
static void prompt_parking(char *buf, int size) {
    printf("Parking? 1=Yes  2=No\n");
    int p = read_int("Choice: ");
    strncpy(buf, (p == 1) ? "Yes" : "No", size - 1);
    buf[size - 1] = '\0';
}

/*
 * renumber_ids -- called after every deletion.
 * Opens persons.csv, reads every remaining guest in order, and rewrites the file
 * giving them fresh sequential IDs starting from 0.
 * Example: if IDs were 0,1,2,3 and ID 1 was deleted,
 * the file becomes 0,1,2 (old IDs 0,2,3 become new IDs 0,1,2).
 */
static void renumber_ids(void) {
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) return;  /* nothing to renumber if the file does not exist */

    FILE *tmp = fopen("temp_renum.csv", "w");
    if (!tmp) { fclose(f); return; }

    char line[512];
    Guest g;
    int new_id = 0;  /* counter that starts at 0 and increases by 1 for each guest */

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &g.id, g.name, &g.age, g.status, g.phone,
                   (int*)&g.side, g.parking) != 7)
            continue;  /* skip any malformed lines */

        /* write the guest back with the new sequential ID instead of the old one */
        fprintf(tmp, "%d,%s,%d,%s,%s,%d,%s\n",
                new_id, g.name, g.age, g.status, g.phone, (int)g.side, g.parking);
        new_id++;  /* move to the next ID for the next guest */
    }

    fclose(f);
    fclose(tmp);

    /* replace the original file with the renumbered one */
    remove(CSV_FILE);
    rename("temp_renum.csv", CSV_FILE);
}


// collects all guest details from the user, validates them, and saves the new guest to the CSV file
void action_add_guest(void) {
    Guest g;

    // temporary buffers to hold raw input before validation and conversion
    char name[100], age_str[10], status[50], phone[20];

    // collect and validate each field one by one
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

    // fill the Guest struct with the validated data
    g.id  = get_next_id();  /* always reads the shared persons.csv so ID is unique across all platforms */
    g.age = atoi(age_str);
    strncpy(g.name,   name,   sizeof(g.name)-1);   g.name[sizeof(g.name)-1]   = '\0';
    strncpy(g.status, status, sizeof(g.status)-1); g.status[sizeof(g.status)-1] = '\0';
    strncpy(g.phone,  phone,  sizeof(g.phone)-1);  g.phone[sizeof(g.phone)-1]  = '\0';
    g.side = prompt_side();
    prompt_parking(g.parking, sizeof(g.parking));

    // open CSV in append mode so existing records are not overwritten
    FILE *f = fopen(CSV_FILE, "a");
    if (!f) { printf("File error.\n"); return; }
    fprintf(f, "%d,%s,%d,%s,%s,%d,%s\n",
            g.id, g.name, g.age, g.status, g.phone, (int)g.side, g.parking);
    fclose(f);

}


// reads all guests from the CSV file and displays them in a formatted table
void action_show_guests(void) {
    if (!check_password()) { printf("Incorrect password!\n"); return; }

    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { printf("No data.\n"); return; }

    // print the table header
    printf("\n| ID | %-16s | Age | %-8s | %-10s | %-5s | Parking |\n",
           "Name", "Status", "Phone", "Side");
    printf("|-----|------------------|-----|----------|------------|-------|--------|\n");

    char line[512];
    Guest g;
    // read and print each guest row from the CSV
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &g.id, g.name, &g.age, g.status, g.phone,
                   (int*)&g.side, g.parking) != 7)
            continue;  // skip malformed lines
        printf("| %-3d | %-16s | %-3d | %-8s | %-10s | %-5s | %-6s |\n",
               g.id, g.name, g.age, g.status, g.phone,
               side_to_string(g.side), g.parking);
    }
    fclose(f);
    printf("\n");
}




// searches the CSV file for a guest with the given ID and copies their data into 'out'
// returns 1 if found, 0 if not found
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



//DELETE GUESTS
void action_delete_guest(void) {
    if (!check_password()) { printf("Incorrect password!\n"); return; }

    int target = read_int("Guest ID to delete: ");
    if (target < 0) { printf("Invalid ID.\n"); return; }

    // open the original file for reading and a temp file for writing
    FILE *f   = fopen(CSV_FILE,  "r"); if (!f)   return;
    FILE *tmp = fopen("temp.csv","w"); if (!tmp) { fclose(f); return; }

    char line[512];
    Guest g;
    int found = 0;
    // copy every guest except the one to delete into the temp file
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9s",
                   &g.id, g.name, &g.age, g.status, g.phone,
                   (int*)&g.side, g.parking) != 7)
            continue;
        if (g.id != target)
            fprintf(tmp, "%d,%s,%d,%s,%s,%d,%s\n",
                    g.id, g.name, g.age, g.status, g.phone, (int)g.side, g.parking);
        else
            found = 1;  // mark that the target was found and skipped
    }
    fclose(f); fclose(tmp);
    // replace the original file with the temp file
    remove(CSV_FILE); rename("temp.csv", CSV_FILE);

    if (!found) {
        printf("ID not found.\n");
        return;
    }

    printf("Deleted guest ID: %d\n", target);

    // re-number all remaining guests so IDs are sequential again starting from 0
    // e.g. if IDs were 0,1,2,3 and we deleted ID 1, they become 0,1,2
    renumber_ids();
    printf("IDs renumbered.\n");
}



// UPDATE GUESTS
// loads the guest's current data, lets the user change any field, then rewrites the CSV
void action_update_guest(void) {
    if (!check_password()) { printf("Incorrect password!\n"); return; }

    int target = read_int("Guest ID to update: ");
    Guest g;
    // load the existing record so unchanged fields keep their original values
    if (!load_guest(target, &g)) { printf("ID not found.\n"); return; }

    // show the current values so the user knows what they are changing
    printf("Current: %s | Age %d | %s | %s | %s | Parking: %s\n",
           g.name, g.age, g.status, g.phone,
           side_to_string(g.side), g.parking);
    printf("(Leave blank to keep current value)\n");

    char buf[100];

    // update each field only if the user provides a non-empty value
    read_line("New name: ", buf, sizeof(buf));
    if (strlen(buf) >= 2) strncpy(g.name, buf, sizeof(g.name)-1);
    else if (strlen(buf) > 0) printf("Name too short \x97 kept original.\n");

    read_line("New age: ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (is_valid_age(buf)) g.age = atoi(buf);
        else printf("Invalid age \x97 kept original.\n");
    }

    read_line("New status: ", buf, sizeof(buf));
    if (strlen(buf) > 0) strncpy(g.status, buf, sizeof(g.status)-1);

    read_line("New phone: ", buf, sizeof(buf));
    if (strlen(buf) > 0) {
        if (is_valid_phone(buf)) strncpy(g.phone, buf, sizeof(g.phone)-1);
        else printf("Invalid phone \x97 kept original.\n");
    }

    printf("Change side? 0=No  1=Yes\n");
    if (read_int("Choice: ") == 1) g.side = prompt_side();

    printf("Change parking? 0=No  1=Yes\n");
    if (read_int("Choice: ") == 1) prompt_parking(g.parking, sizeof(g.parking));

    // rewrite the CSV: copy all rows, replacing the updated guest's row
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
            // write the updated record in place of the old one
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
    // replace the original file with the updated temp file
    remove(CSV_FILE); rename("temp.csv", CSV_FILE);

    if (found) printf("Updated guest ID: %d\n", target);
    else       printf("ID not found.\n");
}

// main entry point -- shows the menu in a loop until the user chooses to exit
int main(void) {
    int choice;
    do {
        // display the menu options
        printf("Wedding \x96 Guest Manager \n");
        printf("1. Add guest\n");
        printf("2. Display all guests\n");
        printf("3. Delete guest\n");
        printf("4. Update guest\n");
        printf("0. Exit\n");
        choice = read_int("Choice: ");

        // call the matching function based on the user's choice
        switch (choice) {
            case 1: action_add_guest();    break;
            case 2: action_show_guests();  break;
            case 3: action_delete_guest(); break;
            case 4: action_update_guest(); break;
            case 0: printf("Goodbye!\n");  break;
            default: printf("Invalid choice.\n"); break;
        }
    } while (choice != 0);  // keep looping until the user enters 0

    return 0;
}
