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

#define LINESIZE 80

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
        printf("PLAY%c%c\n", play.suit, play.rank);
        remove_card(game, &play);
        return DONE;
    }

    //default move
    alice_default_move(game);

    if (game->myID == game->playerCount - 1) {
//        printf("yeehaw\n");
        //set_expected(game, "NEWROUND"); // all players have moved.
    }

    return DONE; //todo change
}

/* shared */

/**
 * Function to remove a card from the hand of a player.
 * @param game struct representing player's tracking of game.
 * @param game struct representing card to remove.
 */
void remove_card(PlayerGame *game, Card *card) {
    int pos = 0;
    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == card->suit) {
            if (game->hand[i].rank == card->rank) {
                pos = i;
                break;
            }
        }
    }

//    printf("-<%c%c>-\n", card->suit, card->rank);
//    for (int i = 0; i < game->handSize; i++) {
//        printf("-(%c%c)-", game->hand[i].suit, game->hand[i].rank);
//    }
//    printf("\n");
    for (int q = pos; q < game->handSize - 1; q++) {
        // shift all cards to compensate for removal.
        game->hand[q] = game->hand[q + 1];
    }
    game->handSize -= 1;
//    for (int i = 0; i < game->handSize; i++) {
//        printf("-(%c%c)-", game->hand[i].suit, game->hand[i].rank);
//    }

}

/**
 * Function to see if there is a card in the lead suit.
 * @param game struct representing player's tracking of game.
 * @return 0 - if present
 *         -1 - if not present
 */
int card_in_lead_suit(PlayerGame *game) {
    for (int i = 0; i < game->handSize; i++) {
        if (game->leadSuit == game->hand[i].suit) {
            return DONE;
        }
    }
    return -1;
}

/**
 * Function to return the lowest card for a given suit
 * @param game struct representing player's tracking of game.
 * @param suit char representing suit we want
 * @return play - the lowest card
 */
Card lowest_in_suit(PlayerGame *game, char suit) {
    int rank = 17;
    Card play;
    play.rank = -1;
    for (int i = 0; i < game->handSize; i++) {
        if (suit == game->hand[i].suit) {
            if (get_rank_integer(game->hand[i].rank) < rank) {
                rank = get_rank_integer(game->hand[i].rank);
                play = game->hand[i];
            }
        }
    }
    return play;
}

/**
 * Function to get the integer rank of a card from its char rank.
 * @param arg - char to convert
 * @return int representing char.
 */
int get_rank_integer(char arg) {
    if (isdigit(arg) != 0) {
        return arg - '0';
    } else {
        return arg - 86;
    }
}

/**
 * Function to handle printing error messages to stderr.
 * @param s - which status to show
 * @return the error status.
 */
PlayerStatus show_player_message(PlayerStatus s) {
    const char *messages[] = {"",
                              "Usage: player players myid threshold handsize\n",
                              "Invalid players\n",
                              "Invalid position\n",
                              "Invalid threshold\n",
                              "Invalid hand size\n",
                              "Invalid message\n",
                              "EOF\n"};
    fputs(messages[s], stderr);
    return s;
}

/**
 * Function to check repeating cards in an array.
 * @param arr - Card array to check
 * @param size - size of the array
 * @return 0 - no repeats
 *         6 - repeats
 */
int check_repeating_cards(Card arr[], int size) {
    for (int i = 0; i < size; i++) {
        for (int j = i + 1; j < size; j++) {
            if (arr[i].rank == arr[j].rank) {
                if (arr[i].suit == arr[j].suit) {
                    return show_player_message(MSGERR);
                }
            }
        }
    }
    return DONE;
}

/**
 * Function to decode the hand message from stdin.
 * @param input - string representing message
 * @param game struct representing player's tracking of game.
 * @return 0 - successfully decoded
 *         5 - error in hand encode
 *         6 - error in message
 */
int decode_hand(char *input, PlayerGame *game) {
    input += 4;
    input[strlen(input) - 1] = '\0'; // remove extra new line char.
    //int inputSize = strlen(input);
    char delim[] = ",";
    char *arrow = strtok(input, delim);

    int i = 0;
    while (arrow != NULL) {
        if (i == 0) {
            for (int j = 0; j < strlen(arrow); j++) {
                if (!isdigit(arrow[j])) {
                    return show_player_message(HANDERR);
                }
            }
            int tempHandSize;
            sscanf(arrow, "%d", &tempHandSize);
            if (tempHandSize != game->handSize) {
                return show_player_message(HANDERR);
            }
        } else if (i > 0) {
            if (strlen(arrow) > 2) {
                return show_player_message(MSGERR);
            } else if (validate_card(arrow[0]) && (isdigit(arrow[1])
                                                   || isxdigit(arrow[1]))) {
                Card card;
                card.suit = arrow[0];
                card.rank = arrow[1];
                game->hand[i - 1] = card;
            } else {
                return show_player_message(MSGERR);
            }

        }
        arrow = strtok(NULL, delim);
        i++;
    }

    if (i - 1 != game->handSize) {
        return show_player_message(MSGERR);
    }

    int repeatStatus = check_repeating_cards(game->hand, game->handSize);
    if (repeatStatus != 0) {
        return repeatStatus;
    }
    return DONE;
}

/**
 * Function to decode the newround message from stdin.
 * @param input - string representing message
 * @param game struct representing player's tracking of game.
 * @return 0 - successfully decoded
 *         6 - error in message
 */
int decode_newround(char *input, PlayerGame *game) {
    game->orderPos = 0;
    input += 8;
    input[strlen(input) - 1] = '\0';
    // get lead player
    char *leadStr = malloc(strlen(input) * sizeof(char));
    strncpy(leadStr, input, strlen(input));
    int i;
    for (i = 0; i < strlen(leadStr); i++) {
        if (!isdigit(leadStr[i])) {
            return show_player_message(MSGERR);
        }
    }
    if (i == 0) {
        return show_player_message(MSGERR);
    }

    game->leadPlayer = atoi(leadStr);

    // make move!
    if (game->myID == 0) {
        alice_strategy(game);
        game->orderPos += 1;
    }
    return DONE;
}

/**
 * Function to decode the played message from stdin.
 * @param input - string representing message
 * @param game struct representing player's tracking of game.
 * @return 0 - successfully decoded
 *         6 - error in message
 */
int decode_played(char *input, PlayerGame *game) {
    Card newCard;
    //PLAYEDid,CARD
    //printf("played\n");
    input += 6;
    input[strlen(input) - 1] = '\0';
    int i = 0;
    while (input[i] != ',') {
        if (!isdigit(input[i])) {
            return show_player_message(MSGERR);
        }
        i++;
    }
    char *playedID = malloc(i * sizeof(char));
    strncpy(playedID, input, i);
    int justPlayed = atoi(playedID);

    //printf("-<<%d>><%d>><<%d>>-\n", justPlayed, game->order[game->playerCount - 1], game->playerCount-1);
    if (justPlayed != game->order[game->orderPos]) {
        return show_player_message(MSGERR);
    } else if (justPlayed == (game->playerCount - 1)) {
        //printf("RESET{}\n");
        game->orderPos = 0;
        set_expected(game, "HAND");
    } else {
        game->orderPos++;
    }
    //todo is this an issue
    game->cardsPlayed = malloc(game->handSize * game->playerCount *
                               sizeof(Card));


    input += i;
    input += 1;

    if (validate_card(input[0]) && (isdigit(input[1]) ||
                                    (isalpha(input[1]) && isxdigit(input[1]) && islower(input[1])))) {
        newCard.suit = input[0];
        newCard.rank = input[1];
        game->cardsPlayed[game->cardPos++] = newCard;
    } else {
        return show_player_message(MSGERR);
    }
    if (strlen(input) != 2) {
        return show_player_message(MSGERR);
    }

    if (justPlayed == game->leadPlayer) {
        game->leadSuit = newCard.suit;
    }

    if ((justPlayed + 1) == game->myID) {
        alice_strategy(game);
        game->orderPos += 1;
    }

    return DONE;
}

/**
 * Function to handle gameover message from stdin.
 * @param input - string representing message
 * @return fixme
 */
int gameover(char *input) {

    return DONE;
}

/**
 * Function to extract the player portion of the played message.
 * @param input - string representing message
 * @return 0 - successfully decoded
 *         6 - error in message
 */
int extract_last_player(char *input) {
    char *dest = malloc(sizeof(char) * strlen(input));
    strncpy(dest, input, strlen(input));
    dest += 6;
    int i = 0;
    while (dest[i] != ',') {
        if (i == strlen(dest)) {
            return -1;
        }
        if (!isdigit(dest[i])) {
            return show_player_message(MSGERR);
        }
        i++;
    }
    return atoi(dest);
}

int process_input(char *input, PlayerGame *game) {
    char dest[500];
    if (validate_hand(input)) {
        strncpy(dest, input, 4);
        dest[4] = 0;
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
            return msgCheck;
        }
        decode_hand(input, game);
    } else if (validate_newround(input)) {
        game->expected = 0;
        strncpy(dest, input, 8);
        dest[8] = 0;
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
            return msgCheck;
        }
        decode_newround(input, game);
    } else if (validate_played(input)) {
        //todo get current from string
        game->playerMove = extract_last_player(input);
        strncpy(dest, input, 6);
        dest[6] = 0;
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
            return msgCheck;
        }
        decode_played(input, game);
    } else if (validate_gameover(input)) {
        gameover(input);

    } else {
        show_player_message(MSGERR);
    }
    return 0;
}

/**
 * Function to read from stdin to get messages
 * @param game struct representing player's tracking of game.
 * @return 0 - clean exit
 *         6 - error in hub message
 *         7 - EOF from hub
 */
int cont_read_stdin(PlayerGame *game) {
    char input[LINESIZE];
    fgets(input, BUFSIZ, stdin);
    while (strcmp(input, "GAMEOVER\n") != 0 && !feof(stdin)) { //fixme check \n?
        //TODO DEBUG REMOVE;
        if (validate_play(input)) {
            alice_strategy(game);
            game->orderPos += 1;
            //printf("X-<<%d>>-%d\n", game->order[game->orderPos], game->orderPos);
            fgets(input, BUFSIZ, stdin);
            continue;
        }
        int processed = process_input(input, game);
        if (processed != 0) {
            return processed;
        }
        fgets(input, BUFSIZ, stdin);
    }
    if (feof(stdin)) {
        return show_player_message(EOFERR);
    }
    return DONE;
}

/**
 * Function to perform further argument checking for player.
 * @param argc - number of arguments
 * @param argv - array of string arguments.
 * @param game struct representing player's tracking of game.
 * @return 1 - less than 4 arguments
 *         2 - threshold less than 2 or not a number
 *         3 - problem reading the deck
 *         4 - Less than P cards in the deck
 */
int further_arg_checks(int argc, char **argv, PlayerGame *game) {
    // check threshold
    char *thresholdArg = malloc(sizeof(char) * strlen(argv[3]));
    if (strlen(argv[3]) == 0) {
        return show_player_message(PLAYERERR);
    }
    for (int s = 0; s < strlen(argv[3]); s++) {
        //check if floating point
        if (argv[3][s] == '.') {
            return show_player_message(THRESHERR);
        }
        // check that dimensions supplied are digits.
        if (!isdigit(argv[3][s])) {
            return show_player_message(THRESHERR);
        }
        //store each character of the number
        thresholdArg[s] = argv[3][s];
    }
    sscanf(thresholdArg, "%d", &game->threshold);
    free(thresholdArg);
    if (game->threshold < 2) {
        return show_player_message(THRESHERR);
    }

    // hand size
    char *handArg = malloc(sizeof(char) * strlen(argv[4]));
    if (strlen(argv[4]) == 0) {
        return show_player_message(PLAYERERR);
    }
    for (int s = 0; s < strlen(argv[4]); s++) {
        //check if floating point
        if (argv[4][s] == '.') {
            return show_player_message(HANDERR);
        }
        // check that dimensions supplied are digits.
        if (!isdigit(argv[4][s])) {
            return show_player_message(HANDERR);
        }
        //store each character of the number
        handArg[s] = argv[4][s];
    }
    sscanf(handArg, "%d", &game->handSize);
    free(handArg);
    if (game->handSize < 1) {
        return show_player_message(HANDERR);
    }
    return DONE;
}

/**
 * Function to perform parse player arguments.
 * @param argc - number of arguments
 * @param argv - array of string arguments.
 * @param game struct representing player's tracking of game.
 * @return 1 - less than 4 arguments
 *         2 - threshold less than 2 or not a number
 *         3 - problem reading the deck
 *         4 - Less than P cards in the deck
 */
int parse_player(int argc, char **argv, PlayerGame *game) {
    //     0           1         2        3        4
    // 2310alice playerCount playerID threshold handSize

    // check playerCount
    char *countArg = malloc(sizeof(char) * strlen(argv[1]));
    if (strlen(argv[1]) == 0) {
        return show_player_message(PLAYERERR);
    }
    for (int s = 0; s < strlen(argv[1]); s++) {
        //check if floating point
        if (argv[1][s] == '.') {
            return show_player_message(PLAYERERR);
        }
        // check that dimensions supplied are digits.
        if (!isdigit(argv[1][s])) {
            return show_player_message(PLAYERERR);
        }
        //store each character of the number
        countArg[s] = argv[1][s];
    }
    sscanf(countArg, "%d", &game->playerCount);
    free(countArg);
    if (game->playerCount < 2) {
        return show_player_message(PLAYERERR);
    }

    // check playerID
    char *idArg = malloc(sizeof(char) * strlen(argv[2]));
    if (strlen(argv[2]) == 0) {
        return show_player_message(PLAYERERR);
    }
    for (int s = 0; s < strlen(argv[2]); s++) {
        //check if floating point
        if (argv[2][s] == '.') {
            return show_player_message(POSERR);
        }
        // check that dimensions supplied are digits.
        if (!isdigit(argv[2][s])) {
            return show_player_message(POSERR);
        }
        //store each character of the number
        idArg[s] = argv[2][s];
    }
    sscanf(idArg, "%d", &game->myID);
    free(idArg);
    if (game->myID < 0 || game->myID >= game->playerCount) {
        return show_player_message(POSERR);
    }

    int otherParse = further_arg_checks(argc, argv, game);
    if (otherParse != 0) {
        return otherParse;
    }

    return DONE;

}

void init_expected(PlayerGame *game) {
    game->current = "start";
    game->round = 0;
    game->cardPos = 0;
    game->order = malloc(sizeof(int) * (game->playerCount - 1));
    game->orderPos = 0;
    for (int i = 0; i < game->playerCount; i++) {
        game->order[i] = i;
        //printf(">>%d", game->order[i]);
    }
}

void set_expected(PlayerGame *game, char *set) {
    game->current = set;
}

int check_expected(PlayerGame *game, char *got, int currentPlayer) {
//    printf("state: %s got:%s\n", game->current, got);
    if (strcmp(game->current, "start") == 0) {
        // we expect HAND - we want to start the game
        if (strcmp(got, "HAND") != 0) {
            return show_player_message(MSGERR);
        }
        // expected current to be newround
        set_expected(game, "HAND");

    } else if (strcmp(game->current, "HAND") == 0) {
        if (strcmp(got, "NEWROUND") != 0) {
            return show_player_message(MSGERR);
        }
        set_expected(game, "NEWROUND");

    } else if (strcmp(game->current, "NEWROUND") == 0) {
        //printf("axp: %d playC: %d, player: %d\n", game->expected, game->playerCount, currentPlayer);
        fflush(stdout);
        // start of round fixme check player numbers in order?
        if (strcmp(got, "PLAYED") != 0) {
            return show_player_message(MSGERR);
        }
        if (game->playerCount - 1 == currentPlayer) {
            //if (game->expected == currentPlayer) {
            // all players have moved, go to new round.
            game->expected = 0;
            return DONE;
            //} else {
            // player has been missed
            return show_player_message(MSGERR);
            //}
        }
        // expect current to be another move
        set_expected(game, "PLAYED");
        game->expected++;


    } else if (strcmp(got, "PLAYED") == 0) {
        //printf("bxp: %d playC: %d, player: %d\n", game->expected, game->playerCount, currentPlayer);
        fflush(stdout);

        // mid round
        if (currentPlayer != game->expected) {
            return show_player_message(MSGERR);
        }
        if (strcmp(got, "PLAYED") != 0) {
            return show_player_message(MSGERR);
        }
        if (game->playerCount - 1 == currentPlayer) {
            //if (game->expected == currentPlayer) {
            // all players have moved, go to new round.
            printf("RESET\n");
            fflush(stdout);
            set_expected(game, "HAND");
            game->expected = 0;
            return DONE;
            //} else {
            // player has been missed
            return show_player_message(MSGERR);
            //}
        }
        // expected current to be another move
        game->expected++;
        set_expected(game, "PLAYED");
    }
    return DONE;
}


/* end shared */



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
        return cont_read_stdin(&game);
    } else {
        return show_player_message(ARGERR);
    }
}

