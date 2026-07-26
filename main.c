Project: Typing of the bombs.
Description: a typing game where players destroy falling bombs by typing words.
Student name: Mirana Rejepova
Student id: 61240017 */

// https://www.geeksforgeeks.org/compiler-design/what-is-cross-compiler/ - the link for cross-compilation, I read all information
// using this link. 
#define _CRT_SECURE_NO_WARNINGS // disable security warnings for standard C functions in Visual Studio
#define _DEFAULT_SOURCE //enable POSIX features for cross-platform compatibility

#include <stdio.h> // standard input/output library
#include <stdlib.h> // standard library for memory allocation and random numbers
#include <string.h> // string manipulation functions
#include <ctype.h> // character type functions (ex: isaplha, tolower)
#include <time.h> // time library to seed the random number generator

/* Cross-Platform compatibility: windows and macos support */
//because I used to write the code using macOS however the code should be done for windows,
//so I decided to use cross-platform compatibility;
#ifdef _WIN32
    #include <windows.h> // windows API for sleep and console control
    #include <conio.h> // console I/O for windows (ex: _khbit, _getch)
#else
    #include <unistd.h> // standard symbolic constants and types for UNIX (mac)
    #include <termios.h> // terminal I/O interfaces for non-blocking input
    #include <sys/select.h> // select function to implement _khbit on UNIX

    /* implementation of sleep in milliseconds for no-Windows systems */
    void Sleep(int ms) { usleep(ms * 1000); }

    /* _khbit: checks if a keyboard key has been pressed */
    int _kbhit(void) {
        struct timeval tv = {0, 0};
        fd_set fds; FD_ZERO(&fds); FD_SET(0, &fds);
        select(1, &fds, NULL, NULL, &tv);
        return FD_ISSET(0, &fds);
    }

    /* reads a char without echoing it to the screen */
    int _getch(void) {
        struct termios old, new;
        int ch;
        tcgetattr(0, &old); new = old;
        new.c_lflag &= ~(ICANON | ECHO); // disable buffering and echo
        tcsetattr(0, 0, &new);
        ch = getchar();
        tcsetattr(0, 0, &old); // restore original terminal settings
        return ch;
    }
#endif

/* game constants */
#define SCREEN_W 80          /* width of the game screen */
#define SCREEN_H 25          /* height of the game screen */
#define MAX_WORD 10          /* max chars in a codeword */
#define MAX_WORDS 100        /* max num of words in the dictionary */
#define PLANE_H 3            // plane height
#define PLANE_W 10           // plane width
#define BOMB_H 3             // bomb height
#define BOMB_W 3             // bomb width
#define CITY_H 6             // city height
#define CITY_W 62            // city width
#define CITY_TOP (SCREEN_H - CITY_H) // y-coordinate where the city starts

/* contains all data needed to save/load the game */
struct GameState {
    int plane_x;             /* horizontal (x) position of the plane */
    int bomb_active;         /* is there an active bomb?  */
    int bomb_x, bomb_y;      /* coordinates of the bomb */
    char word[MAX_WORD + 1]; /* codeword */
    int typed;               /* num of chars the user has typed correctly */
    int score;               /* score */
    int correct, mistakes;   /* the score per word (correct/mistake) */
    int bombs_destroyed;     /* total bombs destroyed */
    int tick;                /* frame counter */
};

/* Global arrays for ASCII Art and dictionary */
char plane_art[PLANE_H][PLANE_W + 1];
char bomb_art[BOMB_H][BOMB_W + 1];
char city_art[CITY_H][CITY_W + 1];
char words[MAX_WORDS][MAX_WORD + 1];
int word_count = 0;

/* clear the screen */
void clear_screen(void)
{
#ifdef _WIN32
    system("cls"); // windows command
#else
    system("clear"); // macos command
#endif
}

/* universal function to load ASCII sprites from next files into memory */
int load_art(const char *filename, char *art, int h, int w)
{
    FILE *fp = fopen(filename, "r"); // open file for reading
    if (!fp) return 0; // return error if file not found

    int row_size = w + 1;  /* +1 for '\0' */

    /* initialize the array with spaces */
    int i, j;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) art[i * row_size + j] = ' ';
        art[i * row_size + w] = '\0';
    }

    /* read line from the file */
    char line[200];
    for (i = 0; i < h; i++) {
        if (!fgets(line, 200, fp)) break;

        /* remove \n and \r */
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        /* copy chars into array */
        for (j = 0; j < len && j < w; j++) art[i * row_size + j] = line[j];
    }

    fclose(fp); // close the file
    return 1;
}

/* load word lists from codewords.txt and perform validation  */
int load_words(void) {
    FILE *fp = fopen("codewords.txt", "r");
    if (!fp) return 0;

    char line[200];
    word_count = 0;

    while (word_count < MAX_WORDS && fgets(line, 200, fp)) {
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0 || len > MAX_WORD) continue;

        /* ensure the word contains only alphabetic chars */
        int ok = 1, i;
        for (i = 0; i < len; i++) {
            if (!isalpha(line[i])) { ok = 0; break; }
        }
        if (!ok) continue;

        /* to lowercase */
        for (i = 0; i < len; i++) line[i] = tolower(line[i]);

        strcpy(words[word_count++], line);
    }

    fclose(fp);
    return word_count > 0;
}

/* pick a random word from the dict and it should match the current levels length */
void pick_word(char *out, int length) {
    int matches[MAX_WORDS], count = 0;
    for (int i = 0; i < word_count; i++) {
        if ((int)strlen(words[i]) == length) matches[count++] = i;
    }
    if (count > 0) strcpy(out, words[matches[rand() % count]]);
    else strcpy(out, words[0]);
}

/* render the game frame to the console */
void draw_screen(struct GameState *g) {
    clear_screen(); // refresh screen for the new frame

    /* frame buffer */
    char screen[SCREEN_H][SCREEN_W + 1];
    for (int r = 0; r < SCREEN_H; r++) {
        for (int c = 0; c < SCREEN_W; c++) screen[r][c] = ' ';
        screen[r][SCREEN_W] = '\0';
    }

    /* plane */
    for (int r = 0; r < PLANE_H; r++)
        for (int c = 0; c < PLANE_W; c++) {
            int x = g->plane_x + c;
            if (x >= 0 && x < SCREEN_W && plane_art[r][c] != ' ')
                screen[r][x] = plane_art[r][c];
        }

    /* bomb + word */
    if (g->bomb_active) {
        for (int r = 0; r < BOMB_H; r++)
            for (int c = 0; c < BOMB_W; c++) {
                int x = g->bomb_x + c, y = g->bomb_y + r;
                if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H && bomb_art[r][c] != ' ')
                    screen[y][x] = bomb_art[r][c];
            }

        /* draw the part of the word that use hasn't typed yet */
        int wy = g->bomb_y + 1, wx = g->bomb_x + BOMB_W;
        int i = g->typed, pos = 0;
        while (g->word[i] && wx + pos < SCREEN_W && wy < SCREEN_H) {
            screen[wy][wx + pos++] = g->word[i++];
        }
    }

    /* place the city at the bottom center */
    int cx = (SCREEN_W - CITY_W) / 2;
    for (int r = 0; r < CITY_H; r++)
        for (int c = 0; c < CITY_W; c++) {
            int x = cx + c, y = CITY_TOP + r;
            if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H)
                screen[y][x] = city_art[r][c];
        }

    /* the score in the top right corner */
    char sc[30];
    sprintf(sc, "Score:%d", g->score);
    int sl = strlen(sc);
    for (int c = 0; c < sl; c++) screen[0][SCREEN_W - sl + c] = sc[c];

    /* final output (frame buffer to the terminal) */
    for (int r = 0; r < SCREEN_H; r++) printf("%s\n", screen[r]);
}

/* NEw bomb */
void new_bomb(struct GameState *g) {
    g->bomb_active = 1;
    g->bomb_x = g->plane_x;
    g->bomb_y = PLANE_H;
    g->typed = g->correct = g->mistakes = 0;

    /* length increases by 1 for every 5 bombs destroyed */
    int len = 3 + g->bombs_destroyed / 5;
    if (len > MAX_WORD) len = MAX_WORD;

    pick_word(g->word, len);
}

/*handle keyboard input and compare it with the current codeword */
void type_letter(struct GameState *g, char letter) {
    if (!g->bomb_active)
        return;

    if (letter == g->word[g->typed]) {
        g->typed++; // match found, advance to next letter
        g->correct++;

        /* if the whole word is typed, update score and spawn a new bomb  */
        if (!g->word[g->typed]) {
            g->score += g->correct - g->mistakes;   /* calculate score  */
            g->bombs_destroyed++;
            new_bomb(g);
        }
    } else {
        g->mistakes++;
    }
}

/* update positions of game objects and check for collisions, returns 1 if bomb fell */
int update(struct GameState *g) {
    g->tick++;

    // move plane every 3 frames
    if (g->tick % 3 == 0) {
        g->plane_x++;
        if (g->plane_x >= SCREEN_W - PLANE_W) g->plane_x = 0;
    }

    // move plane every 8 frames
    if (g->bomb_active && g->tick % 8 == 0) {
        g->bomb_y++;
        if (g->bomb_y + BOMB_H > CITY_TOP) return 1;
    }

    if (!g->bomb_active) new_bomb(g);
    return 0; // game continues
}

/* reset the game */
void reset_game(struct GameState *g)
{
    memset(g, 0, sizeof(*g));
}

/* saving to a binary file */
int save_game(const char *filename, struct GameState *g)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) return 0;
    fwrite(g, sizeof(*g), 1, fp);
    fclose(fp);
    return 1;
}

/* load the GameState structure from a binary file */
int load_game(const char *filename, struct GameState *g) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return 0;
    fread(g, sizeof(*g), 1, fp);
    fclose(fp);
    return 1;
}

/* Game Menu */
int show_menu(int in_game) {
    int choice;
    clear_screen();
    printf("\n\n");
    printf("        =======================================\n");
    printf("              TYPING OF THE BOMBS\n");
    printf("        =======================================\n\n");
    printf("           1. New Game\n");
    printf("           2. Load Saved Game\n");
    printf("           3. Save Current Game\n");
    if (in_game) printf("           4. Return to Game\n"); // hidden if game hasn't started
    printf("           5. Exit\n\n");
    printf("           Choose: ");
    scanf("%d", &choice);
    while (getchar() != '\n'); // clear input
    return choice;
}

/* screen GAME OVER */
int game_over_screen(void)
{
    printf("\n\n        ===========================\n");
    printf("              GAME OVER\n");
    printf("        ===========================\n\n");
    printf("        Play again? (y/n): "); // yes -> y and no -> n
    while (1) {
        char a = _getch();
        if (a == 'y' || a == 'Y') return 1;
        if (a == 'n' || a == 'N') return 0;
    }
}

/* the main game loop. returns 1=lost and 0=ESC */
int play_game(struct GameState *g) {
    while (1) {
        draw_screen(g);

        /* if user presses a key */
        while (_kbhit()) {
            int key = _getch();
            if (key == 27) return 0;   /* return menu if ESC is pressed (ESC = menu)  */

            if (key >= 'A' && key <= 'Z') key = key - 'A' + 'a';
            if (key >= 'a' && key <= 'z') type_letter(g, key);
        }

        if (update(g)) return 1;   /* return 1 if Game over*/
        Sleep(60); // control the frame rate
    }
}

/* MAIN */
int main(void) {
    struct GameState game;
    int in_game = 0; // persistent flag to track if a game is currently active

    srand(time(NULL)); // seed random number generator with system time

    /* attempt to load all external resources */
    if (!load_art("plane.txt", &plane_art[0][0], PLANE_H, PLANE_W) ||
        !load_art("bomb.txt", &bomb_art[0][0], BOMB_H, BOMB_W) ||
        !load_art("city.txt", &city_art[0][0], CITY_H, CITY_W) ||
        !load_words()) {
        printf("Error: missing text files such as plane, bomb, city, or codewords!\n");
        return 1;
    }

    reset_game(&game);

    while (1) {
        int choice = show_menu(in_game);

        if (choice == 1 || (choice == 4 && in_game)) {
            /* new game OR return into the game */
            if (choice == 1) {
                reset_game(&game);
                in_game = 1;
            }

            /* playing before failing */
            while (1) {
                int result = play_game(&game);
                if (result == 0) break;   /* ESC */
                /* game over - again?  */
                if (game_over_screen()) {
                    reset_game(&game);
                } else {
                    in_game = 0;
                    break;
                }
            }
        }
        else if (choice == 2) {
            /* download */
            char fn[100];
            printf("\n        Filename: ");
            scanf("%s", fn);
            while (getchar() != '\n');

            if (load_game(fn, &game)) {
                in_game = 1;
                while (1) {
                    int r = play_game(&game);
                    if (r == 0) break;
                    if (game_over_screen()) reset_game(&game);
                    else { in_game = 0; break; }
                }
            } else {
                printf("\n        No such file\n        Press any key...");
                _getch();
            }
        }
        else if (choice == 3 && in_game) {
            /* save */
            char fn[100];
            printf("\n        Filename: ");
            scanf("%s", fn);
            while (getchar() != '\n');

            if (save_game(fn, &game)) printf("        Saved!\n");
            else printf("        Error\n");
            printf("        Press any key...");
            _getch();
        }
        /* shutdown application */
        else if (choice == 5) {
            break;
        }
    }

    return 0;
}