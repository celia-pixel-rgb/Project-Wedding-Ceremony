#ifndef PERSON_H
#define PERSON_H

typedef enum { GROOM, BRIDE, BOTH } Side;
//it converts enum to readable string 
const char *side_to_string(Side s) {
    return (s == GROOM) ? "Groom" : (s == BRIDE) ? "Bride" : "Both";
}

typedef struct {   //the structure or parameters representing a guest
    int  id;
    char name[100];
    int  age;
    char status[50];
    char phone[20];
    Side side;
    char parking[10];
} Guest;
// function declarartion
void addGuest(Guest p);
void displayGuest();
void deleteGuest();
void updateGuest();

#endif
