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
#include <math.h>

//todo validate change.
#define LINESIZE 80


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


void set_player(PlayerGame *game, int player) {
    game->orderPos = player;
}

int next_player(PlayerGame *game) {
    if (game->orderPos == game->playerCount - 1) {
//        fprintf(stderr, "(%d) %d : %d", game->myID, game->orderPos, game->playerCount - 1);
//        fprintf(stderr, "reset\n");
        game->orderPos = 0;
    } else {
//        fprintf(stderr, "(%d) inc\n", game->myID);
        game->orderPos++;
    }
    return game->orderPos;
}

int peek_next_player(PlayerGame *game) {
    if (game->orderPos == game->playerCount - 1) {
        return 0;
    } else {
        return game->orderPos + 1;
    }
}

/* SHARED PLAYER SECTION */
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

void save_card(PlayerGame *game, Card *card) {
    game->cardsStored[game->cardPos][0] = card->suit;
    game->cardsStored[game->cardPos][1] = '.';
    game->cardsStored[game->cardPos][2] = card->rank;
    game->cardsStored[game->cardPos][3] = '\0';
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
                return show_player_message(MSGERR);
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
    game->cardPos = 0;
    game->dPlayedRound = 0;
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

    if (game->firstRound) {
        game->firstRound = 0;
        if (game->leadPlayer != 0) {
            return show_player_message(MSGERR);
        }
    }
    if (game->leadPlayer >= game->playerCount) {
        return show_player_message(MSGERR);
    }
    // make move!
    if (game->myID == game->leadPlayer) {
//        fprintf(stderr, "NEWROUND: %d MOVED\n", game->myID);
        game->player_strategy(game);
        if (game->myID == game->playerCount - 1) {
            game->orderPos = game->myID;
        }
        next_player(game);
    } else {
        set_player(game, game->leadPlayer);
    }

    if (game->leadPlayer != 0) {
        game->lastPlayer = game->leadPlayer - 1;
    } else {
        game->lastPlayer = game->playerCount - 1;
    }

//    fprintf(stderr, "NEWROUND next player:: %d\n", game->orderPos);
    return DONE;
}

void decide_round_winner(PlayerGame *game) {
    int rank = 0;
    for (int i = 0; i < game->playerCount; i++) {
        if (game->cardsStored[i][0] == game->leadSuit) {
            //printf("\n %d %d %d", i, get_rank_integer(game->cardsStored[i][2]), rank);
            if (get_rank_integer(game->cardsStored[i][2]) > rank) {
                game->roundWinner = i;
                rank = get_rank_integer(game->cardsStored[i][0]);
            }
        }
    }
}

void player_end_of_round_output(PlayerGame *game) {
    decide_round_winner(game);
    fprintf(stderr, "Lead player=%d:", game->leadPlayer);
    for (int i = 0; i < game->cardPos; i++) {
        fprintf(stderr, " %s", game->cardsStored[i]);
    }
    fprintf(stderr, "\n");
    game->dPlayedRound = 0;

}

/**
 * Function to return how many digits there are in the supplied number.
 * @param i - the number to get the digits of
 * @return the number of digits.
 */
int number_digits(int i) {
    if (i == 0) {
        return 1;
    }
    int down = abs(i);
    int log = log10(down);
    int fl = floor(log);
    return fl + 1;
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

//    if (justPlayed != game->order[game->orderPos]) {
//        fprintf(stderr, "%d broke %dvs%d\n", game->myID, justPlayed, game->orderPos);
//        return show_player_message(MSGERR);
    if (justPlayed == (game->playerCount - 1)) {
//        fprintf(stderr, "(%d) %d just played, reset to 0\n", game->myID, justPlayed);
        game->orderPos = 0;
        if (game->lastPlayer == justPlayed) {
            set_expected(game, "HAND");
        } else {
            set_expected(game, "PLAYED");
        }
    } else {
//        fprintf(stderr, "(%d) inc\n", game->myID);
        game->orderPos++;
    }
    //todo is this an issue
    game->cardsPlayed = malloc(game->handSize * game->playerCount * 2 *
                               sizeof(Card));

    input += i;
    input += 1;

    if (validate_card(input[0]) && (isdigit(input[1]) ||
                                    (isalpha(input[1]) && isxdigit(input[1]) && islower(input[1])))) {
        newCard.suit = input[0];
        newCard.rank = input[1];
        save_card(game, &newCard);
//        printf("hi3\n");

        game->cardPos += 1;
    } else {
        return show_player_message(MSGERR);
    }
    if (strlen(input) != 2) {
        return show_player_message(MSGERR);
    }

    if (justPlayed == game->leadPlayer) {
        game->leadSuit = newCard.suit;
    }

    if (justPlayed == game->lastPlayer) {
        player_end_of_round_output(game);
        return DONE;
    }

//    fprintf(stderr, "%d\n", peek_next_player(game));
    if (game->orderPos == game->myID) {
        game->player_strategy(game);
        next_player(game);
    }
    if (newCard.suit == 'D') {
        game->dPlayedRound++;
        game->dPlayerNumber[justPlayed]++;
    }
    return DONE;
}

/**
 * Function to handle gameover message from stdin.
 * @param input - string representing message
 * @return //fixme
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
    char *dest = malloc(sizeof(char) * strlen(input));
    if (validate_hand(input)) {
        strncpy(dest, input, 4);
        dest[4] = 0;
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
            return msgCheck;
        }
        int decode = decode_hand(input, game);
        if (decode != 0) {
            return decode;
        }
    } else if (validate_newround(input)) {
        game->expected = 0;
        strncpy(dest, input, 8);
        dest[8] = 0;
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
            return msgCheck;
        }
        int decode = decode_newround(input, game);
        if (decode != 0) {
            return decode;
        }
    } else if (validate_played(input)) {
        //todo get current from string
        game->playerMove = extract_last_player(input);
        strncpy(dest, input, 6);
        dest[6] = 0;
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
//            fprintf(stderr, "BRUH\n");
            return msgCheck;
        }
        int decode = decode_played(input, game);
        if (decode != 0) {
            return decode;
        }
    } else if (validate_gameover(input)) {
        return gameover(input);
    } else {
        return show_player_message(MSGERR);
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
        //printf("in %s\n", input);
        if (validate_play(input)) {
            game->player_strategy(game);
            game->orderPos += 1;
            fgets(input, BUFSIZ, stdin);
            continue;
        }
//        fprintf(stderr, "(%d) READ: %s", game->myID, input);
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


void set_expected(PlayerGame *game, char *set) {
    game->current = set;
}

int check_expected(PlayerGame *game, char *got, int currentPlayer) {
//    fprintf(stderr, "state: %s got:%s\n", game->current, got);
    if (strcmp(game->current, "start") == 0) {
        /* BEGINNING STATE */
        // we expect HAND - we want to start the game
        if (strcmp(got, "HAND") != 0) {
            return show_player_message(MSGERR);
        }
        // expected next state to be newround
        set_expected(game, "HAND");

    } else if (strcmp(game->current, "HAND") == 0) {
        /* HAND STATE */
        // we expect the next to be newround to start the game
        if (strcmp(got, "NEWROUND") != 0) {
            return show_player_message(MSGERR);
        }
        set_expected(game, "NEWROUND");

    } else if (strcmp(game->current, "NEWROUND") == 0) {
        /* NEWROUND STATE */
        //we expect to get a played message
        if (strcmp(got, "PLAYED") != 0) {
            return show_player_message(MSGERR);
        }
        // expect next state to be another move
        set_expected(game, "PLAYED");

    } else if (strcmp(got, "PLAYED") == 0) {
        /* PLAYED STATE */
        if (strcmp(got, "PLAYED") != 0) {
            return show_player_message(MSGERR);
        }
        if (game->playerCount - 1 == currentPlayer) {
            if (game->lastPlayer == currentPlayer) {
                set_expected(game, "HAND");
            } else {
                set_expected(game, "PLAYED");
            }
            return DONE;

        }
        // expected current to be another move
        set_expected(game, "PLAYED");
    }
    return DONE;
}

void malloc_card_storage(PlayerGame *game) {
    for (int j = 0; j < game->playerCount; j++) {
        game->cardsStored[j] = malloc(4 * sizeof(char));
    }
}

void init_expected(PlayerGame *game) {
    game->current = "start";
    game->round = 0;
    game->firstRound = 1;
    game->cardPos = 0;
    game->cardsStored = (char **) malloc(game->playerCount * sizeof(char*));
    malloc_card_storage(game);
    game->order = malloc(sizeof(int) * (game->playerCount - 1));
    game->orderPos = 0;
    game->dPlayedRound = 0;
    game->dPlayerNumber = malloc(sizeof(int) * game->playerCount);
    for (int i = 0; i < game->playerCount; i++) {
        game->dPlayerNumber[i] = 0;
    }
    int i;
    for (i = 0; i < game->playerCount; i++) {
        game->order[i] = i;
        //printf(">>%d", game->order[i]);
    }
    game->largestPlayer = i;
}

/* end shared */

