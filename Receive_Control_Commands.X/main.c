/*
    \file   main.c
    \brief  Main file of the project.
    (c) 2020 Microchip Technology Inc. and its subsidiaries.
    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#define F_CPU 4000000UL

#include <avr/io.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>

#define BAUD_RATE 115200
#define USART0_BAUD_RATE(BAUD_RATE) (uint16_t)((float)((F_CPU) * 64 / (16 * (float)(BAUD_RATE))) + 0.5)

#define MAX_COMMAND_LEN		8


/* Default fuses configuration:
 - BOD disabled
 - Oscillator in High-Frequency Mode
 - UPDI pin active(WARNING: DO NOT CHANGE!)
 - RESET pin used as GPIO
 - CRC disabled
 - MVIO enabled for dual supply
 - Watchdog Timer disabled
 */
FUSES =
{
.BODCFG = ACTIVE_DISABLE_gc | LVL_BODLEVEL0_gc | SAMPFREQ_128Hz_gc | SLEEP_DISABLE_gc,
.BOOTSIZE = 0x0,
.CODESIZE = 0x0,
.OSCCFG = CLKSEL_OSCHF_gc,
.SYSCFG0 = CRCSEL_CRC16_gc | CRCSRC_NOCRC_gc | RSTPINCFG_GPIO_gc | UPDIPINCFG_UPDI_gc,
.SYSCFG1 = MVSYSCFG_DUAL_gc | SUT_0MS_gc,
.WDTCFG = PERIOD_OFF_gc | WINDOW_OFF_gc,
};

void USART0_init(void)
{
    PORTMUX.USARTROUTEA = PORTMUX_USART0_ALT3_gc;   /* USART0 routed to PORTD that is connected to CDC */
	PORTD.DIRSET = PIN4_bm;
	PORTD.DIRCLR = PIN5_bm;
    
    USART0.BAUD  = USART0_BAUD_RATE(115200);
    USART0.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}

void USART0_sendChar(char c)
{
    while (!(USART0.STATUS & USART_DREIF_bm)); 

    USART0.TXDATAL = c;
}

void SendString(const char* pStr)
{     
    while(*pStr)
        USART0_sendChar(*pStr++); 
}

char USART0_getChar(void)
{
    while (!(USART0.STATUS & USART_RXCIF_bm));
  
    return USART0.RXDATAL;
}

void LED_on(void)
{
    PORTF.OUTCLR = PIN5_bm;
}

void LED_off(void)
{
    PORTF.OUTSET = PIN5_bm;
}

void LED_toggle(void)
{
    PORTF.OUTTGL = PIN5_bm;
}

void LED_init(void)
{
    PORTF.OUTSET = PIN5_bm;
    PORTF.DIRSET = PIN5_bm;
}

void getCommand(char *command)
{
    uint8_t index = 0;

    if(command == NULL)
        return;

    while(1)
    {
        char c = USART0_getChar();
        USART0_sendChar(c); /* perform echo */
        if(c != '\n' && c != '\r')
            command[index++] = c;
        else
        {
            SendString("\n\r");
            break;
        }
        if(index >= MAX_COMMAND_LEN)
        {
            index = 0;
            SendString("\n\r");
            break;
        }
    }
    command[index] = '\0';
}

void executeCommand(const char *command)
{
    if(command != NULL)
    {
        if(strcmp(command, "ON") == 0)
        {
            LED_on();
            SendString("OK, LED ON.\n\r");
            return;
        }
        else if (strcmp(command, "OFF") == 0)
        {
            LED_off();
            SendString("OK, LED OFF.\n\r");
            return;
        }
        else if (strcmp(command, "TOG") == 0)
        {
            LED_toggle();
            SendString("OK, LED TOGGLE.\n\r");
            return;
        } 
    }
    SendString("Type ON/OFF/TOG to control the LED.\n\r");
}

int main(void)
{
    USART0_init();
    LED_init();

	/* This delay allows USB CDC device to connect */
    _delay_ms(2000);
	
    /* this will printout: "Type ON/OFF/TOG to control the LED.\n\r"  */
    executeCommand("");
    
    while (1)
    {
        char command[MAX_COMMAND_LEN];
        getCommand(command);
        executeCommand(command);
    }
}
