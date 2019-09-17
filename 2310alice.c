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

int decode_hand(char* input, PlayerGame *game) {
    input += 4;
    input[strlen(input) - 1] = '\0'; // remove extra new line char.
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
                    game->hand[i-1] = card;
            } else {
                return show_player_message(MSGERR);
            }

        }
        arrow = strtok(NULL, delim);
        i++;
    }

    if (i-1 != game->handSize) {
        return show_player_message(MSGERR);
    }

    int repeatStatus = check_repeating_cards(game->hand, game->handSize);
    if (repeatStatus != 0) {
        return repeatStatus;
    }
    return DONE;
}

int decode_newround(char *input, PlayerGame *game) {
    printf("newround\n");
    input += 8;
    input[strlen(input) - 1] = '\0';
    // get lead player
    char* leadStr = malloc(strlen(input) * sizeof(char));
    strncpy(leadStr, input, strlen(input));
    for (int i = 0; i < strlen(leadStr); i++) {
        if (!isdigit(leadStr[i])) {
            return show_player_message(MSGERR);
        }
    }
    game->leadPlayer = atoi(leadStr);
    return DONE;
}

int decode_played(char *input, PlayerGame *game) {
    //PLAYEDid,CARD
    printf("played\n");
    input += 6;
    input[strlen(input) - 1] = '\0';
    int i = 0;
    while (input[i] != ',') {
        if (!isdigit(input[i])) {
            return show_player_message(MSGERR);
        }
        i++;
    }
    char* playedID = malloc(i * sizeof(char));
    int justPlayed = atoi(playedID);

    if (justPlayed != game->order[game->orderPos]) {
        printf("-<<%d>>--<<%d>>-\n", justPlayed, game->order[game->orderPos]);
        return show_player_message(MSGERR);
    } else if (justPlayed == (game->playerCount - 1)) {
        game->orderPos = 0;
    } else if (justPlayed == (game->order[game->playerCount - 1])) {
        game->orderPos = 0;
    } else {
        game->orderPos++;
    }
    //todo is this an issue
    game->cardsPlayed = malloc(game->handSize * game->playerCount *
            sizeof(Card));

    input += i;
    printf("--(%s)--\n", input);
    return DONE;
}

int gameover(char* input) {
    return DONE;
}

int extract_last_player(char *input) {
    char* dest = malloc(sizeof(char) * strlen(input));
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

int process_input(char* input, PlayerGame *game) {
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

int cont_read_stdin(PlayerGame *game) {
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

int further_arg_checks(int argc, char** argv, PlayerGame *game) {
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

int parse_player(int argc, char** argv, PlayerGame *game) {
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

void init_expected(PlayerGame *game) {
    game->current = "start";
    game->round = 0;
    game->order = malloc(sizeof(int) * (game->playerCount - 1));
    game->orderPos = 0;
    for (int i = 0; i < game->playerCount; i++) {
        if (game->myID == i) {
            game->order[game->orderPos] = (++i);
        } else {
            game->order[game->orderPos] = i;
        }
        game->orderPos++;
    }
    game->orderPos = 0;
}

void set_expected(PlayerGame *game, char* set) {
    game->current = set;
}

int check_expected(PlayerGame *game, char* got, int currentPlayer) {
    printf("before: %s got:%s\n", game->current, got);
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
        printf("exp: %d playC: %d, player: %d\n", game->expected, game->playerCount, currentPlayer);
        fflush(stdout);
        // start of round fixme check player numbers in order?
        if (game->expected != 0) {
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
                set_expected(game, "HAND"); //todo check this and below
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
        printf("exp: %d playC: %d, player: %d\n", game->expected, game->playerCount, currentPlayer);
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



int main(int argc, char** argv) {
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

