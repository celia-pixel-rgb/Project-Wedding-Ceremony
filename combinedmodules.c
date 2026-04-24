/* this is the main entry point for the wedding. it acts as a central menu where users navigate between two sections: the person management and the category management*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------
   Module descriptions
--------------------------------------------------- */
static void print_banner(void)
{
    printf("\n");
    printf("  ================================================\n");
    printf("    WEDDING GUEST SYSTEM  -  GROUP 3\n");
    printf("    Management Portal\n");
    printf("  ================================================\n");
    printf("\n");
}

// here, we will have 2 modules: the person management and the category management
// the person management handles individual guest records( add, update, delete...)
//the category management organises guests into named category(VIP, Family, friends...)
static void print_module_info(void)
{
	//module 1 description
    printf("  MODULE 01 - Person Management\n");
    printf("    Register, update and manage every wedding guest\n");
    printf("    with full validation and CSV persistence.\n");
    printf("    Features:\n");
    printf("      ·  Add & save guests (CSV)\n"); // save guests to file
    printf("      ·  Live field validation\n");   // input is checked live
    printf("      ·  Update guest information\n"); //edit existing records
    printf("      ·  Password-protected delete\n"); // delection nedds password for security and confidentiality
    printf("      ·  Auto-refresh display\n");  //the screen automaticaly updates it self when an information is modified
    printf("\n");
    
    //module 2 management
    printf("  MODULE 02 - Category Management\n");
    printf("    Organise guests into nested linked-list\n");
    printf("    categories such as VIP, Family, Friends.\n");
    printf("    Features:\n");
    printf("      ·  Create named categories\n");   // define new group names
    printf("      ·  Assign guests (nested list)\n");// link guests to a group
    printf("      ·  Sort by guest count\n");            // order groups by size
    printf("      ·  Update & delete categories\n");     // edit or remove groups
    printf("      ·  Remove guests from category\n");    //unlink a guest from the group 
    printf("\n");
    printf("  PASSWORD PROTECTED  -  group3wed!\n"); //the system is protected by a password to insure security and confidentiality
    printf("  Wedding Guest System  v1.0\n");        // this is the version number
    printf("  ------------------------------------------------\n");
}

/* ---------------------------------------------------
   Launch helpers
--------------------------------------------------- */
// this is used to start a separate compiled and returns the code of that demand

// here we firstly have the lauch person management. here, the process wait until the "persk.c" file finishes then returns here and shows the lain menu again
static void launch_person_management(void)
{
    printf("\n[Launcher] Opening Person Management (./persk)...\n");
    int ret = system("./persk");
    if (ret != 0)                  // it is to chck whether the launch succeeded
    {
        printf("[Launcher] Could not launch './persk'.\n");     // it is to inform the user that "persk.c could not be started
        printf("  Make sure it is compiled:\n");        // help the user by giving the instruction and the exact gcc command needed to compile it  
        printf("    gcc persk-1.c -o persk\n");
    }
}

// here is the secong lauch category management. his behavior is identical to the one of the lauch person management. here, the main process waits until the "newcat.c" file finishes. then it returns to the main menu
static void launch_category_management(void)
{
    printf("\n[Launcher] Opening Category Management (./newcat)...\n");
    int ret = system("./newcat");
    if (ret != 0)   // check whether the launch succeeded
    {
        printf("[Launcher] Could not launch './newcat'.\n");    //it informs the user that the "newcat.c" could not be started
        printf("  Make sure it is compiled:\n");                // provide the exact gcc command needed to compile it so as to easy the work of the user
        printf("    gcc newcat.c -o newcat\n");
    }
}

/* ---------------------------------------------------
   Main menu
--------------------------------------------------- */
// this is the entry point for the entire application.
int main(void)
{
    char buf[32];
    int choice;        // it is an integer version of the user choice
 
    do {
        print_banner();
        print_module_info();
        printf("\n");
        printf("  1. Open Module 01 - Person Management\n");
        printf("  2. Open Module 02 - Category Management\n");
        printf("  0. Exit\n");
        printf("\n");
        printf("  Choice: ");
        fflush(stdout);

        if (!fgets(buf, sizeof(buf), stdin)) break;
        choice = atoi(buf);

        switch (choice)
        {
            case 1: launch_person_management();   break;
            case 2: launch_category_management(); break;
            case 0: printf("\n  Goodbye!\n\n");   break;
            default: printf("\n  Invalid choice. Please enter 0, 1 or 2.\n"); break;
        }

    } while (choice != 0);

    return 0;
}
