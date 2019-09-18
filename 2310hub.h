#include "shared.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef HUB_H
#define HUB_H

// struct for the game
typedef struct {
    Deck deck;
    Play *board;   //todo fix for number of players
    Player *players;  //todo fix for number of players
    char *types; //todo fix for number of players
    unsigned int current; // will be 0 - playerNumber
    int threshold;
    int playerCount;
    int leadPlayer;
    int numCardsToDeal;
} Game;

typedef enum {
    OK = 0,
    LESS4ARGS = 1,
    THRESHOLD = 2,
    BADDECKFILE = 3,
    SHORTDECK = 4,
    PLAYERSTART = 5,
    PLAYEREOF = 6,
    PLAYERMSG = 7,
    PLAYERCHOICE = 8,
    GOTSIGHUP = 9
} Status;

//typedef struct {
//    int in;
//    int out;
//} Pipe;

int handler_deck(char* deckName, Game *game);

int load_deck(FILE* input, Deck* deck);

int player_arg_checker(int argc, char** argv, Game *game);

void handle_sighup(int s);

int game_loop(Game *game);

Status show_message(Status s);

int parse(int argc, char** argv, Game *game);

#endif