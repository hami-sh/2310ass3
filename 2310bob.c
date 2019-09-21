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


int bob_lead_move(PlayerGame *game) {
    char *suits = "DHSC";
    Card play;
    play.rank = -1;
    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == suits[0]) {
            play = lowest_in_suit(game, suits[0]);
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
    printf("PLAY%c%c\n", play.suit, play.rank);
    remove_card(game, &play);
    return DONE;
}

int bob_D_card_move(PlayerGame *game) {
    if (card_in_lead_suit(game) == DONE) {
        printf("::\n");
        char rank = 0;
        Card play;
        play.rank = -1;
        for (int i = 0; i < game->handSize; i++) {
//        printf("%d <%c%c>\n", i, game->hand[i].suit, game->hand[i].rank);
            if (game->hand[i].suit == game->leadSuit) {
//            printf("%d vs %d\n", game->hand[i].rank, rank);
                if (game->hand[i].rank >= rank) {
                    rank = game->hand[i].rank;
                    play.rank = game->hand[i].rank;
                    play.suit = game->hand[i].suit;
//                printf("//%c%c//\n", game->hand[i].suit, game->hand[i].rank);
                }
            }
        }
        printf("PLAY%c%c\n", play.suit, play.rank);
        remove_card(game, &play);
        return DONE;
    } else {
        printf("}}\n");
        char *suits = "SCHD";
        Card play;
        play.rank = -1;
        for (int i = 0; i < game->handSize; i++) {
            if (game->hand[i].suit == suits[0]) {
                play = lowest_in_suit(game, suits[0]);
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
        printf("PLAY%c%c\n", play.suit, play.rank);
        remove_card(game, &play);
        return DONE;
    }
}

int bob_default_move(PlayerGame *game) {
    char *suits = "SCDH";
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
            if (strcmp(suits, "H") == 0) {
                break;
            }
            suits++;
        }
    }
    printf("PLAY%c%c\n", play.suit, play.rank);
    remove_card(game, &play);
    return DONE;
}

int player_won_over_threshold(PlayerGame *game) {
    for (int i = 0; i < game->playerCount; i++) {
        if (game->dPlayerNumber[i] >= (game->threshold - 2)) {
            return 1;
        }
    }
    return 0;
}

int d_cards_in_round(PlayerGame *game) {
    if (game->dPlayedRound > 0) {
        return 1;
    }
    return 0;
}

int bob_strategy(PlayerGame *game) {
    //lead move
    if (game->leadPlayer == game->myID) {
        //printf("LEAD\n");
        bob_lead_move(game);
        return DONE;
    }

    //D card move
    if ((player_won_over_threshold(game) == 1) &&
            (d_cards_in_round(game) == 1)) {
        //printf("D move\n");
        bob_D_card_move(game);
        return DONE;
    }

    //if card in lead suit
    if (card_in_lead_suit(game) == DONE) {
        //printf("card in lead suit\n");
        Card play = lowest_in_suit(game, game->leadSuit);
        printf("PLAY%c%c\n", play.suit, play.rank);
        remove_card(game, &play);
        return DONE;
    }

    //default move
    bob_default_move(game);

    if (game->myID == game->playerCount - 1) {
//        printf("yeehaw\n");
        //set_expected(game, "NEWROUND"); // all players have moved.
        player_end_of_round_output(game);
    }
    return DONE;
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
        game.player_strategy = bob_strategy;
        return cont_read_stdin(&game);
    } else {
        return show_player_message(ARGERR);
    }
}