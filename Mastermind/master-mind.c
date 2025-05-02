/* THIS TEMPLATE IS BASED ON THE ORIGINAL CW2 TEMPLATE FROM f28hs-2021-22-cwk2-sys 
 * MasterMind implementation: template; see comments below on which parts need to be completed
 * CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
 * 

 * Compile ( manual compilation ): 
 gcc -c -o lcdBinary.o lcdBinary.c
 gcc -c -o master-mind.o master-mind.c
 gcc -o master-mind master-mind.o lcdBinary.o
 * Run:     
 sudo ./master-mind

 OR use the Makefile to build
 > make all
 and run
 > make run
 and test
 > make test

# NOTE: The header file 'i2c.h' is not explicitly included in the Makefile, as it is assumed that 'i2c.h' will remain unchanged. 
# Therefore, any modifications to this file will require manual recompilation of the affected components.

 ***********************************************************************
 * The Low-level interface to LED, button, and LCD is based on:
 * wiringPi libraries by
 * Copyright (c) 2012-2013 Gordon Henderson.
 ***********************************************************************
 * See:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
*/

/* ======================================================= */
/* SECTION: includes                                       */
/* ------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "i2c.h"

#define DEBUG
#undef ASM_CODE

#define LED 26
#define LED2 5
#define BUTTON 19
#define DELAY   200
#define TIMEOUT 3000000
#define COLS 3
#define SEQL 3

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

static const int colors = COLS;
static const int seqlen = SEQL;

static char* color_names[] = { "red", "green", "blue" };
static int* theSeq = NULL;

#define	PI_GPIO_MASK	(0xFFFFFFC0)

static unsigned int gpiobase ;
static uint32_t *gpio = NULL;
static uint32_t *i2c = NULL;
static int timed_out = 0;

// Function prototypes
int failure (int fatal, const char *message, ...);
void waitForButton (uint32_t *gpio, int button);
void i2c_send_byte(volatile uint32_t *i2c, uint8_t byte);
void pinMode(uint32_t *gpio, int pin, int mode);
void writeLED(uint32_t *gpio, int led, int value);
int readButton(uint32_t *gpio, int button);
void lcd_clear(volatile uint32_t *i2c);
void lcd_init(volatile uint32_t *i2c);
void lcd_string(volatile uint32_t *i2c, const char *message, uint8_t line);
void lcd_byte(volatile uint32_t *i2c, uint8_t bits, uint8_t mode);
void lcd_toggle_enable(volatile uint32_t *i2c, uint8_t bits);
void delay (unsigned int howLong);
void initSeq();
void showSeq(int *seq);
int countMatches(int *seq1, int *seq2);
void showMatches(int code, int *seq1, int *seq2, int lcd_format);
void readSeq(int *seq, int val);
void blinkN(uint32_t *gpio, int led, int c);

void initSeq() {
  if (theSeq == NULL) {
    theSeq = (int*)malloc(seqlen * sizeof(int));
    if (theSeq == NULL) failure(TRUE, "Failed to allocate memory for theSeq\n");
  }
  srand(time(NULL));
  for (int i = 0; i < seqlen; i++) {
    theSeq[i] = (rand() % colors) + 1;
  }
}

void showSeq(int *seq) {
  for (int i = 0; i < seqlen; i++) {
    printf("%d ", seq[i]);
  }
  printf("\n");
}

int countMatches(int *seq1, int *seq2) {
  int exact = 0, approx = 0;
  int used1[SEQL] = {0};
  int used2[SEQL] = {0};
  
  for (int i = 0; i < seqlen; i++) {
    if (seq1[i] == seq2[i]) {
      exact++;
      used1[i] = 1;
      used2[i] = 1;
    }
  }
  
  for (int i = 0; i < seqlen; i++) {
    if (!used2[i]) {
      for (int j = 0; j < seqlen; j++) {
        if (!used1[j] && seq1[j] == seq2[i]) {
          approx++;
          used1[j] = 1;
          break;
        }
      }
    }
  }
  return (exact * 10) + approx;
}

void showMatches(int code, int *seq1, int *seq2, int lcd_format) {
  int exact = code / 10;
  int approx = code % 10;
  if (lcd_format) {
    char line1[17], line2[17];
    sprintf(line1, "Exact: %d       ", exact);
    sprintf(line2, "Approx: %d      ", approx);
    lcd_clear(i2c);
    lcd_string(i2c, line1, LCD_LINE_1);
    lcd_string(i2c, line2, LCD_LINE_2);
  } else {
    printf("%d exact\n%d approximate\n", exact, approx);
  }
}

void readSeq(int *seq, int val) {
  if (seq == NULL) {
    fprintf(stderr, "Error: Sequence pointer is NULL\n");
    exit(EXIT_FAILURE);
  }
  int temp = val;
  int digitCount = 0;
  while (temp > 0) {
    digitCount++;
    temp /= 10;
  }
  if (digitCount != seqlen) {
    fprintf(stderr, "Error: Secret sequence must have exactly %d digits\n", seqlen);
    exit(EXIT_FAILURE);
  }
  for (int i = seqlen - 1; i >= 0; i--) {
    int digit = val % 10;
    if (digit < 1 || digit > colors) {
      fprintf(stderr, "Error: Each digit in secret sequence must be between 1 and %d\n", colors);
      exit(EXIT_FAILURE);
    }
    seq[i] = digit;
    val /= 10;
  }
}

static uint64_t timeInMicroseconds(){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((uint64_t)tv.tv_sec * 1000000) + tv.tv_usec;
}

static uint64_t startT, stopT;

void timer_handler (int signum) {
  stopT = timeInMicroseconds();
  if ((stopT - startT) >= TIMEOUT) timed_out = 1;
}

void initITimer(uint64_t timeout){
  struct sigaction sa;
  struct itimerval timer;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &timer_handler;
  sigaction(SIGALRM, &sa, NULL);
  timer.it_value.tv_sec = timeout / 1000000;
  timer.it_value.tv_usec = timeout % 1000000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &timer, NULL);
  startT = timeInMicroseconds();
  timed_out = 0;
}

int failure (int fatal, const char *message, ...) {
  va_list argp ;
  char buffer [1024] ;
  if (!fatal) return -1 ;
  va_start (argp, message) ;
  vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;
  fprintf (stderr, "%s", buffer) ;
  exit (EXIT_FAILURE) ;
  return 0 ;
}

void delay (unsigned int howLong) {
  struct timespec sleeper, dummy ;
  sleeper.tv_sec  = (time_t)(howLong / 1000) ;
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;
  nanosleep (&sleeper, &dummy) ;
}

void lcd_toggle_enable(volatile uint32_t *i2c, uint8_t bits) {
    i2c_send_byte(i2c, bits | ENABLE);
    usleep(E_PULSE);
    i2c_send_byte(i2c, bits & ~ENABLE);
    usleep(E_DELAY);
}

void lcd_byte(volatile uint32_t *i2c, uint8_t bits, uint8_t mode) {
    uint8_t bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    uint8_t bits_low = mode | (bits << 4)  | LCD_BACKLIGHT;
    i2c_send_byte(i2c, bits_high);
    lcd_toggle_enable(i2c, bits_high);
    i2c_send_byte(i2c, bits_low);
    lcd_toggle_enable(i2c, bits_low);
}

void lcd_init(volatile uint32_t *i2c) {
    lcd_byte(i2c, 0x33, LCD_CMD);
    lcd_byte(i2c, 0x32, LCD_CMD);
    lcd_byte(i2c, 0x28, LCD_CMD);
    lcd_byte(i2c, 0x0C, LCD_CMD);
    lcd_byte(i2c, 0x06, LCD_CMD);
    lcd_byte(i2c, 0x01, LCD_CMD);
    usleep(E_DELAY);
}

void lcd_clear(volatile uint32_t *i2c) {
    lcd_byte(i2c, 0x01, LCD_CMD);
    usleep(E_DELAY);
}

void lcd_string(volatile uint32_t *i2c, const char *message, uint8_t line) {
    char buffer[17];
    int len = strlen(message);

    strncpy(buffer, message, 16);
    buffer[16] = '\0';

    while (len < 16) {
        buffer[len] = ' ';
        len++;
    }
    buffer[16] = '\0';

    lcd_byte(i2c, line, LCD_CMD);

    for (int i = 0; i < 16 && buffer[i] != '\0'; i++) {
        lcd_byte(i2c, buffer[i], LCD_CHR);
    }
}

void blinkN(uint32_t *gpio, int led, int c) { 
  for (int i = 0; i < c; i++) {
    writeLED(gpio, led, HIGH);
    delay(DELAY);
    writeLED(gpio, led, LOW);
    delay(DELAY);
  }
}

int main (int argc, char *argv[]) {
  int found = 0, attempts = 0, i, code;
  int *attSeq;
  int pinLED = LED, pin2LED2 = LED2, pinButton = BUTTON;
  int fd ;
  int exact, contained;
  int verbose = 0, debug = 0, help = 0, opt_s = 0, unit_test = 0;

  // Process command-line arguments
  int opt;
  while ((opt = getopt(argc, argv, "hvdus:")) != -1) {
    switch (opt) {
      case 'v':
        verbose = 1;
        break;
      case 'h':
        help = 1;
        break;
      case 'd':
        debug = 1;
        break;
      case 'u':
        unit_test = 1;
        break;
      case 's':
        opt_s = atoi(optarg);
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u] [-s <secret seq>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (help) {
    fprintf(stderr, "MasterMind program, running on a Raspberry Pi, with connected LED, button and LCD display\n");
    fprintf(stderr, "Use the button for input of numbers. The LCD display will show the matches with the secret sequence.\n");
    fprintf(stderr, "For full specification of the program see: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf\n");
    fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u] [-s <secret seq>]\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  if (verbose) {
    fprintf(stdout, "Settings for running the program\n");
    fprintf(stdout, "Verbose is %s\n", (verbose ? "ON" : "OFF"));
    fprintf(stdout, "Debug is %s\n", (debug ? "ON" : "OFF"));
    fprintf(stdout, "Unittest is %s\n", (unit_test ? "ON" : "OFF"));
    if (opt_s) fprintf(stdout, "Secret sequence set to %d\n", opt_s);
  }

  if (unit_test) {
    if (optind >= argc - 1) {
      fprintf(stderr, "Expected 2 arguments after option -u\n");
      exit(EXIT_FAILURE);
    }
    int seq1[SEQL], seq2[SEQL];
    readSeq(seq1, atoi(argv[optind]));
    readSeq(seq2, atoi(argv[optind + 1]));
    int result = countMatches(seq1, seq2);
    showMatches(result, seq1, seq2, 0);
    exit(EXIT_SUCCESS);
  }

  // Ensure theSeq is allocated before use
  if (theSeq == NULL) {
    theSeq = (int*)malloc(seqlen * sizeof(int));
    if (theSeq == NULL) failure(TRUE, "Failed to allocate memory for theSeq\n");
  }

  // Validate and set secret sequence if -s is used
  if (opt_s) {
    readSeq(theSeq, opt_s);
  } else {
    initSeq(); // Generate random sequence if no -s
  }

  attSeq = (int*)malloc(seqlen * sizeof(int));
  if (attSeq == NULL) failure(TRUE, "Failed to allocate memory for attSeq\n");

  gpiobase = 0xFE200000;
  if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
    return failure(FALSE, "setup: Unable to open /dev/mem: %s\n", strerror(errno));

  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase);
  if ((int32_t)gpio == -1)
    return failure(FALSE, "setup: mmap (GPIO) failed: %s\n", strerror(errno));

  i2c = (uint32_t *)mmap(NULL, I2C_LEN, PROT_READ|PROT_WRITE, MAP_SHARED, fd, I2C_BASE);
  if ((int32_t)i2c == -1)
    return failure(FALSE, "setup: mmap (I2C) failed: %s\n", strerror(errno));

  // Debug output only after mmap
  if (debug) {
    fprintf(stderr, "Debug Mode Enabled:\n");
    fprintf(stderr, "GPIO Base: 0x%X\n", gpiobase);
    fprintf(stderr, "LED Pin: %d (Output), LED2 Pin: %d (Output), Button Pin: %d (Input)\n", pinLED, pin2LED2, pinButton);
    if (i2c) {
      fprintf(stderr, "I2C Address: 0x%X, Data Length: %d\n", I2C_ADDR, i2c[DLEN_OFFSET / 4]);
    } else {
      fprintf(stderr, "I2C not initialized\n");
    }
    fprintf(stderr, "Sequence Length: %d, Colors: %d\n", seqlen, colors);
  }

  pinMode(gpio, pinLED, OUTPUT);
  pinMode(gpio, pin2LED2, OUTPUT);
  pinMode(gpio, pinButton, INPUT);
  writeLED(gpio, pinLED, LOW);
  writeLED(gpio, pin2LED2, LOW);

  i2c[A_OFFSET / 4] = I2C_ADDR;
  i2c[DLEN_OFFSET / 4] = 1;
  lcd_init(i2c);

  lcd_clear(i2c);
  lcd_string(i2c, "MasterMind    ", LCD_LINE_1);
  lcd_string(i2c, "Press to run", LCD_LINE_2);

  char surname[] = "Nurad";
  for (i = 0; i < 5 && surname[i] != '\0'; i++) {
    if (surname[i] == 'a' || surname[i] == 'e' || surname[i] == 'i' || 
        surname[i] == 'o' || surname[i] == 'u') {
      blinkN(gpio, pinLED, 1);
    } else {
      blinkN(gpio, pin2LED2, 1);
    }
  }
  writeLED(gpio, pin2LED2, LOW);

  writeLED(gpio, pinLED, HIGH);
  waitForButton(gpio, pinButton);
  writeLED(gpio, pinLED, LOW);
  blinkN(gpio, pin2LED2, 1);
  writeLED(gpio, pin2LED2, LOW);

  lcd_clear(i2c);
  lcd_string(i2c, "Enter guess   ", LCD_LINE_1);
  lcd_string(i2c, "Guess: 0 0 0  ", LCD_LINE_2);
  fprintf(stderr, "Game started!\n");

  while (!found) {
    attempts++;
    if (debug || verbose) {
      fprintf(stderr, "Attempt #%d started\n", attempts);
    }

    for (i = 0; i < seqlen; i++) {
        int presses = 0;
        initITimer(TIMEOUT);
        if (debug) fprintf(stderr, "Debug: Waiting for input at position %d\n", i+1);
        while (!timed_out) {
            if (readButton(gpio, pinButton) == HIGH && presses < 3) {
                presses++;
                if (debug) fprintf(stderr, "Debug: Button pressed %d times at position %d\n", presses, i+1);
                writeLED(gpio, pinLED, HIGH);
                delay(DELAY);
                while (readButton(gpio, pinButton) == HIGH) delay(10);
                writeLED(gpio, pinLED, LOW);
                char guess[17];
                sprintf(guess, "Guess: %d %d %d  ", 
                        i == 0 ? presses : attSeq[0], 
                        i == 1 ? presses : (i > 1 ? attSeq[1] : 0), 
                        i == 2 ? presses : 0);
                lcd_clear(i2c);
                lcd_string(i2c, "Enter guess   ", LCD_LINE_1);
                lcd_string(i2c, guess, LCD_LINE_2);
                initITimer(TIMEOUT);
            }
            delay(10);
        }
        if (presses > 3) presses = 3;
        attSeq[i] = presses;
        if (debug) fprintf(stderr, "Debug: Input for position %d: %d\n", i+1, presses);
        char final_guess[17];
        sprintf(final_guess, "Guess: %d %d %d  ", 
                attSeq[0], 
                i >= 1 ? attSeq[1] : 0, 
                i >= 2 ? attSeq[2] : 0);
        lcd_clear(i2c);
        lcd_string(i2c, "Enter guess   ", LCD_LINE_1);
        lcd_string(i2c, final_guess, LCD_LINE_2);
        blinkN(gpio, pin2LED2, 1);
        writeLED(gpio, pin2LED2, LOW);
        blinkN(gpio, pinLED, presses);
    }
    blinkN(gpio, pin2LED2, 2);
    writeLED(gpio, pin2LED2, LOW);

    code = countMatches(theSeq, attSeq);
    exact = code / 10;
    contained = code % 10;

    blinkN(gpio, pinLED, exact);
    blinkN(gpio, pin2LED2, 1);
    writeLED(gpio, pin2LED2, LOW);
    blinkN(gpio, pinLED, contained);

    showMatches(code, theSeq, attSeq, 1);

    if (debug || verbose) {
      fprintf(stderr, "Attempt %d result - %d exact, %d approximate\n", attempts, exact, contained);
      fprintf(stderr, "Current guess: ");
      for (i = 0; i < seqlen; i++) {
        fprintf(stderr, "%d ", attSeq[i]);
      }
      fprintf(stderr, "\n");
    }

    if (exact == seqlen) {
      found = 1;
    } else {
      blinkN(gpio, pin2LED2, 3);
      writeLED(gpio, pin2LED2, LOW);
      lcd_clear(i2c);
      lcd_string(i2c, "Try again     ", LCD_LINE_1);
      lcd_string(i2c, "Guess: 0 0 0  ", LCD_LINE_2);
    }
  }
  if (found) {
    writeLED(gpio, pin2LED2, HIGH);
    blinkN(gpio, pinLED, 3);
    lcd_clear(i2c);
    char success_msg[17];
    sprintf(success_msg, "SUCCESS %d    ", attempts);
    lcd_string(i2c, "MasterMind    ", LCD_LINE_1);
    lcd_string(i2c, success_msg, LCD_LINE_2);
    if (debug || verbose) fprintf(stderr, "Game won after %d attempts\n", attempts);
  }
  if (theSeq) free(theSeq);
  free(attSeq);
  if (gpio && gpio != MAP_FAILED) munmap(gpio, BLOCK_SIZE);
  if (i2c && i2c != MAP_FAILED) munmap(i2c, I2C_LEN);
  close(fd);
  return 0;
}
