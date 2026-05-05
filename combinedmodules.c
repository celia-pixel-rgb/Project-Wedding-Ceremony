

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
     Displays detailed information about all available modules.
     This helps users understand what each module does before selecting it.
   
   PARAMETERS:
     void - Takes no parameters
   
   RETURN VALUE:
     void - Returns nothing (only prints information)
   
   MODULES DESCRIBED:
     1. Person Management     - Handles individual guest records
     2. Category Management   - Organises guests into groups
     3. Gift Management       - Manages wedding gifts chosen by guests
   
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

    /* ---- Module 3: Gift Management Description ---- */
    printf("  MODULE 03 - Gift Management\n");
    printf("    Allow guests to choose and register a gift\n");
    printf("    from a predefined list, with payment tracking.\n");

    // Feature list for Module 3
    printf("    Features:\n");
    printf("      ?  Choose gift from 26 options\n");     // Guest picks from the predefined gift list
    printf("      ?  Cash contribution option\n");        // Option for guests who prefer to give cash
    printf("      ?  Multiple payment methods\n");        // Cash, Mobile Money, Credit Card, etc.
    printf("      ?  Password-protected display\n");      // Admin must authenticate to view records
    printf("      ?  Update & delete gift records\n");    // Modify or remove existing gift entries
    printf("\n");
    
    /* ---- System Information Footer ---- */
    printf("  PASSWORD PROTECTED  -  group3wed!\n");  // Security notice
    printf("  Wedding Guest System  v1.0\n");         // Version information
    printf("  ------------------------------------------------\n");
}


/* ============================================================================
   SECTION 2: MODULE LAUNCHERS
   
   These functions launch each sub-module.
   Modules 01 and 02 are external programs launched via system().
   Module 03 (Gift Management) is integrated directly into this file.
============================================================================ */

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


/* ============================================================================
   SECTION 3: GIFT MANAGEMENT MODULE
   
   Integrated directly from gift.c.
   Manages wedding gifts chosen by guests from a predefined list.
   Guests can add, display, update, and delete gift records.
   All sensitive operations are password-protected.
============================================================================ */

/* ----------------------------------------------------------------------------
   CONSTANTS
---------------------------------------------------------------------------- */
#define MAX_GIFTS 200
#define GIFT_PASSWORD "group3wed!"

/* ----------------------------------------------------------------------------
   STRUCT: Gift
   
   Stores all information about a single gift contribution by a guest.
   
   FIELDS:
     id         - Unique identifier assigned automatically (e.g., 1G, 2G)
     name       - Full name of the guest making the contribution
     gift       - Name of the chosen gift item
     unitPrice  - Price per unit of the chosen gift in FCFA
     quantity   - Number of units the guest is contributing
     totalPrice - Total amount to be paid (unitPrice * quantity)
---------------------------------------------------------------------------- */
typedef struct {
    int   id;
    char  name[100];
    char  gift[100];
    float unitPrice;
    int   quantity;
    float totalPrice;
} Gift;

/* ----------------------------------------------------------------------------
   GLOBAL DATA FOR GIFT MODULE
   
   gifts[]  - Array storing all registered gift contributions
   count    - Current number of registered gifts
   nextId   - Auto-incrementing ID counter for new gift entries
---------------------------------------------------------------------------- */
static Gift gifts[MAX_GIFTS];
static int  giftCount = 0;
static int  nextId    = 1;

/* ----------------------------------------------------------------------------
   GIFT LIST
   
   26 predefined gift options that guests can choose from.
   The last option (index 25) is "Cash Contribution" with price 0,
   because the guest enters a custom amount.
---------------------------------------------------------------------------- */
static char *giftNames[] = {
    "House", "Car", "Pot Set", "Spoon Set", "Fork Set",
    "Dish Set", "Glass Set", "Microwave", "Photo Frame",
    "Travel Ticket", "Hotel Reservation", "Wine", "Iron",
    "Bedside Table", "Cushion Set", "Curtain Set", "Television",
    "Flower Pot", "Kettle", "Cup Set", "Sheet Set", "Jewelry",
    "Couple Watch", "Home Decor", "Gas Stove", "Cash Contribution"
};

static float giftPrices[] = {
    15000000, 8000000, 50000, 10000, 10000,
    30000,    20000,   70000, 15000,
    200000,   150000,  10000,
    25000,    40000,   30000, 35000,
    250000,   10000,   15000, 12000,
    30000,    100000,  80000, 50000,
    60000,    0
};

/* ----------------------------------------------------------------------------
   FUNCTION: gift_show_menu()
   
   PURPOSE:
     Displays the full list of 26 gift options with their prices.
     Called before a guest makes or updates their gift choice.
   
   PARAMETERS: void
   RETURN VALUE: void
---------------------------------------------------------------------------- */
static void gift_show_menu(void)
{
    int i;
    printf("\n--- Gift List ---\n");
    for (i = 0; i < 26; i++) {
        printf("%d. %s - %.0f FCFA\n", i + 1, giftNames[i], giftPrices[i]);
    }
}

/* ----------------------------------------------------------------------------
   FUNCTION: gift_authenticate()
   
   PURPOSE:
     Prompts the user for a password and validates it.
     Used to protect sensitive operations (display, update, delete).
   
   PARAMETERS: void
   
   RETURN VALUE:
     int - 1 if password is correct, 0 if incorrect
---------------------------------------------------------------------------- */
static int gift_authenticate(void)
{
    char pass[50];
    printf("Enter password: ");
    scanf("%49s", pass);
    return strcmp(pass, GIFT_PASSWORD) == 0;
}

/* ----------------------------------------------------------------------------
   FUNCTION: gift_add()
   
   PURPOSE:
     Registers a new gift contribution from a guest.
     Prompts for name, gift choice, quantity, and payment method.
     Assigns a unique ID and saves the record to the gifts[] array.
   
   PARAMETERS: void
   RETURN VALUE: void
---------------------------------------------------------------------------- */
static void gift_add(void)
{
    if (giftCount >= MAX_GIFTS) {
        printf("Gift list is full. Cannot add more entries.\n");
        return;
    }

    Gift g;
    int choice;

    printf("\nEnter your name: ");
    scanf(" %99[^\n]", g.name);

    gift_show_menu();
    printf("Choose gift (1-26): ");
    scanf("%d", &choice);

    // Validate choice range
    if (choice < 1 || choice > 26) {
        printf("Invalid choice.\n");
        return;
    }

    // Copy chosen gift name and its unit price
    strcpy(g.gift, giftNames[choice - 1]);
    g.unitPrice = giftPrices[choice - 1];

    if (choice == 26) {
        // Cash Contribution: guest enters a custom amount
        printf("Enter amount to contribute: ");
        scanf("%f", &g.totalPrice);
        g.quantity = 1;
    } else {
        printf("Enter quantity: ");
        scanf("%d", &g.quantity);
        g.totalPrice = g.quantity * g.unitPrice;  // Calculate total
    }

    // Assign unique ID and save to array
    g.id = nextId++;
    gifts[giftCount++] = g;

    printf("Total to pay: %.0f FCFA\n", g.totalPrice);

    // Display payment method options
    printf("\nPayment Methods:\n");
    printf("1. Cash\n");
    printf("2. Mobile Money\n");
    printf("3. Orange Money\n");
    printf("4. Airtel Money\n");
    printf("5. Yoomee\n");
    printf("6. Credit Card\n");
    printf("Choose payment method: ");

    int pay;
    scanf("%d", &pay);

    if (pay == 6) {
        char card[20];
        printf("Enter credit card number: ");
        scanf("%19s", card);
    } else if (pay >= 2 && pay <= 5) {
        // Provide mobile payment numbers
        printf("Send to: 651426895, 640526848, +2378556421\n");
    }

    printf("\nThank you %s for your contribution! Your ID is %dG\n", g.name, g.id);
}

/* ----------------------------------------------------------------------------
   FUNCTION: gift_display_all()
   
   PURPOSE:
     Displays all registered gift contributions.
     Password-protected to ensure only authorised users can view records.
   
   PARAMETERS: void
   RETURN VALUE: void
---------------------------------------------------------------------------- */
static void gift_display_all(void)
{
    if (!gift_authenticate()) {
        printf("Incorrect password. Access denied.\n");
        return;
    }

    if (giftCount == 0) {
        printf("No gift records found.\n");
        return;
    }

    int i;
    printf("\n--- All Gift Records ---\n");
    for (i = 0; i < giftCount; i++) {
        printf("%dG - %s gave %s (%.0f FCFA)\n",
               gifts[i].id,
               gifts[i].name,
               gifts[i].gift,
               gifts[i].totalPrice);
    }
}

/* ----------------------------------------------------------------------------
   FUNCTION: gift_display_one()
   
   PURPOSE:
     Displays the gift record for one specific guest by their ID.
     Password-protected.
   
   PARAMETERS:
     int id - The numeric part of the guest's gift ID (e.g., 2 for "2G")
   
   RETURN VALUE: void
---------------------------------------------------------------------------- */
static void gift_display_one(int id)
{
    if (!gift_authenticate()) {
        printf("Incorrect password. Access denied.\n");
        return;
    }

    int i;
    for (i = 0; i < giftCount; i++) {
        if (gifts[i].id == id) {
            printf("%dG - %s gave %s (%.0f FCFA)\n",
                   gifts[i].id,
                   gifts[i].name,
                   gifts[i].gift,
                   gifts[i].totalPrice);
            return;
        }
    }
    printf("No record found for ID %dG.\n", id);
}

/* ----------------------------------------------------------------------------
   FUNCTION: gift_update()
   
   PURPOSE:
     Allows an authorised user to update a guest's gift choice and quantity.
     Password-protected.
   
   PARAMETERS:
     int id - The numeric part of the guest's gift ID to update
   
   RETURN VALUE: void
---------------------------------------------------------------------------- */
static void gift_update(int id)
{
    if (!gift_authenticate()) {
        printf("Incorrect password. Access denied.\n");
        return;
    }

    int i;
    for (i = 0; i < giftCount; i++) {
        if (gifts[i].id == id) {
            printf("Updating gift for %s\n", gifts[i].name);
            gift_show_menu();

            int choice;
            printf("Choose new gift (1-26): ");
            scanf("%d", &choice);

            if (choice < 1 || choice > 26) {
                printf("Invalid choice.\n");
                return;
            }

            strcpy(gifts[i].gift, giftNames[choice - 1]);
            gifts[i].unitPrice = giftPrices[choice - 1];

            printf("Enter new quantity: ");
            scanf("%d", &gifts[i].quantity);

            gifts[i].totalPrice = gifts[i].quantity * gifts[i].unitPrice;

            printf("Updated successfully.\n");
            return;
        }
    }
    printf("No record found for ID %dG.\n", id);
}

/* ----------------------------------------------------------------------------
   FUNCTION: gift_delete()
   
   PURPOSE:
     Removes a guest's gift record from the array by their ID.
     Password-protected.
     Uses array shifting to close the gap left by the deleted entry.
   
   PARAMETERS:
     int id - The numeric part of the guest's gift ID to delete
   
   RETURN VALUE: void
---------------------------------------------------------------------------- */
static void gift_delete(int id)
{
    if (!gift_authenticate()) {
        printf("Incorrect password. Access denied.\n");
        return;
    }

    int i, j;
    for (i = 0; i < giftCount; i++) {
        if (gifts[i].id == id) {
            // Shift all entries after this one left by one position
            for (j = i; j < giftCount - 1; j++) {
                gifts[j] = gifts[j + 1];
            }
            giftCount--;
            printf("Deleted successfully.\n");
            return;
        }
    }
    printf("No record found for ID %dG.\n", id);
}

/* ----------------------------------------------------------------------------
   FUNCTION: launch_gift_management()
   
   PURPOSE:
     Runs the Gift Management sub-menu in a loop.
     This is the entry point called from the main launcher menu.
     The user can add, display, update, and delete gift records,
     then return to the main menu by choosing option 0.
   
   PARAMETERS: void
   RETURN VALUE: void
---------------------------------------------------------------------------- */
static void launch_gift_management(void)
{
    int option;
    int id;

    do {
        printf("\n  ================================================\n");
        printf("    MODULE 03 - Gift Management\n");
        printf("  ================================================\n");
        printf("  1. Add a gift\n");
        printf("  2. Display all gifts\n");
        printf("  3. Display one gift\n");
        printf("  4. Update a gift\n");
        printf("  5. Delete a gift\n");
        printf("  0. Return to Main Menu\n");
        printf("\n  Choice: ");
        fflush(stdout);

        if (scanf("%d", &option) != 1) break;

        switch (option) {
            case 1:
                gift_add();
                break;
            case 2:
                gift_display_all();
                break;
            case 3:
                printf("Enter guest ID number: ");
                scanf("%d", &id);
                gift_display_one(id);
                break;
            case 4:
                printf("Enter guest ID number: ");
                scanf("%d", &id);
                gift_update(id);
                break;
            case 5:
                printf("Enter guest ID number: ");
                scanf("%d", &id);
                gift_delete(id);
                break;
            case 0:
                printf("\n  Returning to Main Menu...\n");
                break;
            default:
                printf("\n  Invalid option. Please enter 0-5.\n");
                break;
        }

    } while (option != 0);
}


/* ============================================================================
   SECTION 4: MAIN ENTRY POINT
   
   The main() function drives the top-level launcher menu.
   It displays the banner, module descriptions, and handles
   user input to route to the correct module.
============================================================================ */

int main(void)
{
    /* ---- Variable Declarations ---- */
    
    // Buffer to store user input as a string (32 characters max)
    // We use a character array because fgets() reads text input
    char buf[32];
    
    // Integer variable to store the user's menu choice (1, 2, 3, or 0)
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
        printf("  3. Open Module 03 - Gift Management\n");
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
                break;
            
            case 2:
                // User chose option 2: launch Category Management
                launch_category_management();
                break;
            
            case 3:
                // User chose option 3: launch Gift Management (integrated)
                launch_gift_management();
                break;
            
            case 0:
                // User chose option 0: exit the program
                printf("\n  Goodbye!\n\n");
                break;
            
            default:
                // Any other input is invalid
                printf("\n  Invalid choice. Please enter 0, 1, 2 or 3.\n");
                break;
        }
        
    } while (choice != 0);  // Continue loop unless user chose to exit
    
    /* ---- Program Exit ---- */
    
    // Return 0 to operating system
    // By convention, 0 means "program executed successfully"
    return 0;
}
