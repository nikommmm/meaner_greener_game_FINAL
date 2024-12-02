#include "FEHLCD.h"
#include "FEHRandom.h"
#include "FEHImages.h"
#include "FEHUtility.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


////////////////////////
/* GLOBAL DEFINITIONS */
////////////////////////

/* CLASS: Represents an activity and its CO2 emissions (i.e., a "prompt" in the context of a higher-or-lower game).
    Author: Niko
    Members:
        activityDescription - A description of the activity (e.g., "Generating 1 kg of coffee").
        emissionValue - The activity's LCA (life cycle analysis) or direct emissions value in kg CO2eq.
        note - Additional information or fun fact about the activity (optional). 
    Constructors:
        Emission() - Default constructor.
        Emission(const char* activity, double value) - Initializes with activity and value based on parsed data. */
class Emission {
public:
    char activityDescription[128];
    double emissionValue;
    char activityNote[512];
    Emission();
    Emission(const char* activity, double value, const char* note);
};

#define DATA_SIZE 99    // Maximum expected data entries (predetermined and able to be updated as more data is included in .csv)
#define NUM_GIFS 11     // Maximum expected GIFs (also predetermined)
const int NUM_FRAMES[NUM_GIFS] = {25, 9, 79, 55, 98, 91, 50, 65, 97, 92, 99};   // Total frames (really, last frame file ##) for each GIF so that they can be displayed accordingly
Emission emissions[DATA_SIZE];  // Globally define array of data so that it can be referenced in functions and wherever

using namespace std;


/////////////////////////
/* FUNCTION PROTOTYPES */
/////////////////////////

int loadEmissionsFromFile(Emission emissions[], int size);

void getDistinctInts(int max, int* index1, int* index2);
void getDistinctIntForNextRound(int max, int currentIndex, int* newIndex);

void drawButtonWithText(int x1, int y1, int x2, int y2, unsigned int rectColor, const char* textLabel, unsigned int textColor);
int buttonPress(int x1, int y1, int x2, int y2);
void drawBackButton();

void titleScreen();
void instructionsScreen();
void creditsScreen();
void creditsCreditsScreen();
void referencesScreen();
void leaderboardScreen();

void printTextWithinBox(const char* note, unsigned int textColor, int x1, int y1, int x2, int y2);
void displayActivityLeft(int index);
void displayActivityRight(int index);

void losingScreen(int score);

void displayVersus();
void correct_animation();
void incorrect_animation();

void drawNoteButtons();
void slidePrompts(int index1, int index2, int newIndex);
void scrollingValue(int index);

void displayBriefing();
void playGame();

void mainMenu();




///////////////////
/* MAIN FUNCTION */
///////////////////

int main()
{
    // Load data
    int count = loadEmissionsFromFile(emissions, DATA_SIZE);
    
    // Enter game, starting on title screen!
    titleScreen();

    return 0;
}




//////////////////////////
/* FUNCTION DEFINITIONS */
//////////////////////////

/* Default constructor for Emission class.
   Initializes every parameter as empty or some default value. */
Emission::Emission() {
    strcpy(activityDescription, "");
    emissionValue = 0.0;
    strcpy(activityNote, "");
}
/* Parameterized constructor for Emission class.
   Initializes activity description, emission value, note, and text color based on data file. */
Emission::Emission(const char* activity, double value, const char* note) {
    strcpy(activityDescription, activity);
    emissionValue = value;
    strcpy(activityNote, note);
}

/* FUNCTION: Reads emissions data from file emissions_data.csv into an array of Emission objects.
    Author: Niko
    Arguments:
        emissions - Array to store the loaded data.
        size - Maximum number of entries the array can hold.
    Returns:
        count - Number of activity/emissions pair entries loaded or 1 if the file can't be opened.   */
int loadEmissionsFromFile(Emission emissions[], int size) {
    FILE* file = fopen("emissions_data.csv", "r");  // Open the CSV (realistically @SV) file
    if (!file) {
        printf("Error: Unable to open the data file.\n");
        return 1;
        LCD.WriteLine("Error: Unable to open the data file.");
        while (1) {
            LCD.Update();
        }
    }

    char activity[128];
    double value;
    char note[512];
    int count = 0;

    while (fscanf(file, "%127[^@]@%lf@%511[^\n]\n", activity, &value, note) == 3 && count < size) {
        emissions[count] = Emission(activity, value, note);
        count++;
    }

    fclose(file);
    printf("Successfully loaded %i data entries.", count);
    return count;
}

/* FUNCTION: Generates TWO distinct random integers between 0 and max (inclusive), ensuring the two integers are not equal.
    Author: Niko
    Arguments:
        max - The maximum value for the random integers, inclusive.
        index1 - A reference to an integer that will store the first distinct random index.
        index2 - A reference to an integer that will store the second distinct random index, ensuring it's different from index1.
    Returns:
        NONE                                                                                                                            */
void getDistinctInts(int max, int* index1, int* index2) {
    *index1 = Random.RandInt() % (max);
    do {
        *index2 = Random.RandInt() % (max);
    } while (*index2 == *index1);
}

/* FUNCTION: Generates ONE random integer between 0 and max (inclusive), ensuring it is different from the previously selected index.
    Author: Niko
    Arguments:
        max - The maximum value for the random integer, exclusive. The function will generate a random integer in the range [0, max].
        currentIndex - The index of the "winning" value (the current value to compare against).
        newIndex - A reference to an integer that will store the new distinct random index, ensuring it's different from currentIndex.
    Returns:
        NONE                                                                                                                            */
void getDistinctIntForNextRound(int max, int currentIndex, int* newIndex) {
    int attempts = 0;
    const int MAX_ATTEMPTS = 10;  // Max attempts to generate a distinct value randomly

    do {
        *newIndex = Random.RandInt() % max;
        attempts++;
    } while (*newIndex == currentIndex && attempts < MAX_ATTEMPTS);

    // If after multiple attempts we still get currentIndex, adjust manually
    if (*newIndex == currentIndex) {
        *newIndex = (currentIndex + 1) % max;
    }
}

/* FUNCTION: Displays an activity and its emissions value on the LEFT half of the screen.
    Author: Niko
    Arguments:
        index - Index of the activity to display.
    Returns:
        NONE                                                                              */
void displayActivityLeft(int index) {
    char filename[20];
    sprintf(filename, "emissions_images\\%d.png", index);

    FEHImage leftImage(filename);
    leftImage.Draw(0,0);

    LCD.SetFontColor(WHITE);
    unsigned int textColor = WHITE;

    int lineHeight = 17;
    int currentY = 4;

    printTextWithinBox(emissions[index].activityDescription, textColor, 8, 24, 145, 200);

    // Display value below description
    char valueText[20];
    if (emissions[index].emissionValue == (int)emissions[index].emissionValue) {
        sprintf(valueText, "%d", (int)emissions[index].emissionValue);
    } else if (emissions[index].emissionValue == (int)(emissions[index].emissionValue * 10) / 10.0) {
        sprintf(valueText, "%.1f", emissions[index].emissionValue);
    } else {
        sprintf(valueText, "%.2f", emissions[index].emissionValue);
    }

    printTextWithinBox(valueText, textColor, 4, 200, 156, 216);
    printTextWithinBox("kg CO2eq", textColor, 4, 220, 156, 236);

    LCD.Update();
}

/* FUNCTION: Displays an activity and its emissions value on the RIGHT half of the screen.
    Author: Niko
    Arguments:
        index - Index of the activity to display.
    Returns:
        NONE                                                                              */
void displayActivityRight(int index) {
    char filename[20];
    sprintf(filename, "emissions_images\\%d.png", index);

    FEHImage rightImage(filename);
    rightImage.Draw(160,0);

    unsigned int textColor = WHITE;
    LCD.SetFontColor(textColor);

    int lineHeight = 17;
    int currentY = 4;

    printTextWithinBox(emissions[index].activityDescription, textColor, 175, 24, 312, 200);

    FEHImage buttons("images\\meaner_greener_buttons.png");
    buttons.Draw(0,0);
    displayVersus(); 
    FEHImage note_buttons("images\\note_buttons.png");
    note_buttons.Draw(0,0);

    LCD.Update();
}

/* FUNCTION: Draws a button with specified coordinates, colors, and text label.
    Author: Niko
    Arguments:
        x1, y1 - Coordinates for the top-left corner of the button.
        x2, y2 - Coordinates for the bottom-right corner of the button.
        rectColor - Color of the button's rectangle.
        textLabel - Text to be displayed in the button.
        textColor - Color of the text label.
    Returns:
        NONE                                                                    */
void drawButtonWithText(int x1, int y1, int x2, int y2, unsigned int rectColor, const char* textLabel, unsigned int textColor) {
    int buttonWidth = x2 - x1;
    int buttonHeight = y2 - y1;
    
    LCD.SetFontColor(rectColor);
    LCD.DrawRectangle(x1, y1, buttonWidth, buttonHeight);

    LCD.SetFontColor(textColor);
    
    int midX = (x1 + x2) / 2;
    int midY = (y1 + y2) / 2;
    int textWidth = strlen(textLabel) * 12; // Each character is 12 px wide
    int textX = midX - textWidth / 2;
    int textY = midY - 8; 
    
    LCD.WriteAt(textLabel, textX, textY);
    
    LCD.Update();
}

/* FUNCTION: Detects if a button is pressed based on its coordinates.
    Author: Reagan
    Arguments:
        x1, y1 - Coordinates for the top-left corner of the button.
        x2, y2 - Coordinates for the bottom-right corner of the button.
    Returns:
        1 if the button is pressed, 0 otherwise.                       */
int buttonPress(int x1, int y1, int x2, int y2) {
    float x_pos, y_pos;
    float x_trash, y_trash;

    if (LCD.Touch(&x_pos, &y_pos)) {
        if (x_pos >= x1 && x_pos <= x2 && y_pos >= y1 && y_pos <= y2) {
            while (LCD.Touch(&x_trash, &y_trash)) {};

            return 1;
        }
    }

    return 0;
}

/* FUNCTION: Draws standard back button in the bottom right of screen.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                                          */ 
void drawBackButton() {
    drawButtonWithText(252, 209, 319, 239, WHITE, "Back", WHITE);
}

/* FUNCTION: Displays the title screen with a flashing continue arrow.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                                          */
void titleScreen() {
    FEHImage titleScreen("images\\title_screen.png");
    
    // "Continue" arrow flashing functionality
    bool isFlashing = false;
    float lastFlashTime = TimeNow();

    while (1) {
        float currentTime = TimeNow();
        if (currentTime - lastFlashTime > 0.5) {
            isFlashing = !isFlashing;
            lastFlashTime = currentTime;
        }

        LCD.Clear(BLACK);
        titleScreen.Draw(0, 0);

        if (isFlashing) {
            LCD.SetFontColor(BLACK);
            LCD.FillRectangle(273, 206, 33, 21);
        }
        if (buttonPress(273, 206, 306, 227)) {
            mainMenu();
            return;
        }

        LCD.Update();
        Sleep(10);
    }
}

/* FUNCTION: Displays the instructions screen.
    Author: Reagan
    Arguments:
        NONE
    Returns:
        NONE                                  */
void instructionsScreen() {
    FEHImage instructions("images\\instructions.png");
    instructions.Draw(0,0);
    drawBackButton();
    LCD.Update();
    while (1) {
        if (buttonPress(252, 209, 319, 239)) {
            mainMenu();
            return;
        }
    }
}

/* FUNCTION: Displays the credits menu and allows for navigation to references or game credits.
    Author: Reagan
    Arguments:
        NONE
    Returns:
        NONE                                                                                   */
void creditsScreen() {
    LCD.Clear(BLACK);

    drawButtonWithText(35, 60, 285, 102, WHITE, "Credits", WHITE);
    drawButtonWithText(35, 108, 285, 150, WHITE, "Abbr. References", WHITE);
    drawBackButton();
    LCD.Update();
    
    while (1) {
        if (buttonPress(252, 209, 319, 239)) {  // Back to menu
            mainMenu();
            return;
        }
        if (buttonPress(15, 108, 305, 150)) {   // To references
            referencesScreen();
            return;
        }
        if (buttonPress(35, 60, 285, 102)) {    // To credits
            creditsCreditsScreen();
            return;
        }
    }
}

/* FUNCTION: Displays premade "credits" image that illustrates the game's logo and each of our names. 
    Author: Reagan
    Arguments:
        NONE
    Returns:
        NONE                                                                                        */
void creditsCreditsScreen() {
    FEHImage a("images\\credits.png");
    a.Draw(0,0);
    drawBackButton();
    LCD.Update();

    while (1) {
        if (buttonPress(252, 209, 319, 239)) {
            creditsScreen();
            return;
        }
    }
}

/* FUNCTION: Displays premade "abbreviated references" image.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                                 */
void referencesScreen() {
    LCD.Clear(BLACK);
    FEHImage references("images\\references.png");
    references.Draw(0,0);

    drawBackButton();
    while (1) {
        if (buttonPress(252, 209, 319, 239)) {
            creditsScreen();
            return;
        }
    }
}

/* FUNCTION: Displays the leaderboard with the top 5 scores from previous games (reads in .txt file with every 
             single losing score, so high scores are DEVICE SPECIFIC).
    Author: Reagan
    Arguments:
        NONE
    Returns:
        NONE                                                                                                  */
void leaderboardScreen() {
    LCD.Clear(BLACK);
    
    FILE *scoresFile = fopen("losing_scores.txt", "r");
    if (!scoresFile) {
        printf("Error: Unable to open losing_scores.txt\n");
    }

    int scoresData[1000];
    int index = 0;

    while (fscanf(scoresFile, "%i", &scoresData[index]) != EOF && index < 1000) {
        index++;
    }
    fclose(scoresFile);

    //  Iterate to find top 5 scores ever
    int topScores[5] = {0, 0, 0, 0, 0};
    for (int i=0; i<5; i++) {
        int maxIndex = -1;
        for (int j=0; j<index; j++) {
            if (scoresData[j] > topScores[i]) {
                topScores[i] = scoresData[j];
                maxIndex = j;
            }
        }
        if (maxIndex != -1) {
            scoresData[maxIndex] = -1;  // Set currently found top score to -1 so that second highest can be found, then third, then fourth...
        }
    }

    // Display leaderboard title and top 5 scores on LCD
    char scoresStr[5][20];
    for (int i=0; i<5; i++) {
        sprintf(scoresStr[i], "%i", topScores[i]);  // Format top scores as strings for compatibility with printTextWithinBox()
    }
    printTextWithinBox("DEVICE TOP 5 SCORES:", WHITE, 0, 0, 320, 30);
    LCD.SetFontColor(WHITE);
    LCD.DrawLine(35, 32, 285, 32);
    printTextWithinBox(scoresStr[0], WHITE, 0, 40, 319, 70);
    printTextWithinBox(scoresStr[1], WHITE, 0, 80, 319, 110);
    printTextWithinBox(scoresStr[2], WHITE, 0, 120, 319, 150);
    printTextWithinBox(scoresStr[3], WHITE, 0, 160, 319, 190);
    printTextWithinBox(scoresStr[4], WHITE, 0, 200, 319, 230);

    drawBackButton();
    LCD.Update();
    while (1) {
        if (buttonPress(252, 209, 319, 239)) {
            mainMenu();
            return;
        }
    }
}

/* FUNCTION: Displays a any text string both horizontally and vertically centered within a defined "text box" on the LCD.
    Author: Niko
    Arguments:
        note - The note text to display.
        textColor - The color of the text.
        x1, y1 - Coordinates for the top-left corner of the box.
        x2, y2 - Coordinates for the bottom-right corner of the box.
    Returns:    
        NONE                                                                                                             */                                   
void printTextWithinBox(const char* note, unsigned int textColor, int x1, int y1, int x2, int y2) {
    int noteLength = strlen(note);
    const int lineLength = (x2 - x1) / 12; // Each char is 12px wide, determine how many characters fit per line of text w/in dimensions
    const int lineHeight = 17;

    int totalLines = (noteLength + lineLength - 1) / lineLength; // Calculate total lines needed, rounding up
    int textHeight = totalLines * lineHeight;

    int initialY = y1 + ((y2 - y1) - textHeight) / 2;

    int lineStartChar = 0;

    while (lineStartChar < noteLength) {
        int lineEndChar = lineStartChar + lineLength;
        if (lineEndChar < noteLength) {
            // Avoid splitting words in the middle
            while (lineEndChar > lineStartChar && note[lineEndChar] != ' ' && note[lineEndChar] != '\0') {
                lineEndChar--;
            }
        }

        // If no space is found, use the full line length
        if (lineEndChar == lineStartChar) {
            lineEndChar = lineStartChar + lineLength;
        }

        char line[lineEndChar - lineStartChar + 1];
        strncpy(line, note + lineStartChar, lineEndChar - lineStartChar);
        line[lineEndChar - lineStartChar] = '\0';

        // Calculate the X position to center the text horizontally within the box
        int textWidth = strlen(line) * 12;
        int textX = x1 + (x2 - x1 - textWidth) / 2;

        LCD.SetFontColor(textColor);
        LCD.WriteAt(line, textX, initialY);

        // Move the current line start "cursor" down by the line height
        initialY += lineHeight;
        lineStartChar = lineEndChar + 1;
    }
}

/* FUNCTION: Displays a losing screen with a GIF and final score.
    Author: Niko
    Arguments:
        score - The players score after which they lost.
    Returns:
        NONE                                                     */
void losingScreen(int score) {
    // Randomly select a GIF
    int randomGifIndex = Random.RandInt() % NUM_GIFS + 1;

    char folderPath[30];
    sprintf(folderPath, "GIFs\\%d\\", randomGifIndex);

    int frameIndex = 0;
    const unsigned int frameDelayMs = 40;  // Each frame lasts for 40 ms (25 FPS)
    unsigned int lastFrameTimeMs = TimeNow() * 1000;
    char scoreText[20];
    sprintf(scoreText, "Score: %d", score);

    // Infinite loop to play GIF
    while (true) {
        unsigned int currentTimeMs = TimeNow() * 1000;

        if ((currentTimeMs - lastFrameTimeMs) >= frameDelayMs) {
            char filename[100];
            sprintf(filename, "%sframe_%02d_delay-0.04s.png", folderPath, frameIndex);
            
            FEHImage frameImage(filename);
            frameImage.Draw(0, 0);

            frameIndex++;
            if (frameIndex >= NUM_FRAMES[randomGifIndex - 1]) {
                frameIndex = 0;
            }

            lastFrameTimeMs = currentTimeMs;

            drawBackButton();
            printTextWithinBox("You lost!", WHITE, 0, 70, 319, 120);
            printTextWithinBox(scoreText, WHITE, 0, 100, 319, 150);
            
            LCD.Update();
        }

        // Check for the back button press continuously
        if (buttonPress(252, 209, 319, 239)) {
            mainMenu();
            return;
        }

        Sleep(1);
    }
}

/* FUNCTION: Displays premade versus sign image between the two activities.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                                               */
void displayVersus() {
    FEHImage versus("correct_animation\\0.png");
    versus.Draw(0,0);
}

/* FUNCTION: Plays the animation for a CORRECT answer as a sequence of premade frames.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                                                          */
void correct_animation() {
    int frameIndex = 1;
    const unsigned int frameDelayMs = 10;  // Each frame lasts for 10 ms
    unsigned int lastFrameTimeMs = TimeNow() * 1000;

    while (frameIndex <= 31) {
        unsigned int currentTimeMs = TimeNow() * 1000;

        // Only update the frame when the appropriate time has passed
        if ((currentTimeMs - lastFrameTimeMs) >= frameDelayMs) {
            char filename[40];
            sprintf(filename, "correct_animation\\%d.png", frameIndex);

            // Load and draw the current frame
            FEHImage frameImage(filename);
            frameImage.Draw(0, 0);
            LCD.Update();

            frameIndex++;

            lastFrameTimeMs = currentTimeMs;
        }
    }

    Sleep(1.0);
}

/* FUNCTION: Plays the animation for an INCORRECT answer as a sequence of premade frames.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                                                             */
void incorrect_animation() {
    int frameIndex = 1;
    const unsigned int frameDelayMs = 10;  // Each frame lasts for 10 ms
    unsigned int lastFrameTimeMs = TimeNow() * 1000;

    while (frameIndex <= 31) {
        unsigned int currentTimeMs = TimeNow() * 1000;

        // Only update the frame when the appropriate time has passed
        if ((currentTimeMs - lastFrameTimeMs) >= frameDelayMs) {
            char filename[40];
            sprintf(filename, "incorrect_animation\\%d.png", frameIndex);

            // Load and draw the current frame
            FEHImage frameImage(filename);
            frameImage.Draw(0, 0);
            LCD.Update();

            frameIndex++;

            lastFrameTimeMs = currentTimeMs;
        }
    }

    Sleep(1.0);
}

/* FUNCTION: Emulates the Higher Lower Game's "sliding" animation.
             Slides the current prompt off the screen and brings a new one into view.
    Author: Niko
    Arguments:
        index1 - Index of the first prompt.
        index2 - Index of the second prompt.
        newIndex - Index of the new prompt to slide into view.
    Returns:
        NONE                                                                        */
void slidePrompts(int index1, int index2, int newIndex) {
    const int screenWidth = 320;

    /* Start positions for each prompt (think three image columns: 1 must slide off the screen from position left, 2 must slide from 
    position right to position left, and 3 must slide from off the screen to position right) */

    int start1 = 0;              // Activity 1 starts from from the leftmost position
    int start2 = 160;            // Activity 2 starts from the middle of the screen
    int start3 = screenWidth;    // Activity 3 starts off-screen on the right

    int end1 = -160;              // End position of Activity 1 (off-screen left)
    int end2 = 0;                 // End position of Activity 2 (leftmost position)
    int end3 = 160;               // End position of Activity 3 (middle of the screen)

    int steps = 30;               // Total number of frames (adjust to control duration of animation)
    
    for (int i = 0; i <= steps; i++) {
        // Calculate t (in range [0, 1]) and interpolate position coordinate
        float t = (float)i / steps;
        t = 1 - pow(1 - t, 5);

        // Calculate the interpolated positions
        int position1 = start1 + (end1 - start1) * t;
        int position2 = start2 + (end2 - start2) * t;
        int position3 = start3 + (end3 - start3) * t;

        // Draw Prompt 1 if it is still on the screen
        if (position1 + 160 > 0) {
            FEHImage image1;
            char filename1[20];
            sprintf(filename1, "emissions_images\\%d.png", index1);
            image1.Open(filename1);
            image1.Draw(position1, 0);
        }

        // Draw Prompt 2 (always on the screen)
        FEHImage image2;
        char filename2[20];
        sprintf(filename2, "emissions_images\\%d.png", index2);
        image2.Open(filename2);
        image2.Draw(position2, 0);

        // Draw Prompt 3 if it has started to slide in
        if (position3 < screenWidth) {
            FEHImage image3;
            char filename3[20];
            sprintf(filename3, "emissions_images\\%d.png", newIndex);
            image3.Open(filename3);
            image3.Draw(position3, 0);
        }

        // Update the LCD screen to reflect the new positions
        LCD.Update();

        Sleep(5);
    }
}

/* FUNCTION: Draws the premade note buttons image on screen.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                               */
void drawNoteButtons() {
    FEHImage note("images\\note_buttons.png");
    note.Draw(0,0);
}

/* FUNCTION: Emulates the Higher Lower Game's value "scrolling" animation.
             Displays the emissions value of an activity in a scrolling manner.
    Author: Niko
    Arguments:
        index - Index of the activity whose value is to be displayed.
    Returns:
        NONE                                                                   */
void scrollingValue(int index) {
    char filename[20];
    sprintf(filename, "emissions_images\\%d.png", index);
    FEHImage rightImage(filename);

    double emissionValue = emissions[index].emissionValue;
    char valueText[20];
    float currentValue = 0.0;
    int interval = 5; // ms for each update
    int totalFrames = 30; // Number of frames to complete scrolling effect
    float increment = emissionValue / totalFrames;

    for (int i = 0; i < totalFrames; i++) {
        if (currentValue + increment > emissionValue) {
            increment = emissionValue - currentValue;
        }

        currentValue += increment;

        rightImage.Draw(160,0);
        drawNoteButtons();
        displayVersus();

        LCD.SetFontColor(WHITE);
        if ((int)currentValue == currentValue) {
            sprintf(valueText, "%d", (int)currentValue);
        } else {
            sprintf(valueText, "%.2f", currentValue);
        }
        int valueX = 164 + (152 - strlen(valueText) * 12) / 2;
        LCD.WriteAt(valueText, valueX, 213);
        
        printTextWithinBox(emissions[index].activityDescription, WHITE, 175, 24, 312, 200);

        LCD.Update();

        Sleep(interval);
    }

    // Final display of the exact emission value
    rightImage.Draw(160,0);
    displayVersus();
    drawNoteButtons();
    LCD.SetFontColor(WHITE);
    
    if (emissions[index].emissionValue == (int)emissions[index].emissionValue) {
        sprintf(valueText, "%d", (int)emissions[index].emissionValue);
    } else if (emissions[index].emissionValue == (int)(emissions[index].emissionValue * 10) / 10.0) {
        sprintf(valueText, "%.1f", emissions[index].emissionValue);
    } else {
        sprintf(valueText, "%.2f", emissions[index].emissionValue);
    }

    printTextWithinBox(emissions[index].activityDescription, WHITE, 175, 24, 312, 200);
    printTextWithinBox(valueText, WHITE, 164, 200, 316, 216);
    printTextWithinBox("kg CO2eq", WHITE, 164, 220, 316, 236);

    Sleep(1.0);   // Keep value up before moving on too quick
}

/* FUNCTION: Draws the premade "briefing"/"before you play" image to the screen.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                                                    */
void displayBriefing() {
    FEHImage a("images\\before_you_play1.png");
    FEHImage b("images\\before_you_play2.png");
    
    a.Draw(0,0);
    LCD.Update();

    while (1) {
        if (buttonPress(0,0,320,240)) {
            break;
        }
    }

    b.Draw(0,0);
    LCD.Update();

    while (1) {
        if (buttonPress(0,0,320,340)) {
            break;
        }
    }

    return;
}

/* FUNCTION: Main game loop that handles playing the game and the overall Higher Lower Game inspired logic.
    Author: Reagan and Niko
    Arguments:
        NONE
    Returns:
        NONE                                                                                               */
void playGame() {
    displayBriefing();

    LCD.Clear(BLACK);
    LCD.Update();

    int index1, index2, currentIndex, newIndex;
    int score = 0;
    bool gameOn = true;

    // Get the initial two distinct prompts
    getDistinctInts(DATA_SIZE, &index1, &index2);

    while (gameOn) {
        // Display both activities
        displayActivityLeft(index1);
        displayActivityRight(index2);
        displayVersus();
        LCD.Update();

        char choice;
        while (true) {
            // Check if the user presses "Higher" or "Lower" button
            if (buttonPress(164, 213, 238, 236)) {
                choice = 'H';   // User chose "Higher"
                break;
            }
            if (buttonPress(242, 213, 316, 236)) {
                choice = 'L';   // User chose "Lower"
                break;
            }

            // Check if the user presses the left or right note buttons
            if (buttonPress(136, 4, 157, 24)) {
                LCD.Clear(BLACK);
                printTextWithinBox(emissions[index1].activityNote, WHITE, 0, 0, 320, 240);
                LCD.Update();

                while (1) {
                    if (buttonPress(0, 0, 320, 240)) {
                        displayActivityLeft(index1);
                        displayActivityRight(index2);
                        displayVersus();
                        LCD.Update();
                        break;
                    }
                    Sleep(1);
                }
            }
            if (buttonPress(296, 4, 317, 24)) {
                LCD.Clear(BLACK);
                printTextWithinBox(emissions[index2].activityNote, WHITE, 0, 0, 320, 240);
                LCD.Update();

                while (1) {
                    if (buttonPress(0, 0, 320, 240)) {
                        displayActivityLeft(index1);
                        displayActivityRight(index2);
                        displayVersus();
                        LCD.Update();
                        break;
                    }
                    Sleep(1);
                }
            }

            Sleep(1);
        }

        scrollingValue(index2);

        // Determine if the user was correct
        if ((choice == 'H' && emissions[index2].emissionValue > emissions[index1].emissionValue) ||
            (choice == 'L' && emissions[index2].emissionValue < emissions[index1].emissionValue)) {
            // Correct guess
            correct_animation();
            score++;

            int previousLeftIndex = index1;
            int previousRightIndex = index2;
            currentIndex = index2;
            getDistinctIntForNextRound(DATA_SIZE, currentIndex, &newIndex);
            index1 = currentIndex;
            index2 = newIndex;

            slidePrompts(previousLeftIndex, previousRightIndex, newIndex);

        } else {
            // Incorrect guess, end the game
            gameOn = false;
            incorrect_animation();
        }
    }

    // Append the losing score to the statistics file
    FILE* file = fopen("losing_scores.txt", "a");
    if (file) {
        fprintf(file, "%d\n", score);
        fclose(file);
    } else {
        printf("Error: Unable to write to losing_scores.txt\n");
    }

    // Display losing screen with score and GIF
    losingScreen(score);
}

/* FUNCTION: Allows for unbroken navigation of the main menu and its required screens.
    Author: Niko
    Arguments:
        NONE
    Returns:
        NONE                                                                          */
void mainMenu() {
    LCD.Clear(BLACK);

    drawButtonWithText(70, 0, 250, 42, WHITE, "Play Game", WHITE);
    drawButtonWithText(70, 48, 250, 90, WHITE, "Instructions", WHITE);
    drawButtonWithText(70, 96, 250, 138, WHITE, "Credits", WHITE);
    drawButtonWithText(70, 144, 250, 186, WHITE, "Leaderboard", WHITE);

    drawButtonWithText(120, 206, 200, 239, WHITE, "Quit", WHITE);

    LCD.Update();
    
    // Check for button presses and navigate accordingly
    while (1) {
        if (buttonPress(70, 0, 250, 42)) {
            playGame();
            break;
        }
        if (buttonPress(70, 48, 250, 90)) {
            instructionsScreen();
            break;
        }
        if (buttonPress(70, 96, 250, 138)) {
            creditsScreen();
            break;
        }
        if (buttonPress(70, 144, 250, 186)) {
            leaderboardScreen();
            break;
        }
        if (buttonPress(120, 206, 200, 239)) {
            break;
        }

        Sleep(1);
    }
}