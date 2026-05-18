/* this program is used to dress up the guest space for the form, a space to display, update and delete a guest */
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "person.h"

// this csv file will be used for storage
const char *CSV_FILE = "persons.csv";

// shared bookings file – same path used by parking-fixed.c
static const char *BOOKINGS_FILE = "parking_booking.csv";

// this password is to insure the security and the confidentiality of the file and the program
const char *PASSWORD = "group3wed!";

// these are the parameter for the form. it is about the guest information
GtkWidget *name_entry, *age_entry, *status_entry, *phone_entry, *email_entry;
GtkWidget *radio_groom, *radio_bride, *radio_both;
GtkWidget *parking_dropdown;

// to display a guest, update a guest and delete a guest, we need to enter the password
GtkWidget *manager_text;  // display a guest
GtkWidget *display_password_entry;

GtkWidget *update_id_entry, *update_name, *update_age, *update_status, *update_phone, *update_email;  //update a guest information
GtkWidget *update_radio_groom, *update_radio_bride, *update_radio_both;
GtkWidget *update_parking;
GtkWidget *update_password_entry;
GtkWidget *update_fields_box;

GtkWidget *delete_id_entry; //delete a guest
GtkWidget *delete_password_entry;

// now we get the next available ID from the CSV file
int get_next_id() {
    FILE *file = fopen(CSV_FILE, "r");
    if (!file) return 0;

    int max_id = -1, id;
    char line[512];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%d,", &id) == 1 && id > max_id)
            max_id = id;
    }

    fclose(file);
    return max_id + 1;
}

// determine the selected side like: groom, bride, both
Side get_side(GtkWidget *groom, GtkWidget *bride) {
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(groom))) return GROOM;
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(bride))) return BRIDE;
    return BOTH;
}
// now we return this enum to string 

// save new guest into the CSV file
void save_data(GtkButton *btn, gpointer data) {
    Guest g; 
    const char *name = gtk_editable_get_text(GTK_EDITABLE(name_entry)); // get input values
    const char *age_str = gtk_editable_get_text(GTK_EDITABLE(age_entry));

    if (!strlen(name) || !strlen(age_str)) {
        g_print("Fill required fields\n");
        return;
    }

    int age = atoi(age_str);
    if (age <= 0) {
        g_print("Invalid age\n"); // this is used to insure that the age entered by the user is not negative
        return;
    }

    g.id = get_next_id();
    g.age = age;
    strncpy(g.name, name, sizeof(g.name)-1); g.name[sizeof(g.name)-1]='\0';
    strncpy(g.status, gtk_editable_get_text(GTK_EDITABLE(status_entry)), sizeof(g.status)-1); g.status[sizeof(g.status)-1]='\0';
    strncpy(g.phone, gtk_editable_get_text(GTK_EDITABLE(phone_entry)), sizeof(g.phone)-1); g.phone[sizeof(g.phone)-1]='\0';

    /* Email – optional but stored */
    char email[150] = "";
    strncpy(email, gtk_editable_get_text(GTK_EDITABLE(email_entry)), sizeof(email)-1);
    email[sizeof(email)-1] = '\0';

    g.side = get_side(radio_groom, radio_bride);
    
// this one is now for the parking selection
    int p = gtk_drop_down_get_selected(GTK_DROP_DOWN(parking_dropdown));
    strncpy(g.parking, (p==0)?"Yes":"No", sizeof(g.parking)-1); g.parking[sizeof(g.parking)-1]='\0';

    FILE *f = fopen(CSV_FILE, "a");
    if (!f) { g_print("File error\n"); return; }

    /* CSV format: id,name,age,status,phone,side,parking,email */
    fprintf(f,"%d,%s,%d,%s,%s,%d,%s,%s\n",
            g.id,g.name,g.age,g.status,g.phone,g.side,g.parking,email);

    fclose(f);

    gtk_editable_set_text(GTK_EDITABLE(name_entry),"");
    gtk_editable_set_text(GTK_EDITABLE(age_entry),"");
    gtk_editable_set_text(GTK_EDITABLE(status_entry),"");
    gtk_editable_set_text(GTK_EDITABLE(phone_entry),"");
    gtk_editable_set_text(GTK_EDITABLE(email_entry),"");

    g_print("Saved ID: %d\n",g.id);
}

// -----DISPLAY----
// show all guestt in the table format
void show_data(GtkButton *btn, gpointer data) {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(display_password_entry));
    if (strcmp(pw,PASSWORD)!=0) {                        //here, to verify if it is the user, we check the password
        g_print("Incorrect password!\n");
        return;
    }

    FILE *f = fopen(CSV_FILE,"r");
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(manager_text));

    if (!f) {
        gtk_text_buffer_set_text(buf,"No data.\n",-1);
        return;
    }

    char line[512];
    Guest g;
    char email[150];
    //here, we create formatted table base on the ID, the name, the age, the status, the phone, the side, the available parking and email
    GString *content = g_string_new("| ID | Name             | Age | Status   | Phone       | Side  | Parking | Email                          |\n");
    g_string_append(content,"|----|-----------------|-----|----------|------------|-------|---------|--------------------------------|\n");

    while (fgets(line,sizeof(line),f)) {
        email[0] = '\0';
        if (sscanf(line,"%d,%99[^,],%d,%49[^,],%19[^,],%d,%9[^,],%149[^\n]",
                   &g.id,g.name,&g.age,g.status,g.phone,&g.side,g.parking,email) < 7)
            continue;
            
// add row to table
        g_string_append_printf(content,"| %-2d | %-15s | %-3d | %-8s | %-10s | %-5s | %-7s | %-30s |\n",
                               g.id,g.name,g.age,g.status,g.phone,
                               side_to_string(g.side),g.parking,email);
    }

    fclose(f);
    gtk_text_buffer_set_text(buf,content->str,-1);
    g_string_free(content,TRUE);
}

// ---------- AUTO REFRESH ----------
gboolean auto_refresh(gpointer data) {
    show_data(NULL,NULL);
    return TRUE;
}

// ---------- LOAD GUEST FOR UPDATE ----------
void load_guest_for_update(GtkButton *btn, gpointer data) {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(update_password_entry));
    if (strcmp(pw, PASSWORD) != 0) {
        g_print("Incorrect password!\n");
        return;
    }
    
// here, all the possible researches are based on the number of ID because it is unique and it is the primary key.
    const char *id_text = gtk_editable_get_text(GTK_EDITABLE(update_id_entry));
    if (!strlen(id_text)) { g_print("Enter ID\n"); return; }
    int target = atoi(id_text);

    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { g_print("File not found\n"); return; }

    Guest g;
    char line[512];
    gboolean found = FALSE;

    while (fgets(line, sizeof(line), f)) {
        char email[150] = "";
        if (sscanf(line, "%d,%99[^,],%d,%49[^,],%19[^,],%d,%9[^,],%149[^\n]",
                   &g.id, g.name, &g.age, g.status, g.phone, &g.side, g.parking, email) < 7)
            continue;

        if (g.id == target) {
            gtk_editable_set_text(GTK_EDITABLE(update_name), g.name);
            char age_buf[10]; sprintf(age_buf, "%d", g.age);
            gtk_editable_set_text(GTK_EDITABLE(update_age), age_buf);
            gtk_editable_set_text(GTK_EDITABLE(update_status), g.status);
            gtk_editable_set_text(GTK_EDITABLE(update_phone), g.phone);
            gtk_editable_set_text(GTK_EDITABLE(update_email), email);
            gtk_drop_down_set_selected(GTK_DROP_DOWN(update_parking), (strcmp(g.parking, "Yes") == 0) ? 0 : 1);

            gtk_check_button_set_active(GTK_CHECK_BUTTON(update_radio_groom), g.side == GROOM);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(update_radio_bride), g.side == BRIDE);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(update_radio_both),  g.side == BOTH);

            found = TRUE;
            break;
        }
    }

    fclose(f);

    if (!found) {
        g_print("ID not found\n");
        return;
    }

    gtk_widget_set_visible(update_fields_box, TRUE);
}

// ---------- POPULATE DELETE FIELDS ----------
void populate_delete_fields(GtkWidget *entry, gpointer data) {
    const char *id_text = gtk_editable_get_text(GTK_EDITABLE(delete_id_entry));
    int target = atoi(id_text);

    FILE *f = fopen(CSV_FILE,"r");
    if (!f) return;

    Guest g;
    char line[512];

    while (fgets(line,sizeof(line),f)) {
        char email[150] = "";
        if (sscanf(line,"%d,%99[^,],%d,%49[^,],%19[^,],%d,%9[^,],%149[^\n]",
                   &g.id,g.name,&g.age,g.status,g.phone,&g.side,g.parking,email) < 7)
            continue;

        if (g.id==target) {
            g_print("ID %d -> Name: %s, Age: %d, Status: %s, Phone: %s, Side: %s, Parking: %s, Email: %s\n",
                    g.id,g.name,g.age,g.status,g.phone,side_to_string(g.side),g.parking,email);
            break;
        }
    }

    fclose(f);
}

// to delete the guest 
void delete_guest(GtkButton *btn, gpointer data) {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(delete_password_entry));
    if (strcmp(pw,PASSWORD)!=0) { g_print("Incorrect password!\n"); return; }   // to insure the security of the file and their confidentiality, we need to enter the passsword

    const char *id_text = gtk_editable_get_text(GTK_EDITABLE(delete_id_entry));
    if (!strlen(id_text)) { g_print("Enter ID\n"); return; }  // after the validated password, we delete the guest or his information based on the Id of that guest

    int target = atoi(id_text);
    gboolean found = FALSE;

    FILE *f = fopen(CSV_FILE,"r");
    if (!f) { g_print("File not found\n"); return; }

    FILE *tmp = fopen("temp.csv","w");
    if (!tmp) return;

    char line[512];
    Guest g;

    while (fgets(line,sizeof(line),f)) {
        char email[150] = "";
        if (sscanf(line,"%d,%99[^,],%d,%49[^,],%19[^,],%d,%9[^,],%149[^\n]",
                   &g.id,g.name,&g.age,g.status,g.phone,&g.side,g.parking,email) < 7)
            continue;

        if (g.id!=target)
            fprintf(tmp,"%d,%s,%d,%s,%s,%d,%s,%s\n",
                    g.id,g.name,g.age,g.status,g.phone,g.side,g.parking,email);
        else found=TRUE;
    }

    fclose(f);
    fclose(tmp);

    remove(CSV_FILE);
    rename("temp.csv",CSV_FILE);

    if (!found) g_print("ID not found\n");
    else g_print("Deleted ID: %d\n",target);
}

// ---------- CANCEL PARKING BOOKINGS ----------
// Called automatically when a guest's parking is changed from "Yes" to "No".
// Removes every booking row that belongs to 'guest_name' from parking_booking.csv,
// freeing those spots for other guests.
static void cancel_parking_bookings_for_guest(const char *guest_name) {
    FILE *f = fopen(BOOKINGS_FILE, "r");
    if (!f) return;   // no bookings file yet – nothing to cancel

    FILE *tmp = fopen("parking_booking_tmp.csv", "w");
    if (!tmp) { fclose(f); return; }

    char line[512];
    int  removed = 0;

    while (fgets(line, sizeof(line), f)) {
        // CSV format: booking_id,spot_id,guest_name,start_h,start_m,dur,end_h,end_m
        int  bid, spot, sh, sm, dur, eh, em;
        char bname[100];
        if (sscanf(line, "%d,%d,%99[^,],%d,%d,%d,%d,%d",
                   &bid, &spot, bname, &sh, &sm, &dur, &eh, &em) != 8) {
            // Unrecognised line – keep it unchanged
            fputs(line, tmp);
            continue;
        }

        // Case-insensitive comparison so capitalisation never causes a mismatch
        if (g_ascii_strcasecmp(bname, guest_name) == 0) {
            removed++;           // skip this row → spot is freed
        } else {
            fputs(line, tmp);    // keep all other guests' bookings
        }
    }

    fclose(f);
    fclose(tmp);

    remove(BOOKINGS_FILE);
    rename("parking_booking_tmp.csv", BOOKINGS_FILE);

    if (removed > 0)
        g_print("Parking cancelled: removed %d booking(s) for '%s'\n",
                removed, guest_name);
}

// ---------- UPDATE ----------
void update_guest(GtkButton *btn, gpointer data) {
    const char *pw = gtk_editable_get_text(GTK_EDITABLE(update_password_entry));
    if (strcmp(pw,PASSWORD)!=0) { g_print("Incorrect password!\n"); return; }            // to insure the security of the file and their confidentiality, we need to enter the passsword

    const char *id_text = gtk_editable_get_text(GTK_EDITABLE(update_id_entry));
    if (!strlen(id_text)) { g_print("Enter ID\n"); return; }          // after the validated password, we delete the guest or his information based on the Id of that guest

    int target = atoi(id_text);
    gboolean found = FALSE;

    FILE *f = fopen(CSV_FILE,"r");
    if (!f) { g_print("File not found\n"); return; }

    FILE *tmp = fopen("temp.csv","w");
    if (!tmp) return;

    char line[512];
    Guest g;

    while (fgets(line,sizeof(line),f)) {
        char email[150] = "";
        if (sscanf(line,"%d,%99[^,],%d,%49[^,],%19[^,],%d,%9[^,],%149[^\n]",
                   &g.id,g.name,&g.age,g.status,g.phone,&g.side,g.parking,email) < 7)
            continue;

        if (g.id==target) {
            const char *name = gtk_editable_get_text(GTK_EDITABLE(update_name));
            const char *status = gtk_editable_get_text(GTK_EDITABLE(update_status));
            const char *phone = gtk_editable_get_text(GTK_EDITABLE(update_phone));
            const char *age_text = gtk_editable_get_text(GTK_EDITABLE(update_age));
            const char *new_email = gtk_editable_get_text(GTK_EDITABLE(update_email));

            if (strlen(name)) strncpy(g.name,name,sizeof(g.name)-1);
            if (strlen(status)) strncpy(g.status,status,sizeof(g.status)-1);
            if (strlen(phone)) strncpy(g.phone,phone,sizeof(g.phone)-1);
            if (strlen(new_email)) strncpy(email,new_email,sizeof(email)-1);

            int age = atoi(age_text);
            if (age>0) g.age=age;

            // Capture the old parking value before overwriting it
            char old_parking[10];
            strncpy(old_parking, g.parking, sizeof(old_parking)-1);
            old_parking[sizeof(old_parking)-1] = '\0';

            int p = gtk_drop_down_get_selected(GTK_DROP_DOWN(update_parking));
            strncpy(g.parking,(p==0)?"Yes":"No",sizeof(g.parking)-1);

            // If parking was "Yes" and is now changed to "No",
            // automatically cancel all bookings for this guest in parking_booking.csv
            if (g_ascii_strcasecmp(old_parking, "Yes") == 0 &&
                g_ascii_strcasecmp(g.parking,   "No")  == 0) {
                cancel_parking_bookings_for_guest(g.name);
            }

            g.side = get_side(update_radio_groom, update_radio_bride);

            found=TRUE;
        }

        fprintf(tmp,"%d,%s,%d,%s,%s,%d,%s,%s\n",
                g.id,g.name,g.age,g.status,g.phone,g.side,g.parking,email);
    }

    fclose(f);
    fclose(tmp);

    remove(CSV_FILE);
    rename("temp.csv",CSV_FILE);

    if (!found) g_print("ID not found\n");
    else g_print("Updated ID: %d\n",target);
}

// ---------- FORM PAGE ----------
// this is about the presentation of the guest interface. 
GtkWidget* create_form_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);

    name_entry = gtk_entry_new();
    age_entry = gtk_entry_new();
    status_entry = gtk_entry_new();
    phone_entry = gtk_entry_new();
    email_entry = gtk_entry_new();

    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry),"Full Name"); 
    gtk_entry_set_placeholder_text(GTK_ENTRY(age_entry),"17");
    gtk_entry_set_placeholder_text(GTK_ENTRY(status_entry),"Single / Married / Divorced");  // this is used to give the different possibilities of status
    gtk_entry_set_placeholder_text(GTK_ENTRY(phone_entry),"650123456");                // this is used to give an example of how the phone number can be written regarding the Cameroon norms
    gtk_entry_set_placeholder_text(GTK_ENTRY(email_entry),"guest@example.com");

    GtkStringList *list = gtk_string_list_new(NULL);
    gtk_string_list_append(list,"Yes");
    gtk_string_list_append(list,"No");

    parking_dropdown = gtk_drop_down_new(G_LIST_MODEL(list),NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(parking_dropdown),1);    // this one is about the parking availability
    g_object_unref(list);
// this is about the side of the guest. here, he/she can be either from the groom, or bride or both
    radio_groom = gtk_check_button_new_with_label("Groom");
    radio_bride = gtk_check_button_new_with_label("Bride");
    radio_both = gtk_check_button_new_with_label("Both");

    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_bride),GTK_CHECK_BUTTON(radio_groom));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_both),GTK_CHECK_BUTTON(radio_groom));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_groom),TRUE);

    GtkWidget *btn = gtk_button_new_with_label("Save");
    g_signal_connect(btn,"clicked",G_CALLBACK(save_data),NULL);

    GtkWidget *row;

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Full Name"));
    gtk_box_append(GTK_BOX(row), name_entry);
    gtk_box_append(GTK_BOX(box), row);

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Age"));
    gtk_box_append(GTK_BOX(row), age_entry);
    gtk_box_append(GTK_BOX(box), row);

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Status"));
    gtk_box_append(GTK_BOX(row), status_entry);
    gtk_box_append(GTK_BOX(box), row);

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Phone"));
    gtk_box_append(GTK_BOX(row), phone_entry);
    gtk_box_append(GTK_BOX(box), row);

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Email"));
    gtk_box_append(GTK_BOX(row), email_entry);
    gtk_box_append(GTK_BOX(box), row);

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Parking"));
    gtk_box_append(GTK_BOX(row), parking_dropdown);
    gtk_box_append(GTK_BOX(box), row);

    row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Side"));
    gtk_box_append(GTK_BOX(row), radio_groom);
    gtk_box_append(GTK_BOX(row), radio_bride);
    gtk_box_append(GTK_BOX(row), radio_both);
    gtk_box_append(GTK_BOX(box), row);

    gtk_box_append(GTK_BOX(box), btn);

    return box;
}

// ---------- DISPLAY PAGE ----------
// this is about the presentation of the displayed page on the file
GtkWidget* create_display_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);

    display_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(display_password_entry),"Enter password");
    gtk_entry_set_visibility(GTK_ENTRY(display_password_entry), FALSE);

    GtkWidget *btn = gtk_button_new_with_label("Refresh");
    g_signal_connect(btn,"clicked",G_CALLBACK(show_data),NULL);

    manager_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(manager_text),FALSE);

    gtk_box_append(GTK_BOX(box), display_password_entry);
    gtk_box_append(GTK_BOX(box),btn);
    gtk_box_append(GTK_BOX(box),manager_text);

    g_timeout_add(2000, auto_refresh, NULL);

    return box;
}

// ---------- DELETE PAGE ----------
// this is about the presentation of the deleted page on the file
GtkWidget* create_delete_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);

    delete_id_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(delete_id_entry),"Enter ID");
    g_signal_connect(delete_id_entry,"changed",G_CALLBACK(populate_delete_fields),NULL);

    delete_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(delete_password_entry),"Enter password");
    gtk_entry_set_visibility(GTK_ENTRY(delete_password_entry), FALSE);

    GtkWidget *btn = gtk_button_new_with_label("Delete");
    g_signal_connect(btn,"clicked",G_CALLBACK(delete_guest),NULL);

    gtk_box_append(GTK_BOX(box),delete_id_entry);
    gtk_box_append(GTK_BOX(box),delete_password_entry);
    gtk_box_append(GTK_BOX(box),btn);

    return box;
}

// ---------- UPDATE PAGE ----------
// this is about the presentation of the updated page on the file. here, they can either update the person entirely or the specific information
// when it comes with the information, it can: the name, the age, the phone number, the status, the side or even the parking
GtkWidget* create_update_page() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    update_id_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(update_id_entry), "ID to update");

    update_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(update_password_entry), "Enter password");
    gtk_entry_set_visibility(GTK_ENTRY(update_password_entry), FALSE);

    GtkWidget *load_btn = gtk_button_new_with_label("Load Guest");
    g_signal_connect(load_btn, "clicked", G_CALLBACK(load_guest_for_update), NULL);

    gtk_box_append(GTK_BOX(box), update_id_entry);
    gtk_box_append(GTK_BOX(box), update_password_entry);
    gtk_box_append(GTK_BOX(box), load_btn);

    update_fields_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    update_name   = gtk_entry_new();
    update_age    = gtk_entry_new();
    update_status = gtk_entry_new();
    update_phone  = gtk_entry_new();
    update_email  = gtk_entry_new();

    gtk_entry_set_placeholder_text(GTK_ENTRY(update_name),   "Name");
    gtk_entry_set_placeholder_text(GTK_ENTRY(update_age),    "Age");
    gtk_entry_set_placeholder_text(GTK_ENTRY(update_status), "Status");
    gtk_entry_set_placeholder_text(GTK_ENTRY(update_phone),  "Phone");
    gtk_entry_set_placeholder_text(GTK_ENTRY(update_email),  "Email");

    GtkStringList *list = gtk_string_list_new(NULL);
    gtk_string_list_append(list, "Yes");
    gtk_string_list_append(list, "No");
    update_parking = gtk_drop_down_new(G_LIST_MODEL(list), NULL);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(update_parking), 1);
    g_object_unref(list);

    update_radio_groom = gtk_check_button_new_with_label("Groom");
    update_radio_bride = gtk_check_button_new_with_label("Bride");
    update_radio_both  = gtk_check_button_new_with_label("Both");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(update_radio_bride), GTK_CHECK_BUTTON(update_radio_groom));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(update_radio_both),  GTK_CHECK_BUTTON(update_radio_groom));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(update_radio_groom), TRUE);

    GtkWidget *save_btn = gtk_button_new_with_label("Update");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(update_guest), NULL);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(row), gtk_label_new("Side"));
    gtk_box_append(GTK_BOX(row), update_radio_groom);
    gtk_box_append(GTK_BOX(row), update_radio_bride);
    gtk_box_append(GTK_BOX(row), update_radio_both);

    gtk_box_append(GTK_BOX(update_fields_box), update_name);
    gtk_box_append(GTK_BOX(update_fields_box), update_age);
    gtk_box_append(GTK_BOX(update_fields_box), update_status);
    gtk_box_append(GTK_BOX(update_fields_box), update_phone);
    gtk_box_append(GTK_BOX(update_fields_box), update_email);
    gtk_box_append(GTK_BOX(update_fields_box), update_parking);
    gtk_box_append(GTK_BOX(update_fields_box), row);
    gtk_box_append(GTK_BOX(update_fields_box), save_btn);

    gtk_box_append(GTK_BOX(box), update_fields_box);
    gtk_widget_set_visible(update_fields_box, FALSE);

    return box;
}

// ---------- MAIN ----------
// after presenting of each page, we now present the entire interface
void activate(GtkApplication *app, gpointer data) {
    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win),"Person Manager");
    gtk_window_set_default_size(GTK_WINDOW(win),600,400);

    GtkWidget *stack = gtk_stack_new();
    gtk_widget_set_hexpand(stack,TRUE);
    gtk_widget_set_vexpand(stack,TRUE);

    GtkWidget *switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher),GTK_STACK(stack));

    gtk_stack_add_titled(GTK_STACK(stack),create_form_page(),"form","Form");
    gtk_stack_add_titled(GTK_STACK(stack),create_display_page(),"display","Display");
    gtk_stack_add_titled(GTK_STACK(stack),create_update_page(),"update","Update");
    gtk_stack_add_titled(GTK_STACK(stack),create_delete_page(),"delete","Delete");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(box),switcher);
    gtk_box_append(GTK_BOX(box),stack);

    gtk_window_set_child(GTK_WINDOW(win),box);
    gtk_window_present(GTK_WINDOW(win));
}

int main(int argc,char **argv) {
    GtkApplication *app = gtk_application_new("org.example.personmanager",G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app,"activate",G_CALLBACK(activate),NULL);
    return g_application_run(G_APPLICATION(app),argc,argv);
}
