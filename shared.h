#include <stdio.h>

#ifndef SHARED_H
#define SHARED_H

// struct for card
typedef struct {
    char rank;
    char suit;
} Card;

// enum for exit status of player
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
    int *pipeIn;
    int *pipeOut;
    FILE *fileIn;
    FILE *fileOut;
} Player;

// struct for particular play of a card
typedef struct {
    unsigned int player;
    Card card;
} Play;

// struct for player's record of the game.
typedef struct {
    Card hand[60];
    int handSize;
    unsigned int playerMove; // will be 0 - playerNumber
    int myID;
    int threshold;
    int playerCount;
    int leadPlayer;
    char leadSuit;

    char *current;
    int expected;
    int round;

    int *order;
    int orderPos;
    Card *cardsPlayed;
    int cardPos;

    int (*playerStrategy)();
    int dPlayedRound;
    int *dPlayerNumber;
    char *cardsFromRound;
    char **cardsStored;
    int largestPlayer;
    int roundWinner;
    int firstRound; // 0 if not, 1 if so.
    int lastPlayer;

} PlayerGame;

char validate_card(char c);

void play_card(PlayerGame *game, Card *play);

void save_card(PlayerGame *game, Card *card);

int (*playerStrategy)(PlayerGame *game);

int number_digits(int i);

void player_end_of_round_output(PlayerGame *game);

int check_expected(PlayerGame *game, char *got, int currentPlayer);

void set_expected(PlayerGame *game, char *set);

void init_expected(PlayerGame *game);

int get_rank_integer(char arg);

int card_in_lead_suit(PlayerGame *game);

Card lowest_in_suit(PlayerGame *game, char suit);

void remove_card(PlayerGame *game, Card *card);

void alice_default_move(PlayerGame *game);

PlayerStatus show_player_message(PlayerStatus s);

int decode_hand(char *input, PlayerGame *game);

int decode_newround(char *input, PlayerGame *game);

int decode_played(char *input, PlayerGame *game);

int extract_last_player(char *input);

int process_input(char *input, PlayerGame *game);

int cont_read_stdin(PlayerGame *game);

int further_arg_checks(int argc, char **argv, PlayerGame *game);

int parse_player(int argc, char **argv, PlayerGame *game);

#endif







