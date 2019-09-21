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
    char *suits = "SCDH";
    char rank = 0;
    Card play;
    play.rank = -1;
    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == suits[0]) {
            if (game->hand[i].rank >= rank) {
                rank = game->hand[i].rank;
                play = game->hand[i];
            }
        }
        if (i == (game->handSize - 1)) {
            i = -1;
            if (play.rank != -1) {
                break;
            }
            if (strcmp(suits, "H") == 0) {
                break;
            }
            suits++;
        }
    }

    save_card(game, &play);
    //game->cardsPlayed[game->cardPos++] = play;
    printf("PLAY%c%c\n", play.suit, play.rank);
    remove_card(game, &play);
}

/**
 * Function to handle the 'default' alice move (last option)
 * @param game struct representing player's tracking of game.
 */
void alice_default_move(PlayerGame *game) {
//    printf("DEFAULT\n");
    char *suits = "DHSC";
    char rank = 0;
    Card play;
    play.rank = -1;
    for (int i = 0; i < game->handSize; i++) {
//        printf("%d <%c%c>\n", i, game->hand[i].suit, game->hand[i].rank);
        if (game->hand[i].suit == suits[0]) {
//            printf("%d vs %d\n", game->hand[i].rank, rank);
            if (game->hand[i].rank >= rank) {
                rank = game->hand[i].rank;
                play.rank = game->hand[i].rank;
                play.suit = game->hand[i].suit;
//                printf("//%c%c//\n", game->hand[i].suit, game->hand[i].rank);
            }
        }
        if (i == (game->handSize - 1)) {
            i = -1;
            if (play.rank != -1) {
                break;
            }
            if (strcmp(suits, "C") == 0) {
                break;
            }
            suits++;
        }
    }
    save_card(game, &play);
    game->cardPos += 1;
    printf("PLAY%c%c\n", play.suit, play.rank);
    remove_card(game, &play);
}


/**
 * Strategy for alice movements.
 * Will print out the move made to stdout.
 * @param game struct representing player's tracking of game.
 * @return int - 0 when done.
 */
int alice_strategy(PlayerGame *game) {
//    for (int i = 0; i < game->handSize; i++) {
//        printf("-(%c%c)-", game->hand[i].suit, game->hand[i].rank);
//    }
    //if lead - order S C D H
    if (game->leadPlayer == game->myID) {
        //printf("LEAD\n");
        alice_lead_move(game);
        return DONE;
    }

    //if card in lead suit
    if (card_in_lead_suit(game) == DONE) {
        //printf("2nd\n");
        Card play = lowest_in_suit(game, game->leadSuit);
        save_card(game, &play);
        game->cardPos += 1;
        printf("PLAY%c%c\n", play.suit, play.rank);
        remove_card(game, &play);
        return DONE;
    }

    //default move
    alice_default_move(game);

    if (game->myID == game->playerCount - 1) {
//        printf("yeehaw\n");
        //set_expected(game, "NEWROUND"); // all players have moved.
        player_end_of_round_output(game);
    }

    return DONE; //todo change
}

int main(int argc, char **argv) {
    if (argc == 5) {
        PlayerGame game;
        int parseStatus = parse_player(argc, argv, &game);
        if (parseStatus != 0) {
            return parseStatus;
        }
        init_expected(&game);
        fprintf(stdout, "@");
        fflush(stdout);
        game.player_strategy = alice_strategy;
        return cont_read_stdin(&game);
    } else {
        return show_player_message(ARGERR);
    }
}

