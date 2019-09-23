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
 * Function to handle bob's move as the lead player
 * @param game struct representing player's tracking of game.
 * @return 0 when done.
 */
int bob_lead_move(PlayerGame *game) {
    // order in which to check suits
    char *suits = "DHSC";
    Card play;
    play.rank = -1;
    // find the lowest card in the suit
    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == suits[0]) {
            play = lowest_in_suit(game, suits[0]);
        }
        // if we have reached end of hand
        if (i == (game->handSize - 1)) {
            i = -1;
            // found a card, break out
            if (play.rank != -1) {
                break;
            }
            // end of search
            if (strcmp(suits, "C") == 0) {
                break;
            }
            // increase suit to search for.
            suits++;
        }
    }

    // play the card found.
    play_card(game, &play);
    return DONE;
}

/**
 * Function to handle bob's move regarding D cards played.
 * @param game struct representing player's tracking of game.
 * @return 0 when done.
 */
int bob_d_card_move(PlayerGame *game) {
    // if we have a card in the lead suit
    if (card_in_lead_suit(game) == DONE) {
        int rank = 0;
        Card play;
        play.rank = -1;
        // search for the highest card in the lead suit
        for (int i = 0; i < game->handSize; i++) {
            if (game->hand[i].suit == game->leadSuit) {
                if (get_rank_integer(game->hand[i].rank) >= rank) {
                    rank = get_rank_integer(game->hand[i].rank);
                    play.rank = game->hand[i].rank;
                    play.suit = game->hand[i].suit;
                }
            }
        }
        // play the highest card
        play_card(game, &play);
        return DONE;
    } else {
        // search for cards in order below
        char *suits = "SCHD";
        Card play;
        play.rank = -1;
        // find the lowest card in the given suit
        for (int i = 0; i < game->handSize; i++) {
            if (game->hand[i].suit == suits[0]) {
                play = lowest_in_suit(game, suits[0]);
            }
            // if we have reached end of hand size.
            if (i == (game->handSize - 1)) {
                i = -1;
                // if we have found a card, break.
                if (play.rank != -1) {
                    break;
                }
                // end of search
                if (strcmp(suits, "C") == 0) {
                    break;
                }
                // move to next suit in order.
                suits++;
            }
        }

        // play the card found.
        play_card(game, &play);
        return DONE;
    }
}

/**
 * Function to handle bob's default move.
 * @param game struct representing player's tracking of game.
 * @return 0 when done.
 */
int bob_default_move(PlayerGame *game) {
    // order in which to search for a card
    char *suits = "SCDH";
    char rank = 0;
    Card play;
    play.rank = -1;
    // search for the highest card
    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == suits[0]) {
            if (game->hand[i].rank >= rank) {
                rank = game->hand[i].rank;
                play.rank = game->hand[i].rank;
                play.suit = game->hand[i].suit;
            }
        }
        // if we have reached end of hand
        if (i == (game->handSize - 1)) {
            i = -1;
            // found a card, break
            if (play.rank != -1) {
                break;
            }
            // end of search
            if (strcmp(suits, "H") == 0) {
                break;
            }
            // increase suit currently searching for.
            suits++;
        }
    }

    // play the card found.
    play_card(game, &play);
    return DONE;
}

/**
 * Function to check if there is a player that has won D cards over threshold.
 * @param game struct representing player's tracking of game.
 * @return 1 if true, 0 if false.
 */
int player_won_over_threshold(PlayerGame *game) {
    // for all players
    for (int i = 0; i < game->playerCount; i++) {
        // if this player has won threshold - 2 D cards.
        if (game->dPlayerNumber[i] >= (game->threshold - 2)) {
            return 1;
        }
    }
    return 0;
}

/**
 * Function to see if there have been any D cards played this round.
 * @param game struct representing player's tracking of game.
 * @return 1 if true, 0 if false.
 */
int d_cards_in_round(PlayerGame *game) {
    if (game->dPlayedRound > 0) {
        return 1;
    }
    return 0;
}

/**
 * Function to handle overarching decision making of bob's activity.
 * @param game struct representing player's tracking of game.
 * @return 0 when done.
 */
int bob_strategy(PlayerGame *game) {
    //lead move
    if (game->leadPlayer == game->myID) {
        bob_lead_move(game);
        return DONE;
    }

    // D card move - if a D card has been played in the round & someone has
    // won over threshold - 2 D cards
    if ((player_won_over_threshold(game) == 1) &&
            (d_cards_in_round(game) == 1)) {
        bob_d_card_move(game);
        return DONE;
    }

    //if card in lead suit
    if (card_in_lead_suit(game) == DONE) {
        // find lowest card & play it.
        Card play = lowest_in_suit(game, game->leadSuit);
        play_card(game, &play);
        return DONE;
    }

    //default move
    bob_default_move(game);

    if (game->myID == game->playerCount - 1) {
        // all players have moved, output.
        player_end_of_round_output(game);
    }
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
        // setup variables for bob player & check args
        PlayerGame game;
        int parseStatus = parse_player(argc, argv, &game);
        if (parseStatus != 0) {
            return parseStatus;
        }
        init_expected(&game);
        // output @ for hub recognition
        fprintf(stdout, "@");
        fflush(stdout);
        // set up function pointer for bob's moveset
        game.playerStrategy = bob_strategy;
        // read hub's messages
        return cont_read_stdin(&game);
    } else {
        // incorrect arg count.
        return show_player_message(ARGERR);
    }
}