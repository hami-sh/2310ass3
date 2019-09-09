#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include "parse.h"

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

int new_game(int argc, char** argv) {
    Game game;
    int parseStatus = parse(argc, argv, &game);
    if (parseStatus != 0) {
        return parseStatus;
    }
    printf("new game\n");
    for (int i = 0; i < game.deck.count; i++) {
        printf("%c%c\n", game.deck.contents[i].suit, game.deck.contents[i].rank);
    }
    return show_message(OK);
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

    int deckStatus = handler_deck(argv[1], game);
    if (deckStatus != 0) {
        return deckStatus;
    }

    return OK;
}

int handler_deck(char* deckName, Game *game) {
    FILE* f = fopen(deckName, "r");
    if (!f) {
        return show_message(BADDECKFILE);
    }
    int result = load_deck(f, &game->deck);
    fclose(f);
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
        switch (card[0]) {
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
        } else if (isalpha(card[1])) {
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
        printf("%d\n", position);
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

int main(int argc, char** argv) {
    // 2310hub deck threshold player0 player 1...
    if (argc >= 5) {
        return new_game(argc, argv);
    } else {
        return show_message(LESS4ARGS);
    }
}