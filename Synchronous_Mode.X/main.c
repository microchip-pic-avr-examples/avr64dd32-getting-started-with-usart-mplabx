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

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define BAUD_RATE 1000000UL
#define USART_SYNC_BAUD_RATE(BAUD_RATE) (((uint16_t)((float)((F_CPU) / (2 * (float)(BAUD_RATE))) + 0.5)) << 6)

#define DATA_HOST      'H'
#define DATA_CLIENT    'C'

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

void LED_Init(void)
{
    PORTF.OUTSET = PIN5_bm;
    PORTF.DIRSET = PIN5_bm;
}

void LED_Toggle(void)
{
    PORTF.OUTTGL = PIN5_bm;
}

ISR(USART0_RXC_vect)
{
    /* this is host ISR, receiving data from client */
    if(USART0.RXDATAL == DATA_CLIENT)
        LED_Toggle();
}

ISR(USART1_RXC_vect)
{
    /* this is client ISR, receiving data from host */
    if(USART1.RXDATAL == DATA_HOST)
        LED_Toggle();
}

static void USART0_syncInit(void)
{
    /* USART0 is configured in host mode */
    /*USART0 routed to PA4, PA5, PA6 */
    PORTMUX.USARTROUTEA = PORTMUX_USART0_ALT1_gc; 

    PORTA.DIRSET = PIN4_bm; /* TX */
    PORTA.DIRCLR = PIN5_bm; /* RX */
    PORTA.DIRSET = PIN6_bm; /* XCK - USART0 generates clock */
    
    USART0.CTRLA |= USART_RXCIE_bm;  
    USART0.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;  
    USART0.CTRLC |= USART_CMODE_SYNCHRONOUS_gc;
    
    USART0.BAUD = USART_SYNC_BAUD_RATE(BAUD_RATE);   
}

void USART1_syncInit(void)
{
    /* USART1 is configured in client mode */
    PORTC.DIRSET = PIN0_bm;  /* TX */
    PORTC.DIRCLR = PIN1_bm;  /* RX */
    PORTC.DIRCLR = PIN2_bm;  /* XCK - USART1 receives clock */
    
    USART1.CTRLA |= USART_RXCIE_bm; 
    USART1.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;    
    USART1.CTRLC |= USART_CMODE_SYNCHRONOUS_gc;     
    /* Baud rate setting is not required for client mode */
}

void USART0_syncWrite(char data)
{
    while (!(USART0.STATUS & USART_DREIF_bm));
    USART0.TXDATAL = data;
}

void USART1_syncWrite(char data)
{
    while (!(USART1.STATUS & USART_DREIF_bm));
    USART1.TXDATAL = data;
}

int main(void)
{
    LED_Init();
    USART0_syncInit();
    USART1_syncInit();
    
    /* Enable global interrupts */
    sei();
    
    while (1) 
    {
        USART0_syncWrite(DATA_HOST);
        _delay_us(10);
        USART1_syncWrite(DATA_CLIENT);
        _delay_us(10);
    }
}
