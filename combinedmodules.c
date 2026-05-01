

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
   SECTION 1: DISPLAY FUNCTIONS
   
   These functions are responsible for showing information to the user.
   They use printf() to output formatted text to the console.
============================================================================ */

/* ----------------------------------------------------------------------------
   FUNCTION: print_banner()
   
   PURPOSE:
     Displays a decorative welcome banner at the top of the menu.
     This provides visual separation and makes the interface more professional.
   
   PARAMETERS:
     void - This function takes no parameters
   
   RETURN VALUE:
     void - This function doesn't return a value (it only prints to screen)
   
   HOW IT WORKS:
     Uses printf() to output ASCII art borders and text.
     \n creates new lines for spacing.
---------------------------------------------------------------------------- */
static void print_banner(void)
{
    printf("\n");  // Print one blank line for spacing
    printf("  ================================================\n");
    printf("    WEDDING GUEST SYSTEM  -  GROUP 3\n");
    printf("    Management Portal\n");
    printf("  ================================================\n");
    printf("\n");  // Print another blank line
}

/* ----------------------------------------------------------------------------
   FUNCTION: print_module_info()
   
   PURPOSE:
     Displays detailed information about the two available modules.
     This helps users understand what each module does before selecting it.
   
   PARAMETERS:
     void - Takes no parameters
   
   RETURN VALUE:
     void - Returns nothing (only prints information)
   
   MODULES DESCRIBED:
     1. Person Management - Handles individual guest records
     2. Category Management - Organizes guests into groups
   
   NOTES:
     The "?" symbols in the output are bullet points for the feature lists.
     Each feature is briefly described to inform the user of capabilities.
---------------------------------------------------------------------------- */
static void print_module_info(void)
{
    /* ---- Module 1: Person Management Description ---- */
    printf("  MODULE 01 - Person Management\n");
    printf("    Register, update and manage every wedding guest\n");
    printf("    with full validation and CSV persistence.\n");
    
    // Feature list for Module 1
    printf("    Features:\n");
    printf("      ?  Add & save guests (CSV)\n");        // Save guest data to CSV file
    printf("      ?  Live field validation\n");          // Check input as user types
    printf("      ?  Update guest information\n");       // Edit existing guest records
    printf("      ?  Password-protected delete\n");      // Require password to delete guests
    printf("      ?  Auto-refresh display\n");           // Screen updates automatically
    printf("\n");
    
    /* ---- Module 2: Category Management Description ---- */
    printf("  MODULE 02 - Category Management\n");
    printf("    Organise guests into nested linked-list\n");
    printf("    categories such as VIP, Family, Friends.\n");
    
    // Feature list for Module 2
    printf("    Features:\n");
    printf("      ?  Create named categories\n");         // Define new group names (VIP, Family, etc.)
    printf("      ?  Assign guests (nested list)\n");     // Link guests to specific categories
    printf("      ?  Sort by guest count\n");             // Order categories by number of guests
    printf("      ?  Update & delete categories\n");      // Modify or remove category groups
    printf("      ?  Remove guests from category\n");     // Unlink a guest from their category
    printf("\n");
    
    /* ---- System Information Footer ---- */
    printf("  PASSWORD PROTECTED  -  group3wed!\n");  // Security notice
    printf("  Wedding Guest System  v1.0\n");         // Version information
    printf("  ------------------------------------------------\n");
}

static void launch_person_management(void)
{
    // Inform user that module is being launched
    printf("\n[Launcher] Opening Person Management (./person)...\n");
    
    // Execute the person management program
    // system() returns 0 on success, non-zero on failure
    int ret = system("./person");
    
    // Check if the launch was successful
    if (ret != 0)
    {
        // Launch failed - display helpful error message
        printf("[Launcher] Could not launch './person'.\n");
        printf("  Make sure it is compiled:\n");
        
        // Provide exact compilation command for user convenience
        printf("    gcc person-1.c -o person\n");
    }
    // If ret == 0, launch was successful - no error message needed
    // Control returns to main() and the menu will be displayed again
}

static void launch_category_management(void)
{
    // Inform user that module is being launched
    printf("\n[Launcher] Opening Category Management (./category)...\n");
    
    // Execute the category management program
    int ret = system("./category");
    
    // Check if launch was successful
    if (ret != 0)
    {
        // Launch failed - display helpful error message
        printf("[Launcher] Could not launch './category'.\n");
        printf("  Make sure it is compiled:\n");
        
        // Provide exact compilation command
        printf("    gcc category.c -o category\n");
    }
    // If successful, control returns to main() to show menu again
}

 
int main(void)
{
    /* ---- Variable Declarations ---- */
    
    // Buffer to store user input as a string (32 characters max)
    // We use a character array because fgets() reads text input
    char buf[32];
    
    // Integer variable to store the user's menu choice (1, 2, or 0)
    // This is converted from the string input using atoi()
    int choice;
  
    do {
        /* -- Display interface -- */
        print_banner();        // Show decorative header
        print_module_info();   // Show module descriptions
        printf("\n");
        
        /* -- Display menu options -- */
        printf("  1. Open Module 01 - Person Management\n");
        printf("  2. Open Module 02 - Category Management\n");
        printf("  0. Exit\n");
        printf("\n");
        printf("  Choice: ");
        
        // fflush() forces the prompt to display immediately
        // (without this, output might be buffered and not show right away)
        fflush(stdout);
        
        /* -- Read user input -- */
        
        // fgets() reads a line of text from stdin (keyboard)
        // Parameters: buffer to store input, buffer size, input source
        // Returns NULL if read fails (e.g., end of file)
        if (!fgets(buf, sizeof(buf), stdin)) 
            break;  // Exit loop if input fails
        
        // atoi() converts string to integer
        // Example: "2" becomes integer 2, "abc" becomes 0
        choice = atoi(buf);
        
        /* -- Handle user's choice -- */
        
        // switch statement: efficient way to handle multiple cases
        // Compares 'choice' against each case value
        switch (choice)
        {
            case 1:
                // User chose option 1: launch Person Management
                launch_person_management();
                break;  // Exit switch after handling this case
            
            case 2:
                // User chose option 2: launch Category Management
                launch_category_management();
                break;
            
            case 0:
                // User chose option 0: exit the program
                printf("\n  Goodbye!\n\n");
                break;
            
            default:
                // Any other input is invalid
                printf("\n  Invalid choice. Please enter 0, 1 or 2.\n");
                break;
        }
        
    } while (choice != 0);  // Continue loop unless user chose to exit
    
    /* ---- Program Exit ---- */
    
    // Return 0 to operating system
    // By convention, 0 means "program executed successfully"
    return 0;
}


