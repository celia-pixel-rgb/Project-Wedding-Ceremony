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

static void print_module_info(void)
{
    printf("  MODULE 01 - Person Management\n");
    printf("    Register, update and manage every wedding guest\n");
    printf("    with full validation and CSV persistence.\n");
    printf("    Features:\n");
    printf("      ·  Add & save guests (CSV)\n");
    printf("      ·  Live field validation\n");
    printf("      ·  Update guest information\n");
    printf("      ·  Password-protected delete\n");
    printf("      ·  Auto-refresh display\n");
    printf("\n");
    printf("  MODULE 02 - Category Management\n");
    printf("    Organise guests into nested linked-list\n");
    printf("    categories such as VIP, Family, Friends.\n");
    printf("    Features:\n");
    printf("      ·  Create named categories\n");
    printf("      ·  Assign guests (nested list)\n");
    printf("      ·  Sort by guest count\n");
    printf("      ·  Update & delete categories\n");
    printf("      ·  Remove guests from category\n");
    printf("\n");
    printf("  PASSWORD PROTECTED  -  group3wed!\n");
    printf("  Wedding Guest System  v1.0\n");
    printf("  ------------------------------------------------\n");
}

/* ---------------------------------------------------
   Launch helpers
--------------------------------------------------- */
static void launch_person_management(void)
{
    printf("\n[Launcher] Opening Person Management (./persk)...\n");
    int ret = system("./persk");
    if (ret != 0)
    {
        printf("[Launcher] Could not launch './persk'.\n");
        printf("  Make sure it is compiled:\n");
        printf("    gcc persk-1.c -o persk\n");
    }
}

static void launch_category_management(void)
{
    printf("\n[Launcher] Opening Category Management (./newcat)...\n");
    int ret = system("./newcat");
    if (ret != 0)
    {
        printf("[Launcher] Could not launch './newcat'.\n");
        printf("  Make sure it is compiled:\n");
        printf("    gcc newcat.c -o newcat\n");
    }
}

/* ---------------------------------------------------
   Main menu
--------------------------------------------------- */
int main(void)
{
    char buf[32];
    int choice;

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
