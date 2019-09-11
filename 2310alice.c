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

#define LINESIZE 80

/* shared */
PlayerStatus show_player_message(PlayerStatus s) {
    const char* messages[] = {"",
                              "Usage: player players myid threshold handsize\n",
                              "Invalid players\n",
                              "Invalid position\n",
                              "Invalid theshold\n",
                              "Invalid hand size\n",
                              "Invalid message\n",
                              "EOF\n"};
    fputs(messages[s], stderr);
    return s;
}

int decode_hand(char* input, playerGame *game) {
    printf("hand\n");
    input += 4;

    int inputSize = strlen(input);
    char delim[] = ",";
    char* arrow = strtok(input, delim);

    int i = 0;
    while(arrow != NULL)
    {
        if (i == 0) {
            for (int j = 0; j < strlen(arrow); j++) {
                if (!isdigit(arrow[j])) {
                   return show_player_message(HANDERR);
                }
            }
            sscanf(arrow, "%d", &game->handSize);
        } else if (i > 0) {
            // todo card stuff
        }
        printf("'%s'\n", arrow);
        arrow = strtok(NULL, delim);
        i++;
    }
    //game->handSize = input[0];
    printf("hs: %d", game->handSize);
    return DONE;
}

int decode_newround(char *input) {
    printf("newround\n");
    input += 8;
    return DONE;
}

int decode_played(char *input) {
    printf("played\n");
    input += 6;
    return DONE;
}

int gameover(char* input) {
    return DONE;
}

int decode_play(char *input) {
    printf("play\n");
    input += 4;
    return DONE;
}

int process_input(char* input, playerGame *game) {
    if (regex_hand(input)) {
        decode_hand(input, game);
    } else if (regex_newround(input)) {
        decode_newround(input);
    } else if (regex_played(input)) {
        decode_played(input);
    } else if (regex_play(input)) {
        decode_play(input);
    } else if (regex_gameover(input)) {
        gameover(input);
    }
    return 0;
}

int cont_read_stdin(playerGame *game) {
    char input[LINESIZE];
    fgets(input, BUFSIZ, stdin);
    while (strcmp(input, "GAMEOVER\n") != 0) { //fixme check \n?
        int processed = process_input(input, game);
        if (processed != 0) {
            return processed;
        }
        fgets(input, BUFSIZ, stdin);
    }
    return DONE;
}

int further_arg_checks(int argc, char** argv, playerGame *game) {
    // check threshold
    char* thresholdArg = malloc(sizeof(char) * strlen(argv[3]));
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
    char* handArg = malloc(sizeof(char) * strlen(argv[4]));
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

int parse_player(int argc, char** argv, playerGame *game) {
    //     0           1         2        3        4
    // 2310alice playerCount playerID threshold handSize

    // check playerCount
    char* countArg = malloc(sizeof(char) * strlen(argv[1]));
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
    char* idArg = malloc(sizeof(char) * strlen(argv[2]));
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
/* end shared */



int main(int argc, char** argv) {
    if (argc == 5) {
        fprintf(stdout, "@");
        fflush(stdout);
        playerGame game;
        int parseStatus = parse_player(argc, argv, &game);
        if (parseStatus != 0) {
            return parseStatus;
        }
        return cont_read_stdin(&game);
    } else {
        return show_player_message(ARGERR);
    }
}

