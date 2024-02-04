#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "peripherals.h"

unsigned int SEC_IN_MIN = 60;
unsigned int MIN_IN_HR = 60;
unsigned int HR_IN_DAY = 24;
unsigned int COUNTS_PER_SEC = 200;
#define INTERVAl 163  //0.005 s resolution
unsigned int timer_cnt;
unsigned int time;
unsigned int prev_month = 0;
char intToStr[3];
char floatToStr[5];

#define CALADC12_15V_30C  *((unsigned int *)0x1A1A)
// Temperature Sensor Calibration = Reading at 85 degrees C is stored at addr 1A1Ch                                            //See device datasheet for TLV table memory mapping
#define CALADC12_15V_85C  *((unsigned int *)0x1A1C)

unsigned int in_temp;
volatile float temperatureDegC;
float tempReadings[36];
unsigned int numOfReadings = 0;
float getAvgTemp();



struct Date {
   unsigned int month;
   unsigned int day;
   unsigned int  hour;
   unsigned int  min;
   unsigned int  sec;
};

struct Temp {
    float celcius;
    float F;
};

struct Date date;
struct Temp temp;

void initDate() {
    date.month = 0;
    date.day = 1;
    date.hour = 0;
    date.min = 0;
    date.sec = 0;
}

void initTemp() {
    temp.celcius = 0;
    temp.F = 0;
}



//function prototypes
void startTimerA2();
void displayDate();
unsigned int updateMonth(unsigned int month, unsigned int days);
const char* intToString(int num);
const char* intToMonth(unsigned int num);
void displayTemp(float inAvgTempC);
char intToChar(int num);
float readTemp();
char readButtons();
void editSettings();



int main() {

    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
    //enable_interrupt();
        _BIS_SR(GIE);

        REFCTL0 &= ~REFMSTR;     // Reset REFMSTR to hand over control of
                                 // internal reference voltages to
                                 // ADC12_A control registers
        ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON;     // Internal ref = 1.5V

        ADC12CTL1 = ADC12SHP;                     // Enable sample timer

         // Using ADC12MEM0 to store reading
        ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC i/p ch A10 = temp sense
                                               // ACD12SREF_1 = internal ref = 1.5v

         __delay_cycles(100);                    // delay to allow Ref to settle
         ADC12CTL0 |= ADC12ENC;              // Enable conversion




    initDate();
    startTimerA2();
    configDisplay();
    displayDate();


    while(1) {
        if((timer_cnt % COUNTS_PER_SEC) == 0) {

            if(readButtons() & BIT0) {
                stopTimer();
                editSettings();
            }

            temperatureDegC = readTemp();
            tempReadings[numOfReadings % 36] = temperatureDegC;
            numOfReadings++;

            volatile float tempToDisplay = (numOfReadings < 36) ? temperatureDegC : (getAvgTemp());

            displayDate();
            displayTemp(tempToDisplay);
            Graphics_flushBuffer(&g_sContext); // update screen

        }

    }


}


float readTemp() {

    volatile float degC_per_bit;
    volatile unsigned int bits30, bits85;

       ADC12CTL0 &= ~ADC12SC;      // clear the start bit
       ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
                                   // Single conversion (single channel)

       // Poll busy bit waiting for conversion to complete
       while (ADC12CTL1 & ADC12BUSY)
           __no_operation();

       in_temp = ADC12MEM0;      // Read in results if conversion

       // Temperature in Celsius. See the Device Descriptor Table section in the
       // System Resets, Interrupts, and Operating Modes, System Control Module
       // chapter in the device user's guide for background information on the
       // formula.

       // Use calibration data stored in info memory
               bits30 = CALADC12_15V_30C;
               bits85 = CALADC12_15V_85C;
               degC_per_bit = ((float)(85.0 - 30.0))/((float)(bits85-bits30));


       return (float)((long) in_temp - CALADC12_15V_30C) * degC_per_bit +30.0;

}



void startTimerA2() {

    TA2CTL = TASSEL_1 + MC_1 + ID_0;
    TA2CCR0 = INTERVAl; //
    TA2CCTL0 = CCIE;
}

#pragma vector=TIMER2_A0_VECTOR
    __interrupt void TimerA2_ISR() {
        timer_cnt++;

        if((timer_cnt % COUNTS_PER_SEC) == 0)
        {
            date.sec++;

            if(date.sec == 60) {
                date.min++;
                date.sec = 0;
            }

            if(date.min == 60) {
                date.min = 0;
                date.hour++;
            }

            if(date.hour == 24) {
                date.day++;
                date.hour = 0;
            }

            if(date.day >= 28 && updateMonth(date.month, date.day)){

                date.month++;
                date.day = 1;

                }
            }

    }


void stopTimer() {
        TA2CTL = MC_0;
        TA2CCTL0 &= ~CCIE;
        timer_cnt = 0;
        time = 0;
    }

void displayDate() {


    Graphics_clearDisplay(&g_sContext); // Clear the display

    // Write some text to the display
           Graphics_drawStringCentered(&g_sContext, intToMonth(date.month), AUTO_STRING_LENGTH, 38, 15, TRANSPARENT_TEXT);
           Graphics_drawStringCentered(&g_sContext, intToString(date.day), AUTO_STRING_LENGTH, 58, 15, TRANSPARENT_TEXT);

           Graphics_drawStringCentered(&g_sContext, intToString(date.hour), AUTO_STRING_LENGTH, 28, 25, TRANSPARENT_TEXT);
           Graphics_drawStringCentered(&g_sContext, " : ", AUTO_STRING_LENGTH, 38, 25, TRANSPARENT_TEXT);

           Graphics_drawStringCentered(&g_sContext, intToString(date.min), AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
           Graphics_drawStringCentered(&g_sContext, " : ", AUTO_STRING_LENGTH, 58, 25, TRANSPARENT_TEXT);

           Graphics_drawStringCentered(&g_sContext, intToString(date.sec), AUTO_STRING_LENGTH, 68, 25, TRANSPARENT_TEXT);

}

const char* intToString(int num) {

    sprintf(intToStr, "%d", num);
    return intToStr;

}

char intToChar(int num) {
    return num + '0';

}

const char* floatToString(float fnum) {

    float remain = 0;

    // converts the hundreds place
    unsigned int hundreds = fnum / 100;
    remain = fnum - (hundreds * 100);

    // converts the tens place
    unsigned int tens = remain / 10;
    remain = remain - (tens*10);

    // converts the ones place
    unsigned int ones = floor(remain);
    remain = remain - ones;

    // converts the tenths place
    unsigned int tenths = floor(remain * 10);

    floatToStr[0] = intToChar(hundreds);
    floatToStr[1] = intToChar(tens);
    floatToStr[2] = intToChar(ones);
    floatToStr[3] = '.';
    floatToStr[4] = intToChar(tenths);



    return floatToStr;

}


const char* intToMonth(unsigned int num) {

    unsigned int month = num % 12;

    switch(month) {

    case 0:
        return "JAN";
    case 1:
        return "FEB";
    case 2:
        return "MAR";
    case 3:
        return "APR";
    case 4:
        return "MAY";
    case 5:
        return "JUN";

    case 6:
        return "JUL";
    case 7:
        return "AUG";
    case 8:
        return "SEP";
    case 9:
        return "OCT";
    case 10:
        return "NOV";
    case 11:
        return "DEC";
    }

    return "";

}

unsigned int updateMonth(unsigned int month, unsigned int days) {

    if((month == 0 || month == 2 || month == 4 || month == 6 || month == 7 || month == 9 || month == 11 ) && days == 32) {
        // returns true if it is one of the months with 31 days

        return 1;
    } else if((month == 3 || month == 5 || month == 8 || month == 10) && days == 31) {
        // months with 30 days
        return 1;
    } else if(month == 1 && days == 29) {
        return 1;
    }

    return 0;

}

void displayTemp(float inAvgTempC) {
    temp.celcius = inAvgTempC;
    temp.F = (9.0/5.0) * inAvgTempC + 32.0;

    Graphics_drawStringCentered(&g_sContext, floatToString(temp.celcius), AUTO_STRING_LENGTH, 48, 60, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, floatToString(temp.F), AUTO_STRING_LENGTH, 48, 70, TRANSPARENT_TEXT);
}

float getAvgTemp() {

    float tempCollector = 0;
    int i;

                    for(i = 0; i < 36; i++) {
                        tempCollector += tempReadings[i];
                    }

                    return tempCollector / 36.0;

                }


char readButtons() {
    // Button S1: 7.0 and S4: 7.4 (these are configured correctly)
        P7SEL &= ~(BIT0 | BIT4);
        P7DIR &= ~(BIT0 | BIT4);
        P7REN |=  (BIT0 | BIT4);
        P7OUT |=  (BIT0 | BIT4);

        char x;
        char v;


        x = (~(P7IN) & (BIT0));        //Button S1  7.0  Enables Editing State
        v = ~(P7IN) & (BIT4);         //Button S4   7.4  Disables Editing and Saves Settings

        char outbits = (x | v);
        return outbits;

}

void editSettings() {

    ADC12CTL0 = ADC12SHT0_9 | ADC12ON;     // No internal reference needed
    ADC12CTL1 = ADC12SHP;                     // Enable sample timer

    // Using ADC12MEM0 to store reading
    ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0;  // ADC i/p ch A0 = scroll wheel
                                                   // ACD12SREF_0 = AVCC 3.3v

    __delay_cycles(100);                    // delay to allow Ref to settle
    ADC12CTL0 |= ADC12ENC;              // Enable conversion

    unsigned int scrollWheelADC;
    while(!(readButtons() & BIT4)) {

        ADC12CTL0 &= ~ADC12SC;      // clear the start bit
        ADC12CTL0 |= ADC12SC;       // Sampling and conversion start
                                           // Single conversion (single channel)
        // Poll busy bit waiting for conversion to complete
        while (ADC12CTL1 & ADC12BUSY)
              __no_operation();

        scrollWheelADC = ADC12MEM0 & 0x0FFF;

        if()
    }
}

