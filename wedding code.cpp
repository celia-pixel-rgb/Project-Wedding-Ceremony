#include <stdio.h>
#include "person.h"

Person create_person(void) {

    Person p;
    int choice;

    printf("Enter ID: ");
    scanf("%d", &p.id);
    while (getchar() != '\n');

    printf("Enter Name: ");
    scanf(" %[^\n]", p.name);

    printf("Enter Age: ");
    scanf("%d", &p.age);
    while (getchar() != '\n');

    printf("Enter Social Class: ");
    scanf(" %[^\n]", p.social_class);

    printf("Choose Side:\n");
    printf("1. LE\n");
    printf("2. LA\n");
    printf("Choice: ");
    scanf("%d", &choice);

    if (choice == 1)
        p.side = LE;
    else
        p.side = LA;

    return p;
}

void display_person(Person p) {
    printf("\n--- Person Details ---\n");
    printf("ID           : %d\n", p.id);
    printf("Name         : %s\n", p.name);
    printf("Age          : %d\n", p.age);
    printf("Social Class : %s\n", p.social_class);
    if (p.side == LE)
        printf("Side : LE (Groom side)\n");
    else
        printf("Side : LA (Bride side)\n");
}

void update_person(int id) {
    printf("Update function - coming soon\n");
}

int main(void) {
    Person p = create_person();
    display_person(p);
    return 0;
}


