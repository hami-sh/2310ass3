#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>
#include "shared.h"
#include <ctype.h>

/**
 * Function to handle alice's move set & decisions.
 * @param game struct representing player's tracking of game.
 */
void alice_lead_move(PlayerGame *game) {
    // order to search cards in
    char *suits = "SCDH";
    char rank = 0;
    Card play;
    play.rank = -1;
    // search for highest card
    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == suits[0]) {
            if (game->hand[i].rank >= rank) {
                rank = game->hand[i].rank;
                play = game->hand[i];
            }
        }
        // if we reach the end of hand size
        if (i == (game->handSize - 1)) {
            i = -1;
            // found a card, start again
            if (play.rank != -1) {
                break;
            }
            // if we are at the end of the search, break
            if (strcmp(suits, "H") == 0) {
                break;
            }
            // move to next suit
            suits++;
        }
    }

    // play the card chosen.
    play_card(game, &play);
}

/**
 * Function to handle the 'default' alice move (last option)
 * @param game struct representing player's tracking of game.
 */
void alice_default_move(PlayerGame *game) {
    // order of suits to search for.
    char *suits = "DHSC";
    char rank = 0;
    Card play;
    play.rank = -1;
    // find the largest card
    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == suits[0]) {
            if (game->hand[i].rank >= rank) {
                rank = game->hand[i].rank;
                play.rank = game->hand[i].rank;
                play.suit = game->hand[i].suit;
            }
        }
        // if we reach the end of hand size
        if (i == (game->handSize - 1)) {
            i = -1;
            // found a card, start again
            if (play.rank != -1) {
                break;
            }
            // end of the search
            if (strcmp(suits, "C") == 0) {
                break;
            }
            suits++;
        }
    }

    // play the card.
    play_card(game, &play);
}


/**
 * Strategy for alice movements.
 * Will print out the move made to stdout.
 * @param game struct representing player's tracking of game.
 * @return int - 0 when done.
 */
int alice_strategy(PlayerGame *game) {
    //if lead player.
    if (game->leadPlayer == game->myID) {
        alice_lead_move(game);
        return DONE;
    }

    //if card in lead suit
    if (card_in_lead_suit(game) == DONE) {
        Card play = lowest_in_suit(game, game->leadSuit);
        play_card(game, &play);
        return DONE;
    }

    //default move
    alice_default_move(game);

    return DONE;
}

/**
 * Function acting as entry point for the program when first loaded.
 * @param argc - number of arguments supplied at command line.
 * @param argv - array of strings supplied at startup.
 * @return 0 - normal exit
 *         1 - incorrect number of arguments
 *         2 - number of players < 2 or not a number
 *         3 - invalid position for number of players
 *         4 - threshold < 2 or not a number
 *         6 - invalid message from hub.
 *         7 - unexpected EOF from hub.
 */
int main(int argc, char **argv) {
    if (argc == 5) {
        // create the player
        PlayerGame game;
        int parseStatus = parse_player(argc, argv, &game);
        if (parseStatus != 0) {
            return parseStatus;
        }
        init_expected(&game);

        // output @ for hub recognition
        fprintf(stdout, "@");
        fflush(stdout);

        //set up function pointer to alice's moveset
        game.playerStrategy = alice_strategy;
        // wait for hub input
        return cont_read_stdin(&game);
    } else {
        // incorrect arg count.
        return show_player_message(ARGERR);
    }
}

