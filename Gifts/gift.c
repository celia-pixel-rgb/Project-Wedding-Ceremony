/* this program is about the gift managemnt in a wedding ceremony based on the free choice of the guest among a list of variety possible gift*/
#include <stdio.h>  
#include <string.h>
#include "gift_management.h"

Gift gifts[MAX];
int count = 0;  // this  count will help you in share the id, and to specifically know the number of gift or people that give a gift

int nextId = 1;
int i, j;

// Gift list + prices (FCFA)
char *giftNames[] = {
    "House", "Car", "Pot Set", "Spoon Set", "Fork Set", 
	"Dish Set", "Glass Set", "Microwave", "Photo Frame", 
	"Travel Ticket", "Hotel Reservation", "Wine", "Iron", 
	"Bedside Table", "Cushion Set", "Curtain Set","Television", 
	"Flower Pot", "Kettle", "Cup Set", "Sheet Set", "Jewelry", 
	"Couple Watch", "Home Decor", "Gas Stove", "Cash Contribution"
};    // these are a list of all the possible goods, items or gift that a guest can give during the wedding

float prices[] = {
    15000000, 8000000, 50000, 10000, 10000,
    30000, 20000, 70000, 15000,
    200000, 150000, 10000,
    25000, 40000, 30000, 35000,
    250000, 10000, 15000, 12000,
    30000, 100000, 80000, 50000,
    60000, 0
};               // this is about all the possible price according to the item that the guest is going to choose

void showMenu() {   // thsi menu help the user or the guest to easily do the operation that he or she wants . it can be to add, delet, update, display a gift
    printf("\n--- Gift List ---\n");
    int i;
    for(i=0; i<26; i++) {
        printf("%d. %s - %.0f FCFA\n", i+1, giftNames[i], prices[i]);
    }
}

void addGift() {   // since there is many guest, we need to create a space where each of them can regsiter
    Gift g;
    int choice;

    printf("\nEnter your name: ");  // this name will help so that  the bride and the groom should identify and thanks you 
    scanf(" %[^\n]", g.name);

    showMenu();
    printf("Choose gift: ");  // your are choosing the gift or your favorite item among a list of several items.
    scanf("%d", &choice);

    strcpy(g.gift, giftNames[choice-1]);
    g.unitPrice = prices[choice-1];

    if(choice == 26) {       // this is the latest choice for those who don't have the possibility to buy a goods from the list
        printf("Enter amount to contribute: ");
        scanf("%f", &g.totalPrice);
        g.quantity = 1;
    } else {
        printf("Enter quantity: ");
        scanf("%d", &g.quantity);
        g.totalPrice = g.quantity * g.unitPrice;    // if the quantity that the guest wants to offer is superior to 1, then we need to tell him exactly what he will need to pay so that he should not pay lower or upper the tatal price
    }

    g.id = nextId++;  // the id is crucial because it is unique and it is the primary key. it will help in many later operations
    gifts[count++] = g; 

    printf("Total to pay: %.0f FCFA\n", g.totalPrice);

    printf("\nPayment Methods:\n");
    printf("1. Cash\n2. Mobile Money\n3. Orange Money\n4. Airtel Money\n5. Yoomee\n6. Credit Card\n");

    int pay;
    scanf("%d", &pay);

    if(pay == 6) {
        char card[20];
        printf("Enter credit card number: ");
        scanf("%s", card);
    } else if(pay >= 2 && pay <= 5) {
        printf("Send to: 651426895, 640526848, +2378556421\n");
    }

    printf("\nThank you %s for your contribution and your participation! Your ID is %dG\n", g.name, g.id); // it gives the unique id to the guest
}

int authenticate() {   // here, than to continousely repeat the same command too many times to insure security, we will better call a command called "authenticate"
    char pass[20];
    printf("Enter password: ");
    scanf("%s", pass);

    return strcmp(pass, PASSWORD) == 0;
}

void displayAll() {            // if the guest chooses to display all the guest, then we have list them all 
    if(!authenticate()) return; // to make the program shorter, we call the command "authenticate". it helps to insure the confidentiality annd the security of informations in the program 

    for(i=0; i<count; i++) {
        printf("%dG - %s gave %s (%.0f FCFA)\n",
               gifts[i].id,
               gifts[i].name,
               gifts[i].gift,
               gifts[i].totalPrice);
    }
}

void displayOne(int id) {      // this is where the guest chooses to only display a specific guest which has already register himself
    if(!authenticate()) return;  // this is to verify the user identity and to insure the securiy and the confidentiality of information in the program

    for( i=0; i<count; i++) {
        if(gifts[i].id == id) {
            printf("%dG - %s gave %s (%.0f FCFA)\n",
                   gifts[i].id,
                   gifts[i].name,
                   gifts[i].gift,
                   gifts[i].totalPrice);
        }
    }
}

void deleteGift(int id) {          // this is to delete a gift or a person
    if(!authenticate()) return;    // this is to verify the user identity and to insure the securiy and the confidentiality of information in the program

    for(i=0; i<count; i++) {
        if(gifts[i].id == id) {
            for(j=i; j<count-1; j++) {
                gifts[j] = gifts[j+1];
            }
            count--;
            printf("Deleted successfully.\n");
            return;
        }
    }
}

void updateGift(int id) {  // this is to update the gift choosen by a person. here, may be the guets would have done a mistake, then he wants to correct it. at that stage, he will need to enter his id so as to modify his choices
    if(!authenticate()) return;   // this is to verify the user identity and to insure the securiy and the confidentiality of information in the program

    for( i=0; i<count; i++) {
        if(gifts[i].id == id) {
            printf("Updating gift for %s\n", gifts[i].name);  // after entering the password, and the id, we now ask the guest for his new choice
            showMenu();                  // To clearly see his new choice, we firstly show him the menu so that he should have a large idea of the different possibilities that they can have

            int choice;      // here, we read and regsiter the new choice
            scanf("%d", &choice);

            strcpy(gifts[i].gift, giftNames[choice-1]);
            gifts[i].unitPrice = prices[choice-1];

            printf("Enter quantity: ");  // since he choose the option, he need to choose his quantity
            scanf("%d", &gifts[i].quantity);

            gifts[i].totalPrice =       // after the choice and the quantity, he need to be aware of the amount that he need to afford
                gifts[i].quantity * gifts[i].unitPrice;

            printf("Updated successfully.\n");
            return;
        }
    }
}
int main() {
    int option;
    int id;

    do {
        printf("\n=== Wedding Gift Management ===\n");
        printf("1. Add a gift\n");
        printf("2. Display all gifts\n");
        printf("3. Display one gift\n");
        printf("4. Update a gift\n");
        printf("5. Delete a gift\n");
        printf("0. Exit\n");
        printf("Choose an option: ");
        scanf("%d", &option);

        switch(option) {
            case 1:
                addGift();
                break;
            case 2:
                displayAll();
                break;
            case 3:
                printf("Enter guest ID: ");
                scanf("%d", &id);
                displayOne(id);
                break;
            case 4:
                printf("Enter guest ID: ");
                scanf("%d", &id);
                updateGift(id);
                break;
            case 5:
                printf("Enter guest ID: ");
                scanf("%d", &id);
                deleteGift(id);
                break;
            case 0:
                printf("Goodbye!\n");
                break;
            default:
                printf("Invalid option.\n");
        }
    } while(option != 0);

    return 0;
}
