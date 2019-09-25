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

#define LINESIZE 80

/**
 * Function to check if char supplied matches a suit.
 * @param c - character to check
 * @return 0 if doesnt match, 1 if it does.
 */
char validate_card(char c) {
    return c == 'S' ||
            c == 'C' ||
            c == 'D' ||
            c == 'H';
}

/**
 * Function to set the next player expected to move.
 * @param game
 * @param player
 */
void set_player(PlayerGame *game, int player) {
    game->orderPos = player;
}

/**
 * Function to increment to the next player that should move.
 * @param game struct representing player's tracking of game.
 * @return int - next player to move.
 */
int next_player(PlayerGame *game) {
    // if we are at end of players, reset the pos (looping)
    if (game->orderPos == game->playerCount - 1) {
        game->orderPos = 0;
    } else {
        // normal progression otherwise.
        game->orderPos++;
    }
    return game->orderPos;
}

/**
 * Function to remove a card from the hand of a player.
 * @param game struct representing player's tracking of game.
 * @param game struct representing card to remove.
 */
void remove_card(PlayerGame *game, Card *card) {
    int pos = 0;
    // find position of card to remove
    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == card->suit) {
            if (game->hand[i].rank == card->rank) {
                pos = i;
                break;
            }
        }
    }

    for (int q = pos; q < game->handSize - 1; q++) {
        // shift all cards to compensate for removal.
        game->hand[q] = game->hand[q + 1];
    }
    game->handSize -= 1;
}

/**
 * Function to save a card as being played.
 * @param game struct representing player's tracking of game.
 * @param card struct representing card to save
 */
void save_card(PlayerGame *game, Card *card) {
    // store the card as a string for endgame output
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
    // search all cards for matching suit of players hand.
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
    int rank = 17; // higher than highest possible rank.
    Card play;
    play.rank = -1;
    // find matching card
    for (int i = 0; i < game->handSize; i++) {
        if (suit == game->hand[i].suit) {
            if (get_rank_integer(game->hand[i].rank) < rank) {
                // set new lowest
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
        // 1-9
        return arg - '0';
    } else {
        // HEX
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
    // loop over all cards to determine if any match.
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
    char *oldInput = malloc(sizeof(char) * strlen(input));
    strcpy(oldInput, input);
    input[strlen(input) - 1] = '\0'; // remove extra new line char.
    char delim[] = ",";
    char *arrow = strtok(input, delim);

    int i = 0;
    // split string on ','
    while (arrow != NULL) {
        if (i == 0) {
            // parse hand size.
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
            // parse a card
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

    // if card count not equal to hand size, throw
    if (i - 1 != game->handSize) {
        return show_player_message(MSGERR);
    }
    // check for repeating cards
    int repeatStatus = check_repeating_cards(game->hand, game->handSize);
    if (repeatStatus != 0) {
        return repeatStatus;
    }
    // check msg not extra long
    if (strlen(oldInput) != number_digits(game->handSize)
        + 3*game->handSize + 1) {
        return show_player_message(MSGERR);
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
    // set required variables if newround successful.
    game->orderPos = 0;
    game->cardPos = 0;
    game->dPlayedRound = 0;
    input += 8;
    input[strlen(input) - 1] = '\0';
    // get lead player
    char *leadPlayer = malloc(strlen(input) * sizeof(char));
    strncpy(leadPlayer, input, strlen(input));
    int i;
    for (i = 0; i < strlen(leadPlayer); i++) {
        if (!isdigit(leadPlayer[i])) {
            return show_player_message(MSGERR);
        }
    }
    // if there is no player given
    if (i == 0) {
        return show_player_message(MSGERR);
    }
    game->leadPlayer = atoi(leadPlayer);
    // make sure first round means player 0 first.
    if (game->firstRound) {
        game->firstRound = 0;
        if (game->leadPlayer != 0) {
            return show_player_message(MSGERR);
        }
    }
    if (game->leadPlayer >= game->playerCount) {
        return show_player_message(MSGERR);
    }
    // if we are player to move, perform move!
    if (game->myID == game->leadPlayer) {
        game->playerStrategy(game);
        if (game->myID == game->playerCount - 1) {
            game->orderPos = game->myID;
        }
        next_player(game);
    } else {
        // otherwise there is a non 0 player to move first.
        set_player(game, game->leadPlayer);
    }
    // set last player to move (so we know when to expect newround)
    if (game->leadPlayer != 0) {
        game->lastPlayer = game->leadPlayer - 1;
    } else {
        game->lastPlayer = game->playerCount - 1;
    }
    return DONE;
}

/**
 * Function to decide who was the winner of the round.
 * @param game struct representing player's tracking of game.
 */
void decide_round_winner(PlayerGame *game) {
    int rank = 0;
    // loop through all players and determine highest card played
    for (int i = 0; i < game->playerCount; i++) {
        if (game->cardsStored[i][0] == game->leadSuit) {
            if (get_rank_integer(game->cardsStored[i][2]) > rank) {
                game->roundWinner = i;
                rank = get_rank_integer(game->cardsStored[i][0]);
            }
        }
    }
}

/**
 * Function to output end of round msg to stderr at the end of the round.
 * @param game struct representing player's tracking of game.
 */
void player_end_of_round_output(PlayerGame *game) {
    decide_round_winner(game);
    // print out who is lead and all cards played that round
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
 * Function to perform final checks of the played message received.
 * @param game struct representing player's tracking of game.
 * @param input string representing message
 * @param newCard struct representing card played
 * @param justPlayed player number that just played.
 * @return 0 - no errors
 *         6 - message error.
 */
int misc_played_checking(PlayerGame *game, char *input, Card *newCard,
        int justPlayed) {
    // check that card is of proper format
    if (validate_card(input[0]) && (isdigit(input[1]) ||
            (isalpha(input[1]) && isxdigit(input[1]) && islower(input[1])))) {
        newCard->suit = input[0];
        newCard->rank = input[1];
        save_card(game, newCard);
        game->cardPos += 1;
    } else {
        return show_player_message(MSGERR);
    }
    if (strlen(input) != 2) {
        return show_player_message(MSGERR);
    }

    // if suit is D, increase how many D cards were played that round
    if (newCard->suit == 'D') {
        game->dPlayedRound++;
        game->dPlayerNumber[justPlayed]++;
    }
    // set lead suit
    if (justPlayed == game->leadPlayer) {
        game->leadSuit = newCard->suit;
    }
    // output end of round if we are at the end
    if (justPlayed == game->lastPlayer) {
        player_end_of_round_output(game);
        return DONE;
    }
    // if it is our turn to play, make a move!
    if (game->orderPos == game->myID) {
        game->playerStrategy(game);
        next_player(game);
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
    input += 6;
    char *oldInput = malloc(sizeof(char) * strlen(input));
    strcpy(oldInput, input);
    Card newCard;
    input[strlen(input) - 1] = '\0';
    int i = 0;
    // get player ID
    while (input[i] != ',') {
        if (!isdigit(input[i])) {
            return show_player_message(MSGERR);
        }
        i++;
    }
    char *playedID = malloc(i * sizeof(char));
    strncpy(playedID, input, i);
    int justPlayed = atoi(playedID);

    // check msg length.
    if (strlen(oldInput) != number_digits(justPlayed) + 4) {
        return show_player_message(MSGERR);
    }

    // set next msg expected based on player ID
    if (justPlayed == (game->playerCount - 1)) {
        game->orderPos = 0;
        if (game->lastPlayer == justPlayed) {
            set_expected(game, "HAND");
        } else {
            set_expected(game, "PLAYED");
        }
    } else {
        game->orderPos++;
    }
    game->cardsPlayed = malloc(game->handSize * game->playerCount * 2 *
            sizeof(Card));

    // remove prefix from string for easier checking
    input += i;
    input += 1;

    // perform further checks
    int misc = misc_played_checking(game, input, &newCard, justPlayed);
    if (misc != 0) {
        return misc;
    }

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
    // remove PLAYED portion
    dest += 6;
    int i = 0;
    // check that ID is a digit until comma.
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

/**
 * Function to decide what the player will do based on input from hub.
 * @param input - string from stdin.
 * @param game struct representing player's tracking of game.
 * @return 0 - no errors
 *         6 - invalid message from the hub.
 */
int process_input(char *input, PlayerGame *game) {
    char *dest = malloc(sizeof(char) * strlen(input));
    if (strncmp(input, "HAND", 4) == 0) {
        strncpy(dest, input, 4);
        dest[4] = 0;
        // check that hand should be arriving now
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
            return msgCheck;
        }
        int decode = decode_hand(input, game);
        if (decode != 0) {
            return decode;
        }
    } else if (strncmp(input, "NEWROUND", 8) == 0) {
        game->expected = 0;
        strncpy(dest, input, 8);
        dest[8] = 0;
        // check that newround should be arriving now
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
            return msgCheck;
        }
        int decode = decode_newround(input, game);
        if (decode != 0) {
            return decode;
        }
    } else if (strncmp(input, "PLAYED", 6) == 0) {
        game->playerMove = extract_last_player(input);
        strncpy(dest, input, 6);
        dest[6] = 0;
        // check that played should be arriving now
        int msgCheck = check_expected(game, dest, game->playerMove);
        if (msgCheck != 0) {
            return msgCheck;
        }
        int decode = decode_played(input, game);
        if (decode != 0) {
            return decode;
        }
    } else if (strncmp(input, "GAMEOVER", 8) == 0) {
        // clean return (exit with 0)
        return 0;
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
    // keep reading until gameover or EOF from hub
    while (strcmp(input, "GAMEOVER\n") != 0 && !feof(stdin)) {
        // decide what to do on message
        int processed = process_input(input, game);
        if (processed != 0) {
            return processed;
        }
        // get next message
        fgets(input, BUFSIZ, stdin);
    }
    if (feof(stdin)) {
        // return EOF if end of file.
        return show_player_message(EOFERR);
    }
    return DONE;
}

/**
 * Function to send message to hub as to  what card was played.
 * @param game struct representing player's tracking of game.
 * @return 0 when done.
 */
void play_card(PlayerGame *game, Card *play) {
    // record card as being played
    save_card(game, play);
    if (play->suit == 'D') {
        game->dPlayedRound += 1;
    }
    game->cardPos += 1;
    // output
    printf("PLAY%c%c\n", play->suit, play->rank);
    fflush(stdout);
    // remove from hand, so we cant play again.
    remove_card(game, play);
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

/**
 * Function to set the next expected msg from hub.
 * @param game struct representing player's tracking of game.
 * @param set string representing state.
 */
void set_expected(PlayerGame *game, char *set) {
    game->current = set;
}

/**
 * Function to check the msg received from the hub to ensure correct ordering.
 * @param game struct representing player's tracking of game.
 * @param got string that the hub sent.
 * @param currentPlayer integer representing the current player playing.
 * @return 0 - no error
 *         6 - invalid message from hub.
 */
int check_expected(PlayerGame *game, char *got, int currentPlayer) {
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

/**
 * Function to set up state handling along with initialisation of variables.
 * @param game struct representing player's tracking of game.
 */
void init_expected(PlayerGame *game) {
    // setup variables used for the game
    game->current = "start";
    game->round = 0;
    game->firstRound = 1;
    game->cardPos = 0;

    // malloc various storages
    game->cardsStored = (char **) malloc(game->playerCount * sizeof(char *));
    for (int j = 0; j < game->playerCount; j++) {
        game->cardsStored[j] = malloc(4 * sizeof(char));
    }
    game->order = malloc(sizeof(int) * (game->playerCount - 1));
    game->orderPos = 0;
    game->dPlayedRound = 0;
    game->dPlayerNumber = malloc(sizeof(int) * game->playerCount);
    for (int i = 0; i < game->playerCount; i++) {
        game->dPlayerNumber[i] = 0;
    }

    // store order for players
    int i;
    for (i = 0; i < game->playerCount; i++) {
        game->order[i] = i;
    }
    game->largestPlayer = i;
}