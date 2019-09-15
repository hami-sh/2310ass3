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
#include "parse.h"

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

int numberDigits (int i) {
    if (i == 0) {
        return 1;
    }
    int down = abs(i);
    int log = log10(down);
    int fl = floor(log);
    return fl + 1;
}

int create_players(Game *game, char** argv) {
    game->players = malloc(game->playerCount * sizeof(Player));
    game->numCardsToDeal = floor((game->deck.count / game->playerCount));

    for (int i = 0; i < game->playerCount; i++) {
        printf("%d %s\n", i, argv[i + 3]);
        game->players[i].pipeIn = malloc(sizeof(int) * 2);
        game->players[i].pipeOut = malloc(sizeof(int) * 2);
        pipe(game->players[i].pipeIn);
        pipe(game->players[i].pipeOut);
        if (!fork()) {
            // child
            close(game->players[i].pipeIn[1]); // for child - close write end.
            close(game->players[i].pipeOut[0]); // for child - close read end.
            dup2(game->players[i].pipeIn[0], STDIN_FILENO); //send pipeA stuff to stdin of child.
            dup2(game->players[i].pipeOut[1], STDOUT_FILENO); //send stdout child to write of pipeB

            char* args[6];
            args[0] = argv[i + 3];
            args[1] = malloc((numberDigits(game->playerCount) + 1) * sizeof(int));
            args[2] = malloc((numberDigits(i) + 1) * sizeof(int));
            args[3] = malloc((numberDigits(game->threshold) + 1) * sizeof(int));
            args[4] = malloc((numberDigits(game->numCardsToDeal) + 1) * sizeof(int));
            sprintf(args[1], "%d", game->playerCount);
            sprintf(args[2], "%d", i);
            sprintf(args[3], "%d", game->threshold);
            sprintf(args[4], "%d", game->numCardsToDeal);
            args[5] = 0;
            execv(argv[i + 3], args);
            exit(5);
            return show_message(PLAYERSTART);

        } else {
            // parent
            game->players[i].size = game->numCardsToDeal;
            close(game->players[i].pipeIn[0]);
            close(game->players[i].pipeOut[1]);
        }
    }

    for (int i = 0; i < game->playerCount; i++) {
        game->players[0].fileIn = fdopen(*game->players[0].pipeIn, "w");
        game->players[0].fileOut = fdopen(*game->players[0].pipeOut, "r");
    }

    return OK;
}

int new_game(int argc, char** argv) {
    Game game;
    int parseStatus = parse(argc, argv, &game);
    if (parseStatus != 0) {
        return parseStatus;
    }
    printf("new game\n");

    /*for (int i = 0; i < game.deck.count; i++) {
        printf("%c%c\n", game.deck.contents[i].suit, game.deck.contents[i].rank);
    }*/

    create_players(&game, argv);
    for (int i = 0; i < game.playerCount; i++) {
        printf("%d in:%d out:%d\n", i, game.players[i].pipeIn[1], game.players[i].pipeOut[0]);
    }
    char buf[] = "HELLO WORLD!";
    write(game.players[0].pipeIn[1], buf, sizeof(buf));

    char c;
    c = fgetc(game.players[0].fileOut);
    printf(">%c\n", c);
//    printf("%p\n", game.players[0].fileOut);
    return show_message(game_loop(&game));
}

int game_loop(Game *game) {
    // setup SIGHUP detection
    struct sigaction sa_sighup;
    sa_sighup.sa_handler = handle_sighup;
    sa_sighup.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa_sighup, 0);

    while (true) {
        while (!sighup) {
            struct timespec nap;
            nap.tv_sec = 0;
            nap.tv_nsec = 5000000000;
            nanosleep(&nap, 0);
        }
        //todo kill children
        return GOTSIGHUP;
    }

    return OK;
}

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

int handler_deck(char* deckName, Game *game) {
    FILE* f = fopen(deckName, "r");
    if (!f) {
        return show_message(BADDECKFILE);
    }
    int result = load_deck(f, &game->deck);
    fclose(f);

    // check deck size is sufficient.
    if (game->deck.count < game->playerCount) {
        return show_message(SHORTDECK);
    }
    return result;
}

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
        if (!regex_card(card[0])) {
            return show_message(BADDECKFILE);
        }
        /*switch (card[0]) {
            case 'S':
                break;
            case 'C':
                break;
            case 'D':
                break;
            case 'H':
                break;
            default:
                return show_message(BADDECKFILE);
        }*/
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