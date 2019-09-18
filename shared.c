#include <stdio.h>
#include "shared.h"
//todo validate change.

char validate_hand(char *str) {
    return
            str[0] == 'H' &&
            str[1] == 'A' &&
            str[2] == 'N' &&
            str[3] == 'D';
}

char validate_newround(char *str) {
    return
            str[0] == 'N' &&
            str[1] == 'E' &&
            str[2] == 'W' &&
            str[3] == 'R' &&
            str[4] == 'O' &&
            str[5] == 'U' &&
            str[6] == 'N' &&
            str[7] == 'D';
}

char validate_played(char *str) {
    return
            str[0] == 'P' &&
            str[1] == 'L' &&
            str[2] == 'A' &&
            str[3] == 'Y' &&
            str[4] == 'E' &&
            str[5] == 'D';
}

char validate_gameover(char *str) {
    return
            str[0] == 'G' &&
            str[1] == 'A' &&
            str[2] == 'M' &&
            str[3] == 'E' &&
            str[4] == 'O' &&
            str[5] == 'V' &&
            str[6] == 'E' &&
            str[7] == 'R';
}

char validate_play(char *str) {
    return
            str[0] == 'x';
}

char validate_card(char c) {
    return
            c == 'S' ||
            c == 'C' ||
            c == 'D' ||
            c == 'H';
}




/*typedef struct {
    Card hand[60];
    int handSize;
    unsigned int current; // will be 0 - playerNumber
    int myID;
    int threshold;
    int playerCount;
    int leadPlayer;
    char* next;
    int expected;
} PlayerGame;*/


