#include <p32xxxx.h>
#include <stdio.h>
#include <stdlib.h>
#define _SUPPRESS_PLIB_WARNING
#include <plib.h>

// Configuration Bits
#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz)
                                    // see figure 8.1 in datasheet for more info
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FSOSCEN = OFF        // Disable Secondary Oscillator

//Debugging configuration
#pragma config ICESEL = ICS_PGx2// ICE / ICD Comm Channel Select
#pragma config DEBUG = ON       // Debugger Disabled for Starter Kit

#define SYSCLK  40000000L

int ADCConfig() {
    //Configure Analog Port Inputs
    AD1PCFG = 0x00000000;   //PORTB as Analog ADC Inputs
    TRISB = 0xFFFFFFFF;     //PORTB as Analog Input

    //Select Format of ADC Result
    AD1CON1bits.FORM = 001; //Signed Integer 16-Bits

    //Sample Clock Source
    AD1CON1bits.SSRC = 010; //Timer2 period match ends sampling and starts conversion
    AD1CON1bits.ASAM = 1;

    //Select Voltage Reference Source
    AD1CON2


}

int main(int argc, char** argv) {
    SYSTEMConfigPerformance(SYSCLK);

    return (EXIT_SUCCESS);
}

