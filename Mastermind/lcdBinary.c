/* ***************************************************************************** */
/* You can use this file to define the low-level hardware control fcts for       */
/* LED, button and LCD devices.                                                  */ 
/* Note that these need to be implemented in Assembler.                          */
/* You can use inline Assembler code, or use a stand-alone Assembler file.       */
/* Alternatively, you can implement all fcts directly in master-mind.c,          */  
/* using inline Assembler code there.                                            */
/* The Makefile assumes you define the functions here.                           */
/* ***************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "i2c.h"

#ifndef	TRUE
#  define	TRUE	(1==1)
#  define	FALSE	(1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)
#define	INPUT			 0
#define	OUTPUT			 1
#define	LOW			 0
#define	HIGH			 1
#define DELAY   200

int failure (int fatal, const char *message, ...);
extern void delay(unsigned int howLong);

/* ----------------------------------------------------------------------------- */
/* Functions to implement here (or directly in master-mind.c) */

/* this version needs i2c as argument, because it is in a separate file */
/* Send a byte to the I2C controller */
void i2c_send_byte(volatile uint32_t *i2c, uint8_t byte) {
  asm volatile (
    "str %1, [%0, #0x10]\n\t"
    "mov r2, #0x80\n\t"
    "orr r2, r2, #0x8000\n\t"
    "str r2, [%0, #0x00]\n\t"
    "1:\n\t"
    "ldr r2, [%0, #0x04]\n\t"
    "tst r2, #2\n\t"
    "beq 1b\n\t"
    "ldr r2, =0x302\n\t"
    "str r2, [%0, #0x04]\n\t"
    :
    : "r" (i2c), "r" (byte)
    : "r2"
  );
}

/* ----------------------------------------------------------------------------- */
/* adapted from setPinMode */
/* Set GPIO pin mode (INPUT or OUTPUT) */
void pinMode(uint32_t *gpio, int pin, int mode) {
  int fsel = pin / 10;
  int shift = (pin % 10) * 3;
  asm volatile (
    "ldr r2, [%0]\n\t"
    "mov r3, #7\n\t"
    "lsl r3, r3, %2\n\t"
    "bic r2, r2, r3\n\t"
    "mov r3, %3\n\t"
    "lsl r3, r3, %2\n\t"
    "orr r2, r2, r3\n\t"
    "str r2, [%0]\n\t"
    :
    : "r" (gpio + fsel), "r" (pin), "r" (shift), "r" (mode)
    : "r2", "r3"
  );
}

/* ----------------------------------------------------------------------------- */
/* Write value (HIGH or LOW) to an LED pin */
void writeLED(uint32_t *gpio, int led, int value) {
  int offset = (value) ? 7 : 10;
  asm volatile (
    "mov r2, #1\n\t"
    "lsl r2, r2, %1\n\t"
    "str r2, [%0, %2]\n\t"
    :
    : "r" (gpio), "r" (led), "r" (offset * 4)
    : "r2"
  );
}

/* ----------------------------------------------------------------------------- */
/* Read the state of the button */
int readButton(uint32_t *gpio, int button) {
  volatile uint32_t *gplev0 = gpio + 13; // GPLEV0 = 0x34 / 4 = 13
  int state = (*gplev0 & (1 << button)) ? 1 : 0;
  return state;
}

/* ----------------------------------------------------------------------------- */
/* Wait for button press */
/* you can use readButton(), depending on your implementation */
void waitForButton(uint32_t *gpio, int button) {
  while (1) {
    int state = readButton(gpio, button);
    if (state == HIGH) {
      delay(DELAY);
      while (readButton(gpio, button) == HIGH) delay(10);
      break;
    }
    delay(10);
  }
}
