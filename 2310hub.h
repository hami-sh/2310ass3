// enum for exit status
typedef enum {
    OK = 0,
    LESS4ARGS = 1,
    THRESHOLD = 2,
    BADDECKFILE = 3,
    SHORTDECK = 4,
    PLAYERSTART = 5,
    PLAYEREOF = 6,
    PLAYERMSG = 7,
    PLAYERCHOICE = 8,
    GOTSIGHUP = 9
} Status;

// struct for card
typedef struct {
    char rank;
    char suit;
} Card;

// struct for deck
typedef struct {
    unsigned int count;
    unsigned int used;
    Card* contents;
} Deck;

// struct for player hand
typedef struct {
    unsigned int size;
    Card cards[60]; //max number of cards for one player (15 * 4 suits)
} Player;

// struct for particular play of a card
typedef struct {
    unsigned int player;
    Card card;
} Play;

// struct for the game
typedef struct {
    Deck deck;
    Play *board;   //todo fix for number of players
    Player *players;  //todo fix for number of players
    char *types; //todo fix for number of players
    unsigned int current; // will be 0 - playerNumber
    int threshold;
    int playerCount;
} Game;

Status show_message(Status s);

int handler_deck(char* deckName, Game *game);

int load_deck(FILE* input, Deck* deck);

int player_arg_checker(int argc, char** argv, Game *game);

void handle_sighup(int s);

int game_loop(Game *game);