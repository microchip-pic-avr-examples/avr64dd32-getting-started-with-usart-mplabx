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
#include <stdint.h>
#include <stdbool.h>


#define BAUD_RATE 9600
/* waiting for 1 byte duration, 1 byte = 10 bits, expressed in multiples of 10us */
#define RX_TIMEOUT     (uint8_t)(10 * 100000UL / BAUD_RATE + 1) 

#define USART0_BAUD_RATE(BAUD_RATE) (uint16_t)((float)((F_CPU) * 64 / (16 * (float)(BAUD_RATE))) + 0.5)

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

void USART0_oneWireSendByte(uint8_t tx_data)
{
    while (!(USART0.STATUS & USART_DREIF_bm));
    USART0.TXDATAL = tx_data;
}

void USART0_oneWireCleanRx(void)
{
    do
    {   /* flush RX buffer */
        (void)USART0.RXDATAL;
    } while (USART0.STATUS & USART_RXCIF_bm);
}

bool USART0_oneWireRxLoopBack(uint8_t sent_data)
{
    uint8_t timeout_counter = RX_TIMEOUT;

    while (!(USART0.STATUS & USART_RXCIF_bm) && (timeout_counter != 0) )
    {
        timeout_counter--;
        _delay_us(10);
    }
    if(timeout_counter == 0)
        return true; /* timeout on RX, return error */

    if(USART0.RXDATAL != sent_data)
        return true; /* received loopback data is corrupted, return error */
    else
        return false;
}

bool SendString(const char* pStr)
{
    uint8_t sent_data;
    bool    collision_detected = false;

    while(*pStr)
    {
        sent_data = *pStr++;
        USART0_oneWireCleanRx();
        USART0_oneWireSendByte(sent_data);
        collision_detected = USART0_oneWireRxLoopBack(sent_data);
        if(collision_detected == true)
        {
            break;
        }
    }
    return collision_detected;
}

void USART0_oneWireInit(void) 
{
    PORTA.PIN4CTRL |= PORT_PULLUPEN_bm;
    PORTMUX.USARTROUTEA = PORTMUX_USART0_ALT1_gc;   /* USART0 OneWire mode routed to PA4 */
    
    USART0.CTRLA |= USART_LBME_bm;
    USART0.CTRLB |= USART_ODME_bm;
    USART0.BAUD   = USART0_BAUD_RATE(BAUD_RATE); 
    USART0.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}

int main(void)
{
    LED_Init();
    USART0_oneWireInit();
    
    while (1) 
    {
        if(SendString("Microchip\n\r") == true)
        {
            LED_Toggle();_delay_ms(50);
            LED_Toggle();_delay_ms(50);
            LED_Toggle();_delay_ms(50);
            LED_Toggle();_delay_ms(50);
            LED_Toggle();_delay_ms(300);
        }
        else
        {
            LED_Toggle();
            _delay_ms(500);
        }
    }
}
