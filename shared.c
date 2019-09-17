#include <stdio.h>
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


// enum for exit status
typedef enum {
    DONE = 0,
    ARGERR = 1,
    PLAYERERR = 2,
    POSERR = 3,
    THRESHERR = 4,
    HANDERR = 5,
    MSGERR = 6,
    EOFERR = 7
} PlayerStatus;

// struct for card
typedef struct {
    char rank;
    char suit;
} Card;


// struct for deck
typedef struct {
    unsigned int count;
    unsigned int used;
    Card *contents;
} Deck;

// struct for player
typedef struct {
    unsigned int size;
    Card cards[60]; //max number of cards for one player (15 * 4 suits)
    int* pipeIn;
    int* pipeOut;
    FILE* fileIn;
    FILE* fileOut;
} Player;

// struct for particular play of a card
typedef struct {
    unsigned int player;
    Card card;
} Play;

typedef struct {
    Card hand[60];
    int handSize;
    unsigned int current; // will be 0 - playerNumber
    int myID;
    int threshold;
    int playerCount;
    int leadPlayer;
    char* next;
    int expected;
} PlayerGame;

int check_expected(PlayerGame *game, char* got, int currentPlayer);
void set_expected(PlayerGame *game, char* set);
void init_expected(PlayerGame *game);
