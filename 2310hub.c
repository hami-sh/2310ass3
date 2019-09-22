#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "2310hub.h"
#include <limits.h>

// global variable for SIGHUP signal.
bool sighup = false;

/**
 * Function to output an error message and return status.
 * @param s status to return with.
 * @return error status.
 */
Status show_message(Status s) {
    const char* messages[] = {"",
             "Usage: 2310hub deck threshold player0 {player1}\n",
             "Invalid threshold\n",
             "Deck error\n",
             "Not enough cards\n",
             "Player error\n",
             "Player EOF\n",
             "Invalid message\n",
             "Invalid card choice\n",
             "Ended due to signal\n"};
    fputs(messages[s], stderr);
    return s;
}


/**
 * Function to perform forking of players
 * @param game struct representing player's tracking of game.
 * @param argv arguments from command line.
 * @return 0 - no errors
 *         5 - error starting the players
 */
int create_players(Game *game, char** argv) {
    game->pidChildren = malloc(game->playerCount * sizeof(int));
    game->players = malloc(game->playerCount * sizeof(Player));
    game->numCardsToDeal = floor((game->deck.count / game->playerCount));

    for (int i = 0; i < game->playerCount; i++) {
        game->players[i].pipeIn = malloc(sizeof(int) * 2);
        game->players[i].pipeOut = malloc(sizeof(int) * 2);
        pipe(game->players[i].pipeIn);
        pipe(game->players[i].pipeOut);
//        printf(">>IN%d in:%d out:%d\n", i, game->players[i].pipeIn[0], game->players[i].pipeIn[1]);
//        printf(">>OUT%d in:%d out:%d\n", i, game->players[i].pipeOut[0], game->players[i].pipeOut[1]);
        pid_t pid;
        if ((pid = fork()) < 0) {
            return show_message(PLAYERSTART);
        } else if (pid == 0) {
            // child
            close(game->players[i].pipeIn[1]); // for child - close write end.
            close(game->players[i].pipeOut[0]); // for child - close read end.
            dup2(game->players[i].pipeIn[0], STDIN_FILENO); //send pipeA stuff to stdin of child.
            dup2(game->players[i].pipeOut[1], STDOUT_FILENO); //send stdout child to write of pipeB

            //todo DEBUG REPLACE
            int dev = open("/dev/null", O_WRONLY);
            dup2(dev, 2); // supress stderr of child


            char* args[6];
            args[0] = argv[i + 3];
            args[1] = malloc((number_digits(game->playerCount) + 1) * sizeof(int));
            args[2] = malloc((number_digits(i) + 1) * sizeof(int));
            args[3] = malloc((number_digits(game->threshold) + 1) * sizeof(int));
            args[4] = malloc((number_digits(game->numCardsToDeal) + 1) * sizeof(int));
            sprintf(args[1], "%d", game->playerCount);
            sprintf(args[2], "%d", i);
            sprintf(args[3], "%d", game->threshold);
            sprintf(args[4], "%d", game->numCardsToDeal);
            //printf("%d\n", game->numCardsToDeal);
            args[5] = 0;
            execv(argv[i + 3], args);
            return show_message(PLAYERSTART);

        } else {
            // parent
            game->pidChildren[i] = pid;
            game->players[i].size = game->numCardsToDeal;
            close(game->players[i].pipeIn[0]);
            close(game->players[i].pipeOut[1]);
        }
    }

    for (int i = 0; i < game->playerCount; i++) {
        game->players[i].fileIn = fdopen(game->players[i].pipeIn[1], "w");
        game->players[i].fileOut = fdopen(game->players[i].pipeOut[0], "r");
    }


    return OK;
}

/**
 * Function to go through each player and read to check if we get "@"
 * @param game struct representing player's tracking of game.
 * @return 0 if ok
 *         5 if player cannot be found.
 */
int check_players(Game *game) {
    for (int i = 0; i < game->playerCount; i++) {
        char c = fgetc(game->players[i].fileOut);
//        printf("%d %c\n", i, c);
        if (c != '@') {
            return show_message(PLAYERSTART);
        }
    }
    return show_message(OK);
}

/**
 * Function to handle the creation of a game.
 * @param argc - number of command line args
 * @param argv - arguments supplied on command line.
 * @return 0 - normal exit after game
 *         3 - error parsing deck
 *         4 - less than P cards in deck.
 *         5 - error starting player
 *         6 - EOF from player
 *         7 - invalid player message
 *         8 - invalid card choice from player
 *         9 - received SIGHUP signal.
 */
int new_game(int argc, char** argv) {
    Game game;
    game.firstRound = 1;
    game.leadPlayer = 0;
    int parseStatus = parse(argc, argv, &game);
    if (parseStatus != 0) {
        return parseStatus;
    }

    int createStatus = create_players(&game, argv);
    if (createStatus != 0) {
        return createStatus;
    }

    int playerStatus = check_players(&game);
    if (playerStatus != 0) {
        return playerStatus;
    }

    init_state(&game);
    return show_message(game_loop(&game));
}

/**
 * Function to handle the dealing of cards and sending of card message to
 * players
 * @param game struct representing player's tracking of game.
 * @param id - ID of the player to send the cards to.
 * @return 0 when done.
 */
int deal_card_to_player(Game *game, int id) {
    Card cardsForPlayer[game->numCardsToDeal];
    int j;
    for (j = 0; j < game->numCardsToDeal; j++) {
        cardsForPlayer[j] = game->deck.contents[j];
//        printf("%c%c-", cardsForPlayer[j].suit, cardsForPlayer[j].rank);
    }
    for (j = 0; j < game->numCardsToDeal; j++) {
        remove_deck_card(game, &cardsForPlayer[j]);
    }

    //HANDx,C1,C2,C3...,Cn
    int cardNo = game->numCardsToDeal;
    char* hand = malloc((4 + number_digits(cardNo) + cardNo + (cardNo * 2) + 2)
            * sizeof(char));
    strcpy(hand, "HAND");
    char insertNumber[cardNo];
    sprintf(insertNumber, "%d", cardNo);
    int i;
    for (i = 4; i < (number_digits(cardNo) + 4); i++) {
        hand[i] = insertNumber[i - 4];
    }
    int pos = 0;
    for (i = i; i < (cardNo + (cardNo * 2) + number_digits(cardNo) + 3); i++ ) {
        if ((i - (4 + number_digits(cardNo))) % 3 == 0) {
            // comma
            hand[i] = ',';
        } else {
            hand[i] = cardsForPlayer[pos].suit;
            hand[++i] = cardsForPlayer[pos].rank;
            pos++;
        }
    }
    hand[i] = '\n';
    hand[i + 1] = '\0';
    fprintf(game->players[id].fileIn, hand);
    fflush(game->players[id].fileIn);
    return DONE;
}

void newround_msg(Game *game) {
    if (game->firstRound) {
        for (int i = 0; i < game->playerCount; i++) {
            fprintf(game->players[i].fileIn, "NEWROUND%d\n", game->leadPlayer);
            fflush(game->players[i].fileIn);
        }
    }
    if (game->leadPlayer != 0) {
        game->lastPlayer = game->leadPlayer - 1;
    } else {
        game->lastPlayer = game->playerCount - 1;
    }
    next_state(game);
}

void calculate_scores(Game *game) {
    int rank = 0;
    int winner = -1;
    int dCardCount = 0;
    for (int i = 0; i < game->playerCount; i++) {
        Card playedCard = game->cardsByRound[game->roundNumber][i];
        if (playedCard.suit == 'D') {
            dCardCount++;
        }
        if (game->leadSuit == playedCard.suit) {
            if (get_rank_integer(playedCard.rank) >= rank) {
                rank = get_rank_integer(playedCard.rank);
                winner = i;
            }
        }
//        printf("%c%c ", playedCard.suit, playedCard.rank);
    }
    game->leadPlayer = winner;
    game->nScore[winner] += 1;
    game->dScore[winner] += dCardCount;
//    printf("----(%d %d %d)----\n", winner, game->nScore[winner], game->dScore[winner]);


}


int send_and_receive(Game *game) {
    int playerMove = game->leadPlayer;
    bool go = true;
    int numberPlays = 0;
    while (go) {
        // get current player move
//        printf("HUB READING: (%d)\n", playerMove);
        const short buffSize = (short)log10(INT_MAX) + 3; // +2 to allow for \n\0
        char buffer[buffSize];
        if (!fgets(buffer, buffSize - 1, game->players[playerMove].fileOut)
            || feof(game->players[playerMove].fileOut)
            || ferror(game->players[playerMove].fileOut)) {
            //fixme do something here
            printf("bruh\n");
        }
//        printf("%d: %s", playerMove, buffer);
        //todo validate move

        if (playerMove == game->leadPlayer) {
            game->leadSuit = buffer[4];
        }

        Card playedCard;
        playedCard.suit = buffer[4];
        playedCard.rank = buffer[5];
        game->cardsByRound[game->roundNumber][playerMove] = playedCard;
        game->cardsByRoundOrderPlayed[game->roundNumber][numberPlays] = playedCard;


        // send move to other players
        char* playedMsg = malloc(7 + number_digits(playerMove) + 2);
        sprintf(playedMsg, "%s%d,%c%c\n", "PLAYED", playerMove, buffer[4], buffer[5]);
        for (int i = 0; i < game->playerCount; i++) {
            if (i != playerMove) {
                fprintf(game->players[i].fileIn, playedMsg);
                fflush(game->players[i].fileIn);
            }
        }
        playerMove += 1; // move to next player
        numberPlays += 1;
//        printf("%dvs%d %d\n", game->lastPlayer, playerMove, numberPlays);
        if (numberPlays == game->playerCount) {
            go = false;
            break;
        } else if (numberPlays != game->playerCount) {
            if (playerMove == game->playerCount) {
                playerMove = 0;
            }
        }
    }
    return OK;
}

void end_round_output(Game *game) {
//    printf("ENDROUND\n");
    printf("Lead player=%d\n", game->leadPlayer);
    printf("Cards=");
    for (int i = 0; i < game->playerCount; i++) {
        Card playedCard = game->cardsByRoundOrderPlayed[game->roundNumber][i];
        if (i < game->playerCount - 1) {
            printf("%c.%c ", playedCard.suit, playedCard.rank);
        } else {
            printf("%c.%c", playedCard.suit, playedCard.rank);
        }
    }
    printf("\n");
    calculate_scores(game);
}

void end_game_output(Game *game) {
    for (int i = 0; i < game->playerCount; i++) {
        if (game->dScore[i] >= game->threshold) {
            game->finalScores[i] = game->nScore[i] + game->dScore[i];
        } else {
            game->finalScores[i] = game->nScore[i] - game->dScore[i];
        }
    }
//    printf("ENDGAME\n");
    for (int i = 0; i < game->playerCount; i++) {
        if (i != game->playerCount - 1) {
            printf("%d:%d ", i, game->finalScores[i]);
        } else {
            printf("%d:%d", i, game->finalScores[i]);
        }
    }
    printf("\n");
}

/**
 * //fixme check return status'
 * Function to handle the overarching game loop.
 * @param game struct representing player's tracking of game.
 * @return 0 - normal exit
 *         9 - received SIGHUP
 */
int game_loop(Game *game) {
    // setup SIGHUP detection
    struct sigaction sa_sighup;
    sa_sighup.sa_handler = handle_sighup;
    sa_sighup.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa_sighup, 0);

//    while (true) {
    while (!sighup) {
        struct timespec nap;
        nap.tv_sec = 0;
        nap.tv_nsec = 5000000000;
        nanosleep(&nap, 0);

        if (get_state(game) == START) {
            for (int i = 0; i < game->playerCount; i++) {
                deal_card_to_player(game, i);
            }
            next_state(game);
            next_state(game); //fixme bruh
            continue;
        } else if (get_state(game) == NEWROUND) {
            //todo continue & break below into separate section.
            newround_msg(game);
        } else if (get_state(game) == PLAYING) {
            int response = send_and_receive(game);
            if (response != 0) {
                return response;
            }
            next_state(game);
            continue;
        } else if (get_state(game) == ENDROUND) {
            end_round_output(game);
            game->roundNumber++;
            next_state(game);
            continue;
        } else if (get_state(game) == ENDGAME) {
            end_game_output(game);
            return OK;
        }
    }
    // kill children
    for (int i = 0; i < game->playerCount; i++) {
        fclose(game->players[i].fileOut);
        fclose(game->players[i].fileIn);
        close(game->players[i].pipeIn[1]);
        close(game->players[i].pipeOut[0]);
        kill(game->pidChildren[i], SIGKILL); //kill children
        wait(NULL); //reap zombies
    }
    return GOTSIGHUP;
//    }

    return OK;
}

/**
 * Function to handle the parsing of command line arguments
 * @param argc - number of arguments supplied
 * @param argv - arguments supplied at command line.
 * @param game struct representing player's tracking of game.
 * @return 0 - all ok
 *         2 - error in threshold count
 *         3 - deck error
 *         4 - less than P cards in deck.
 */
int parse(int argc, char** argv, Game *game) {
    //   0      1       2       3        4
    //2310hub deck threshold player0 {player1}

    char* thresholdArg = malloc(sizeof(char) * strlen(argv[2]));
    for (int s = 0; s < strlen(argv[2]); s++) {
        //check if floating point
        if (argv[2][s] == '.') {
            return show_message(THRESHOLD);
        }
        // check that dimensions supplied are digits.
        if (!isdigit(argv[2][s])) {
            return show_message(THRESHOLD);
        }
        //store each character of the number
        thresholdArg[s] = argv[2][s];
    }
    sscanf(thresholdArg, "%d", &game->threshold);
    free(thresholdArg);
    if (game->threshold < 2) {
        return show_message(THRESHOLD);
    }

    game->playerCount = argc - 3;
    int deckStatus = handler_deck(argv[1], game);
    if (deckStatus != 0) {
        return deckStatus;
    }

    /*int playerStatus = player_arg_checker(argc, argv, game);
    if (playerStatus != 0) {
        return playerStatus;
    }*/

    return OK;
}

/*int player_arg_checker(int argc, char** argv, Game *game) {
    game->types = (char *) malloc((argc - 3) * sizeof(char));
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "./2310alice") == 0 ) {
            game->types[i-3] = 'a';
        } else if (strcmp(argv[i], "./2310bob") == 0) {
            game->types[i-3] = 'b';
        } else {
            return show_message(PLAYERSTART);
        }
    }
    game->types[game->playerCount] = '\0';
    return OK;
}*/

/**
 * Function to handle the reading deck contents.
 * @param deckName - name of deck from command line
 * @param game struct representing player's tracking of game.
 * @return 3 - error parsing the deck
 *         4 - less than P cards in deck.
 */
int handler_deck(char* deckName, Game *game) {
    FILE* f = fopen(deckName, "r");
    if (!f) {
        return show_message(BADDECKFILE);
    }
    int result = load_deck(f, &game->deck);
    if (result != 0) {
        return result;
    }
    fclose(f);

    // check deck size is sufficient.
    if (game->deck.count < game->playerCount) {
        return show_message(SHORTDECK);
    }
    return result;
}

/**
 * Function to check format of card read in from deck
 * @param card - string representing card.
 * @return 0 - no errors
 *         3 - error in card.
 */
int check_string(char *card) {
    //if over 2 char
    if (strlen(card) > 2) {
        return show_message(BADDECKFILE);
    }
    //ensure letter first
    if (isalpha(card[0]) == 0) {
        return show_message(BADDECKFILE);
    } else {
        //ensure letter is S, C, D, H
        if (!validate_card(card[0])) {
            return show_message(BADDECKFILE);
        }
    }

    //ensure letter is captial
    if (islower(card[0])) {
        return show_message(BADDECKFILE);
    }

    //ensure digit or hex last & is lowercase
    if (isdigit(card[1]) == 0) {
        if (isxdigit(card[1] == 0)) {
            return show_message(BADDECKFILE);
        } else if (isalpha(card[1]) && isxdigit(card[1])) {
            if (isupper(card[1])) {
                return show_message(BADDECKFILE);
            } else {
                return OK;
            }
        }
        return show_message(BADDECKFILE);
    }

    return OK;
}

/**
 * Function to handle running of deck loading.
 * @param input - deck file to read from.
 * @param deck - deck to read in to.
 * @return 0 - no errors
 *         3 - error in deck file formatting
 */
int load_deck(FILE* input, Deck* deck) {
    int position = 0;
    while (1) {
        char *card = malloc(sizeof(char) * 4);
        // read each line of the deck file
        if (fscanf(input, "%s", card) != 1) {
            break;
        }
        if (position == 0) {
            for (int i = 0; i < strlen(card); i++) {
                // if any position of the first line is not a number, break
                if (isdigit(card[i]) == 0) {
                    return show_message(BADDECKFILE);
                }
                // if we are over 60 (max card number - no duplicates)
                if (atoi(card) > 60){
                    return show_message(BADDECKFILE);
                }
                deck->count = atoi(card);
                deck->contents = malloc(deck->count * sizeof(card));
            }
        }
        // check if card format is ok
        if (position >= 1) {
            int check = check_string(card);
            if (check != 0) {
                return check;
            }
            // create card
            Card newCard;
            newCard.suit = card[0];
            newCard.rank = card[1];
            deck->contents[position-1] = newCard;
        }
        position++;
    }
    if (feof(input)) {
        // incorrect number of cards.deckA
        if (deck->count != position - 1) {
            return show_message(BADDECKFILE);
        }
        return OK;
    } else {
        return show_message(BADDECKFILE);
    }
}

void init_state(Game *game) {
    game->state = "start";
    game->cardsByRound = (Card **) malloc(sizeof(Card *) * game->numCardsToDeal);
    game->cardsByRoundOrderPlayed = (Card **) malloc(sizeof(Card *) * game->numCardsToDeal);
    for (int i = 0; i < game->numCardsToDeal; i++) {
        game->cardsByRoundOrderPlayed[i] = (Card *)malloc(sizeof(Card) * game->playerCount);
        game->cardsByRound[i] = (Card *)malloc(sizeof(Card) * game->playerCount);
    }

    game->nScore = (int *) malloc(sizeof(int) * game->playerCount);
    game->dScore = (int *) malloc(sizeof(int) * game->playerCount);
    game->finalScores = (int *) malloc(sizeof(int) * game->playerCount);
    for (int i = 0; i < game->playerCount; i++) {
        game->nScore[i] = 0;
        game->dScore[i] = 0;
        game->finalScores[i] = 0;
    }
}

void set_state(Game *game, char *state) {
    game->state = state;
}

int get_state(Game *game) {
    if (strcmp(game->state, "start") == 0) {
        return START;
    } else if (strcmp(game->state, "HAND") == 0) {
        return HAND;
    } else if (strcmp(game->state, "NEWROUND") == 0) {
        return NEWROUND;
    } else if (strcmp(game->state, "PLAYING") == 0) {
        return PLAYING;
    } else if (strcmp(game->state, "ENDROUND") == 0) {
        return ENDROUND;
    } else if (strcmp(game->state, "ENDGAME") == 0) {
        return ENDGAME;
    }
    return -1;
}

int next_state(Game *game) {
    if (strcmp(game->state, "start") == 0) {
        // start of game
        game->state = "HAND";
        return HAND;
    } else if (strcmp(game->state, "HAND") == 0) {
        // given out hands
        game->state = "NEWROUND";
        return NEWROUND;
    } else if (strcmp(game->state, "NEWROUND") == 0) {
        // read player 0, send out PLAYED0
        // read player 1, send out PLAYED1 ...
        game->state = "PLAYING";
        return PLAYING;
    } else if (strcmp(game->state, "PLAYING") == 0) {
        // end round stuff
        game->state = "ENDROUND";
        return ENDROUND;
    } else if (strcmp(game->state, "ENDROUND") == 0) {
        if (game->roundNumber == game->numCardsToDeal) {
            game->state = "ENDGAME";
            return ENDGAME;
        } else {
            game->state = "NEWROUND";
            return NEWROUND;
        }
    }
    return DONE;
}

void remove_deck_card(Game *game, Card *card) {
    int pos = 0;
    for (int i = 0; i < game->deck.count; i++) {
        if (game->deck.contents[i].suit == card->suit) {
            if (game->deck.contents[i].rank == card->rank) {
                pos = i;
                break;
            }
        }
    }
    for (int q = pos; q < game->deck.count - 1; q++) {
        // shift all cards to compensate for removal.
        game->deck.contents[q] = game->deck.contents[q + 1];
    }
    game->deck.count -= 1;
    game->deck.used += 1;
}

void handle_sighup(int s) {
    sighup = true;
}

int main(int argc, char** argv) {
    // 2310hub deck threshold player0 player 1...
    if (argc >= 5) {
        return new_game(argc, argv);
    } else {
        return show_message(LESS4ARGS);
    }
}