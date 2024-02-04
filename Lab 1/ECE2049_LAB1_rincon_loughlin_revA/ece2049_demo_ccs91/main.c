/************** ECE2049 DEMO CODE ******************/
/**************  25 August 2021   ******************/
/***************************************************/

#include <msp430.h>
/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"
#include <stdlib.h>

// Function Prototypes
void swDelay(char numLoops);
void init();
void countdownScreen();
int randfunc();
void toggleWelcomeScreen();
void  resetTopRow();


// Declare globals here
enum state {welcome, countdown, drawAliens, checkPos, checkKey, updateAliens, loser};
enum state curr_state = welcome;
#define MAX_ALIEN 5
unsigned int MAX_COUNTER = 40000;
volatile char aliens[MAX_ALIEN][MAX_ALIEN];

// globals
int k;
int i;
int j;
int num;
char randomChar;
unsigned int numFound = 0;
int toggleWelcome = 1;  // turns the welcome screen on and off.
char random;
char randfuncChar();

unsigned char val[3];
unsigned char dispThree[3];
unsigned int counter = 0;
unsigned int roundCompleted = 0;


void main(void)
{

    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
                                 // You can then configure it properly, if desired

    unsigned char currKey=0;

        // ***   Useful code starts here   ***


        dispThree[0] = ' ';
        dispThree[2] = ' ';

        val[0] = ' ';
        val[2] = ' ';
        init();


        while (1)    // Forever loop
            {

            currKey = getKey();

            switch(curr_state) {
                case welcome:
                    if(toggleWelcome) {
                        init();
                        toggleWelcome = 0;
                    }
                    if(currKey == '*') {
                        curr_state = countdown;
                    }
                    break;

                case countdown:
                      countdownScreen();
                      curr_state = drawAliens;
                      break;


                case drawAliens:

                    // generate numbers
                      for(k = randfunc(); k > 0; k--) {
                          aliens[0][randfunc()] = randfuncChar();  // [row] [column]
                           }

                      Graphics_clearDisplay(&g_sContext); // Clear the display

                      // place aliens on screen
                           for(i = 0; i < 5; i++) {
                               for(j = 0; j < 5; j++) {
                                   if(aliens[i][j] > '0') {
                                       int x = (i + 1) * 15;
                                       int y = (j + 1) * 15;
                                       val[1] = aliens[i][j];
                                       Graphics_drawStringCentered(&g_sContext, val, AUTO_STRING_LENGTH, y, x, TRANSPARENT_TEXT);


                                       // Refresh the display so it shows the new data
                                       Graphics_flushBuffer(&g_sContext);
                                   }
                               }
                           }

                           curr_state = checkPos;

                                     break;


                case checkPos:
                            for(i = 0; i < 5; i++) {
                               if(aliens[4][i] > '0') {
                                   curr_state = loser;
                               } else {
                                   curr_state = checkKey;
                               }

                               }

                            roundCompleted++;

                                  if(roundCompleted == 4) {
                                     roundCompleted = 0;
                                       MAX_COUNTER /= 2;
                              }

                            break;

                case checkKey:

//                    while the counter is less than the max counter
                    // we are going to poll for a key press and
                    // increment the timer.

                    // when the counter is greater than the max counter, the while loop will stop executing
                    // and we will have a value for currKey

                    // well we need to check for a valid keyPress. why keep polling when we have what we need?

                    while(counter < MAX_COUNTER) {

                        currKey = getKey();

                        if(currKey >= '1' && currKey <= '5') {
                            break;
                        }

                        counter++;
                    }

                        counter = 0;

                            // check if valid input
                          if(currKey >= '1' && currKey <= '5') {

                              i = 4;  //row
                              j = 0; // column

                             while(1) {
                                 if(aliens[i][j] > '0') {
                                    // numFound = 1;
                                     break;
                                 }

                                 j++;

                              if(j > 4) {
                                  i--;
                                  j = 0;
                             }
                           }

                             for(j = 0; j < 5; j++) {
                                  if(aliens[i][j] == currKey && aliens[i][j] > '0') {
                                     aliens[i][j] = '0';
                                     break;
                                     }
                                 }
                             }

                          curr_state = updateAliens;
                          currKey = '0';

                         break;

                case updateAliens:

                for(i = 4; i > 0; i--) {
                    for(j = 0; j < 5; j++) {
                        aliens[i][j] = aliens[i-1][j];
                       }
                        }

                    resetTopRow();
                      curr_state = drawAliens;
                      break;

                case loser:

                    Graphics_clearDisplay(&g_sContext); // Clear the display

                       // Write some text to the display
                      Graphics_drawStringCentered(&g_sContext, "LOL LOSER", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);

                      // Draw a box around everything because it looks nice
                            Graphics_Rectangle box = {.xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
                            Graphics_drawRectangle(&g_sContext, &box);

                            // We are now done writing to the display.  However, if we stopped here, we would not
                            // see any changes on the actual LCD.  This is because we need to send our changes
                            // to the LCD, which then refreshes the display.
                            // Since this is a slow operation, it is best to refresh (or "flush") only after
                            // we are done drawing everything we need.

                            Graphics_flushBuffer(&g_sContext);

                           swDelay(5);

                            curr_state = welcome;


                    } // switch closing

            counter++;

            } // while loop close

            }


void swDelay(char numLoops)
{
    // This function is a software delay. It performs
    // useless loops to waste a bit of time
    //
    // Input: numLoops = number of delay loops to execute
    // Output: none
    //
    // smj, ECE2049, 25 Aug 2021

    volatile unsigned int i,j;  // volatile to prevent removal in optimization
                                // by compiler. Functionally this is useless code

    for (j=0; j<numLoops; j++)
    {
        i = 50000 ;                 // SW Delay
        while (i > 0)               // could also have used while (i)
           i--;
    }
}

void init() {

    configDisplay();
    configKeypad();

    // *** Intro Screen ***
       Graphics_clearDisplay(&g_sContext); // Clear the display

       // Write some text to the display
       Graphics_drawStringCentered(&g_sContext, "SPACE", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
       Graphics_drawStringCentered(&g_sContext, "INVADERS", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
       Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
       Graphics_drawStringCentered(&g_sContext, "to START", AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);


       // Draw a box around everything because it looks nice
       Graphics_Rectangle box = {.xMin = 5, .xMax = 91, .yMin = 5, .yMax = 91 };
       Graphics_drawRectangle(&g_sContext, &box);

       // We are now done writing to the display.  However, if we stopped here, we would not
       // see any changes on the actual LCD.  This is because we need to send our changes
       // to the LCD, which then refreshes the display.
       // Since this is a slow operation, it is best to refresh (or "flush") only after
       // we are done drawing everything we need.

       Graphics_flushBuffer(&g_sContext);

}

void countdownScreen() {

    // *** Countdown Screen ***


          char runTime = 0x33; // ASCII representation of value 3
          int run = 3;
          unsigned char result[3];
          result[0] = ' ';
          result[2] = ' ';
          long int counter = 0;

          while(run > 0) {
              // we will assume this is one second
              if(counter == MAX_COUNTER) {
                  Graphics_clearDisplay(&g_sContext); // Clear the display
                  result[1] = runTime;
                  Graphics_drawStringCentered(&g_sContext, result , AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT); // takes in string
                  Graphics_flushBuffer(&g_sContext);
                  runTime--;
                  run--;
                  counter = counter - MAX_COUNTER;
              }
              counter++;
          }
          toggleWelcomeScreen();
          Graphics_clearDisplay(&g_sContext); // Clear the display
}


// function generates random number from 1 to 5
int randfunc() {
    while(1) {
        num = rand();
        int modNum = num % 6;
        if( modNum > 0) {
            return modNum;
        }
    }
}

void toggleWelcomeScreen() {
    toggleWelcome = 1;
}


// function generates random char from 49 (1) to 53 (5)
char randfuncChar() {
    while(1) {
        randomChar = rand();
        if(randomChar >= 49 && randomChar <= 53) {
            return randomChar;
        }
    }
}

void resetTopRow() {

    for(i = 0; i < 5; i++) {
        aliens[0][i] = '0';
    }

}




