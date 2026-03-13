#include <stdio.h>
#include <string.h>

typedef enum { LE, LA } Side;

typedef struct {
    int id;
    char name[100];
    int age;
    char social_class[50];
    Side side;
} Person;

Person create_person() {
    Person p;
    int side_choice;

    printf("\nEnter ID: ");
    scanf("%d", &p.id);

    printf("Enter name: ");
    scanf("%s", p.name);

    printf("Enter age: ");
    scanf("%d", &p.age);

    printf("Enter social class: ");
    scanf("%s", p.social_class);

    printf("Side (0 = Groom side LE, 1 = Bride side LA): ");
    scanf("%d", &side_choice);

    p.side = (side_choice == 0) ? LE : LA;

    return p;
}

void display_person(Person p) {
    printf("\n--- Person Information ---\n");
    printf("ID: %d\n", p.id);
    printf("Name: %s\n", p.name);
    printf("Age: %d\n", p.age);
    printf("Social Class: %s\n", p.social_class);

    if (p.side == LE)
        printf("Side: Groom (LE)\n");
    else
        printf("Side: Bride (LA)\n");
}

void update_person(Person *p) {
    int side_choice;

    printf("\nUpdating person ID %d\n", p->id);

    printf("Enter new name: ");
    scanf("%s", p->name);

    printf("Enter new age: ");
    scanf("%d", &p->age);

    printf("Enter new social class: ");
    scanf("%s", p->social_class);

    printf("Side (0 = Groom LE, 1 = Bride LA): ");
    scanf("%d", &side_choice);

    p->side = (side_choice == 0) ? LE : LA;
}

int main() {
    int n;
    printf("Enter number of guests in the marriage ceremony: ");
    scanf("%d", &n);

    Person guests[n];
    printf("\n--- Enter guest information ---\n");

    for (int i = 0; i < n; i++) {
        printf("\nGuest %d:\n", i + 1);
        guests[i] = create_person();
    }
    printf("\n--- Guest List ---\n");

    for (int i = 0; i < n; i++) {
        display_person(guests[i]);
    }

    int index;
    printf("\nEnter guest index to update (1 to %d): ", n);
    scanf("%d", &index);

    if (index >= 1 && index <= n) {
        update_person(&guests[index - 1]);
    }

    printf("\n--- Updated Guest List ---\n");

    for (int i = 0; i < n; i++) {
        display_person(guests[i]);
    }

    return 0;
}
