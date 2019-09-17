#include <stdio.h>
#ifndef SHARED_H
#define SHARED_H
//todo validate change.

char validate_hand(char *str);

char validate_newround(char *str);

char validate_played(char *str);

char validate_gameover(char *str);

char validate_play(char *str);

char validate_card(char c);


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
    unsigned int playerMove; // will be 0 - playerNumber
    int myID;
    int threshold;
    int playerCount;
    int leadPlayer;
    char* current;
    int expected;
    int round;
    int* order;
    int orderPos;
    Card* cardsPlayed;
} PlayerGame;

int check_expected(PlayerGame *game, char* got, int currentPlayer);
void set_expected(PlayerGame *game, char* set);
void init_expected(PlayerGame *game);



#endif






