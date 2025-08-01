// ███████████████████████████████████████████████
// █▄─▄─▀█─▄▄─█▄─▄▄▀█▄─▄▄─█▄─▄▄▀█▄▄▄ █▄─▄▄▀█─▄▄▄▄█
// ██─▄─▀█─██─██─▄─▄██─▄█▀██─██─██▄▄ ██─██─█▄▄▄▄─█
// █▄▄▄▄██▄▄▄▄█▄▄█▄▄█▄▄▄▄▄█▄▄▄▄██▄▄▄▄█▄▄▄▄██▄▄▄▄▄█

// █▀▄▀█ ▄▀█ █ █▄ █   █▀▀
// █ ▀ █ █▀█ █ █ ▀█ ▄ █▄▄

// Programmed by ZcraftElite

//   ╔════════════════════════════════════════════════╗
// ══╣                    INCLUDES                    ╠══
//   ╚════════════════════════════════════════════════╝
#include <3ds.h>
#include <stdio.h>
#include <string.h>


//   ╔════════════════════════════════════════════════╗
// ══╣              VERSION INFORMATION               ╠══
//   ╚════════════════════════════════════════════════╝
const char *versionString = "v0.0.1a"; /** @brief Semantic version of the program @note Format: v{major}.{minor}.{patch}{alpha/beta} */
const char *versionDate = "2025-08-01"; /** @brief Date of the last version of the program @note Format: YYYY-MM-DD */
const int versionRev = 12; /** @brief Revision of the program, used for tracking changes in the codebase */
char versionText[32]; /** @brief Full semantic version/revision version text for display purposes */


//   ╔════════════════════════════════════════════════╗
// ══╣        IMPORTANT IDENTIFIER DEFINITIONS        ╠══
//   ╚════════════════════════════════════════════════╝

// Initialize the objects for the screens
PrintConsole topScreen; /** @brief Console object for the top screen */
PrintConsole bottomScreen; /** @brief Console object for the bottom screen */


//   ╔════════════════════════════════════════════════╗
// ══╣           IMPORTANT GLOBAL VARIABLES           ╠══
//   ╚════════════════════════════════════════════════╝

// Key Detection
u32 kDown; /** @brief List of keys being held down (this frame) */
u32 kPress; /** @brief List of keys that were pressed (this frame) */

// Menu Navigation
int menuSelection; /** @brief Index of the current menu selection */
int menuPrevSelection; /** @brief Index of the previous menu selection */
int menuMaxSelection; /** @brief Max value of the menu selection index */
bool menuSelectionChosen; /** @brief Whether the menu selection has been chosen */
bool menuSelectionShown; /** @brief Whether the menu selection has been printed */
bool menuOptionsVisible; /** @brief Whether the menu options are visible */

// Phase Tracking
int phase; /** @brief Current phase of the application @note Phases are what I am using to track the part of the game the user is in. */
int nextPhase; /** @brief Next phase of the application @note Phases are what I am using to track the part of the game the user is in. */
bool loadingScreenActive; /** @brief Whether the loading screen is active */
bool loadingScreenShown; /** @brief Whether the loading screen has been printed */

//   ╔════════════════════════════════════════════════╗
// ══╣                   FUNCTIONS                    ╠══
//   ╚════════════════════════════════════════════════╝

/**
 * @fn void clearScreen(char *screen)
 * @since rev2 (v0.0.1a)
 * @brief Clears the specified console screen.
 * @param screen The console screen to clear ("top", "bottom" or "both").
 */
void clearScreen(char *screen)
{
    if (strcmp(screen, "top") == 0) {
        consoleSelect(&topScreen);
        printf("\x1b[2J\x1b[H");
    } else if (strcmp(screen, "bottom") == 0) {
        consoleSelect(&bottomScreen);
        printf("\x1b[2J\x1b[H");
    } else if (strcmp(screen, "both") == 0) {
        consoleSelect(&topScreen);
        printf("\x1b[2J\x1b[H");
        consoleSelect(&bottomScreen);
        printf("\x1b[2J\x1b[H");
    } else {
        return; // Invalid screen specified
    }
}

/**
 * @fn void printCenter(char *text, int row)
 * @since rev3 (v0.0.1a)
 * @brief Prints autocentered text based on the specified screen width and row.
 * @param text The text to be printed.
 * @param row The row of the screen on which to print the text.
 * @param width The width of the screen to center the text.
 * @note The 3DS top screen is 50 characters wide, and the bottom screen is 32 characters wide.
 */
void printCenter(char *text, int row, int width)
{
    int len = strlen(text);
    int col = (width - len) / 2;
    if (col < 0) col = 0; // Makes sure the column doesn't end up negative
    printf("\x1b[%d;%dH%s", row, col + 1, text);
}

/**
 * @fn void printBanner(char *text, int row, int width)
 * @since rev4 (v0.0.1a)
 * @brief Prints centered text with a "banner" style, being padded on both sides with "="
 * @param text The text to be printed.
 * @param row The row of the screen on which to print the text.
 * @param width The width of the screen to center the text.
 * @note The 3DS top screen is 50 characters wide, and the bottom screen is 32 characters wide.
 */
void printBanner(char *text, int row, int width)
{
    int len = strlen(text);
    int totalPadding = width - len - 2;
    if (totalPadding < 0) totalPadding = 0; // Makes sure the padding doesn't end up negative

    int leftPadding = totalPadding / 2;
    int rightPadding = totalPadding - leftPadding;

    printf("\x1b[%d;1H", row); // Moves the cursor to the start of the specified row

    for (int i = 0; i < leftPadding; i++) printf("="); // Left padding
    printf(" %s ", text); // Text
    for (int i = 0; i < rightPadding; i++) printf("="); // Right padding

}


/**
 * @fn void menuOption(char *text, int option)
 * @since rev4 (v0.0.1a)
 * @brief Used to print an option on the screen in a menu/selection format.
 * @param optionLabel The label of the option to be printed.
 * @param num The selection number of the option to be printed.
 */
void menuPrintOption(char *optionLabel, int num)
{
    consoleSelect(&topScreen);
    printf("\x1b[%d;1H+------------------------------------------------+", ((4 * num) - 3) + 4);
    printf("\x1b[%d;1H|                                                |", ((4 * num) - 2) + 4);
    printf("\x1b[%d;3H%s", (((4 * num) - 2) + 4), optionLabel);
    printf("\x1b[%d;1H+------------------------------------------------+", ((4 * num) - 1) + 4);
}

/**
 * @fn void menuNavigation()
 * @since rev8 (v0.0.1a)
 * @brief Handles the navigation of the menu, allowing the user to select options.
 */
void menuNavigation()
{
    if (kPress & KEY_A) menuSelectionChosen = true; // If A is pressed, the selection is chosen
    if (kPress & KEY_B && menuSelectionChosen == true) { // If B is pressed and a selection is chosen, return to the main menu
        clearScreen("both");
        menuSelectionChosen = false;
        menuSelectionShown = false;
    };
    
    menuPrevSelection = menuSelection; // Store the previous selection

    if ((kPress & KEY_UP)) menuSelection = (menuSelection > 1) ? menuSelection - 1 :  menuMaxSelection; // If UP is pressed, the selection is moved up
    if ((kPress & KEY_DOWN)) menuSelection = (menuSelection <  menuMaxSelection) ? menuSelection + 1 : 1; // If DOWN is pressed, the selection is moved down

    consoleSelect(&topScreen);
    printf("\x1b[%d;47H  ", ((4 *  menuPrevSelection) - 2) + 4); // Remove the highlight from the previous selection
    printf("\x1b[%d;47H<-", ((4 * menuSelection) - 2) + 4); // Highlight the current selection
}


//   ╔════════════════════════════════════════════════╗
// ══╣             FUNCTION POINTER SETUP             ╠══
//   ╚════════════════════════════════════════════════╝
/**
 * @fn typedef void (*functionPointer)();
 * @since rev2 (v0.0.1a)
 * @brief Serves as a function pointer for use in other functions.
 */
typedef void (*functionPointer)();


//   ╔════════════════════════════════════════════════╗
// ══╣                 MAIN FUNCTION                  ╠══
//   ╚════════════════════════════════════════════════╝
int main(int argc, char **argv)
{
    // Set the version text
    sprintf(versionText, "%s (rev%d)", versionString, versionRev);

    // Start da graphix engines
    gfxInitDefault();

    // Start the romfs filesystem
    romfsInit();

    // Use the PrintConsoles for the graphics
    consoleInit(GFX_TOP, &topScreen);
    consoleInit(GFX_BOTTOM, &bottomScreen);

    // Initialize the selection variables
    menuSelection = 1;
    menuMaxSelection = 4;
    menuPrevSelection = 0;
    menuSelectionChosen = false;
    menuSelectionShown = false;
    menuOptionsVisible = true;

    // Initialize the part variables
    phase = 1;
    nextPhase = 1;
    loadingScreenActive = false;
    loadingScreenShown = false;

    // Print options
    void mainMenu()
    {
        consoleSelect(&topScreen);
        printBanner("Bored3DS: Main Menu", 1, 50);
        menuPrintOption("Start Game", 1);
        menuPrintOption("How to Play", 2);
        menuPrintOption("Credits", 3);
        menuPrintOption("Exit", 4);
        printBanner(versionText, 30, 50);
    }

    mainMenu();
    
    while (aptMainLoop())
    {
        hidScanInput();

        kDown = hidKeysHeld();
        kPress = hidKeysDown();

        // ----------------- Loading Screen -----------------
        if (loadingScreenActive)
        {
            if (!loadingScreenShown)
            {
                clearScreen("both");
                consoleSelect(&topScreen);
                printBanner("Bored3DS", 1, 50);
                printCenter("Loading...", 15, 50);
                printBanner("Use START to exit if frozen", 30, 50);
                loadingScreenShown = true;
            }

            // Variables
            phase = nextPhase;
            menuSelection = 1;
            menuSelectionChosen = false;
            menuSelectionShown = false;

            if (kPress & KEY_START) {
                break; // If Start is pressed during a loading screen, exit the application
            }
        }

        // -------------------- Phase 1 ---------------------
        // ------------------- Main Menu --------------------
        if (phase == 1) {
            if (!menuSelectionChosen)
            {
                if (!menuOptionsVisible)
                {
                    clearScreen("both");
                    mainMenu();
                    menuOptionsVisible = true;
                };

                menuNavigation();
            };

            if (menuSelectionChosen && !menuSelectionShown)
            {
                clearScreen("top");
                menuOptionsVisible = false;

                consoleSelect(&topScreen);
                printBanner("Press B to return to main menu", 30, 50);
                if (menuSelection == 1)
                {
                    nextPhase = 2;
                    loadingScreenActive = true;
                }
                else if (menuSelection == 2)
                {
                    consoleSelect(&topScreen);
                    printBanner("HOW TO PLAY", 1, 50);
                    printCenter("Use the D-Pad to navigate the menu", 3, 50);
                    printCenter("Press A to select an option.", 4, 50);
                    printCenter("Have fun and enjoy the game!", 6, 50);

                    consoleSelect(&bottomScreen);
                    printCenter("You'll get the hang of it!", 15, 40);
                }
                else if (menuSelection == 3)
                {
                    consoleSelect(&topScreen);
                    printBanner("CREDITS", 1, 50);
                    printCenter("Bored3DS", 3, 50);
                    printCenter("A 3DS homebrew game made of boredom", 4, 50);
                    printCenter(versionText, 6, 50);
                    printCenter("Developed & Designed by ZcraftElite", 8, 50);
                    printCenter("(C) 2025 Z-NET", 10, 50);
                }
                else if (menuSelection == 4)
                {
                    clearScreen("top");
                    consoleSelect(&topScreen);
                    printBanner("EXITING GAME", 30, 50);
                    break;
                }

                menuSelectionShown = true;
            };
        };

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    // Clean up
    gfxExit();

    return 0;
}