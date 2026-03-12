#include<stdio.h>
#include<string.h>
typedef enum { LE, LA } Side;

typedef struct {
    int id;
    char name[100];
    int age;
    char social_class[50];
    Side side;
} Person;

Person create_person(){
    Person p;

    printf("Enter your ID: ");
    scanf("%d", &p.id);

    printf("Enter your name: ");
    scanf("%s", &p.name);

    printf("Enter your age: ");
    scanf("%d", &p.age);

    printf("Enter your social class: ");
    scanf("%s", &p.social_class);

    printf("Enter your side (0 = LE, 1 = LA): ");
    scanf("%d", &p.side);

    return p;
}
int main(){
    Person person1;
    person1 = create_person();

    return 0;
}

