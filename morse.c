/*
  Simple morse code beeper.

  Principally developed for a headless computer running on a mobile robot, it can beep me messages
  about status etc.

  Designed to work with a passive buzzer strapped across (GPIO 10) header pins 19 & 20 but that can be changed
  using program switches
  
  =====
  [3v3]   1 2   [5V]
  [GPIO 2 (SDA)]   3 4   [5V]
  [GPIO 3 (SCL)]   5 6   [Gnd]
  [GPIO 4 GPCLK0)]   7 8   [GPIO 14 (UART TX)]
  [Gnd]   9 10  [GPIO 15 (UART RX)]
  [GPIO 17]  11 12  [GPIO 18 (PCM CLK)]
  [GPIO 27]  13 14  [Gnd]
  [GPIO 22]  15 16  [GPIO 23]
  [3v3]  17 18  [GPIO 24]
  Buzzer [GPIO 10 (SPI0 MOSI)]  19 20  [Gnd] Buzzer
  =====
  [GPIO 9 (SPI0 MISO)]  21 22  [GPIO 25]
  [GPIO 11 (SPI0 SCLK)]  23 24  [GPIO 8 (SPI0 CE0)]
  [Gnd]  25 26  [GPIO 7 (SPI0 CE1)]
  [GPIO 0 (EEPROM SDA)]  27 28  [GPIO 1 (EEPROM SCL)]
  [GPIO 5]  29 30  [Gnd]
  [GPIO 6]  31 32  [GPIO 12 (PWM)]
  [GPIO 13 (PWM1)]  33 34  [Gnd]
  [GPIO 19 (PCM FS)]  35 36  [GPIO 16]
  [GPIO 26]  37 38  [GPIO 20 (PCM DIN)]
  [Gnd]  39 40  [GPIO 21 (PCM DOUT)]

*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <gpiod.h>

#define GPIO 10
#define HZ  800
#define WPM 18

#define FALSE 0
#define TRUE 1;

static int verbose=FALSE;

static int gpio = GPIO;
static int freq  = HZ;
static unsigned int wpm = WPM;

static int dit;

static char *chars[] = {
  "A.-",
  "B-...",
  "C-.-.",
  "D-..",
  "E.",
  "F..-.",
  "G--.",
  "H....",
  "I..",
  "J.---",
  "K-.-",
  "L.-..",
  "M--",
  "N-.",
  "O---",
  "P.--.",
  "Q--.-",
  "R.-.",
  "S...",
  "T-",
  "U..--",
  "V...-",
  "W.--",
  "X-..-",
  "Y-.--",
  "Z--..",
  "1.----",
  "2..---",
  "3...--",
  "4....-",
  "5.....",
  "6-....",
  "7--...",
  "8---..",
  "9----.",
  "0-----",
  "..-.-.-",
  ",--..--",
  "?..--..",
  "'.----.",
  "!-.-.--",
  "/-..-.",
  "(-.--.",
  ")-.--.-",
  "&.-...",
  ":---...",
  ";-.-.-.",
  "=-...-",
  "+.-.-.",
  "--....-",
  "_..--.-"
};
  
void fatal(char *fmt, ...) {
  char buf[256];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  fprintf(stderr, "%s\n", buf);

  exit(EXIT_FAILURE);
}


void usage(char *argv[]) {
  fprintf(stderr, "\n" \
          "Usage: %s [OPTION] ...\n"   \
          "   -g value, gpio, 0-31       (%d)\n"   \
          "   -h value, Frequency in Hz  (%d)\n"   \
          "   -v enable verbose mode\n"             \
          "   -w value, Words per minute (%d)\n"  \
          "EXAMPLE\n"                          \
          "echo \"CQ\" | %s -w 10\n"              \
          "    Send CQ at 10 words per minute\n"  \
          "\n", argv[0], GPIO, HZ, WPM, argv[0]);
}

static void initOpts(int argc, char *argv[]) {
  int i, opt;

  while ((opt = getopt(argc, argv, "h:g:vw:")) != -1) {
    i = -1;

    switch (opt) {
    case 'h':
      i = atoi(optarg);
      if (i >= 1) freq = i;
      else fatal("invalid -h option (%d)", i);
      break;
    case 'g':
      i = atoi(optarg);
      if ((i >= 1) && (i <= 31)) gpio = i;
      else fatal("invalid -g option (%d)", i);
      break;
    case 'v' :
      verbose=TRUE;
      break;
    case 'w':
      i = atoi(optarg);
      if (i >= 1) wpm = i;
      else fatal("invalid -w option (%d)", i);
      break;
    default: /* '?' */
      usage(argv);
      exit(EXIT_FAILURE);
    }
  }
}

void beep(struct gpiod_line *line, unsigned long length) {
  // length is the time in uSec 
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  uint64_t now_time=(now.tv_sec - start.tv_sec) * 1000000 + (now.tv_nsec - start.tv_nsec) / 1000;
  uint64_t end_time=now_time + length;
  
  int bit=0;
  while (end_time > now_time) {
    gpiod_line_set_value(line, bit&1);
    bit++;
    usleep(500000/freq);  // half a second divided by Hz gives the toggle rate
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    now_time=(now.tv_sec - start.tv_sec) * 1000000 + (now.tv_nsec - start.tv_nsec) / 1000;  
  } 
}


int main(int argc, char *argv[])
{
  struct gpiod_chip *chip;
  struct gpiod_line *line;
  int ret;
   
  initOpts(argc, argv);
  
  dit=(1200/wpm) * 1000; // uSec
   
  chip=gpiod_chip_open_by_label("pinctrl-bcm2835");
  if (chip==NULL) {
    perror("Opening GPIO chip 'pinctrl-bcm2835'");
    exit(errno);
  }

  line = gpiod_chip_get_line(chip, gpio);
  if (!line) {
    char msg[50];
    sprintf(msg,"Failed to allocate BCM%d", gpio);
    perror(msg);
    gpiod_chip_close(chip);
    exit(errno);
  }

  ret = gpiod_line_request_output(line, "Buzzer", 0);
  if (ret < 0) {
    char msg[50];
    sprintf(msg,"Request for line BCM%d as output failed", gpio);
    perror(msg);
    gpiod_line_release(line);
    gpiod_chip_close(chip);
    exit(errno);
  }

  if (verbose)
    printf("Chars has %ld entries\n",sizeof(chars)/sizeof(chars[0]));
   
  char c;
  while ((c=getc(stdin)) != 0xFF ) {
    c=toupper(c);
    if (verbose)
      printf("%c 0x(%02X) ", c, c);
    if (isspace(c)) {
      usleep(dit*7);
    } else {
      for (int i=0; i < sizeof(chars)/sizeof(chars[0]); i++) {
        if (c == *chars[i]) {
          for (int j=1; *(chars[i]+j) != 0; j++) {
            if (verbose)
              printf("%c", *(chars[i]+j));
             
            if (*(chars[i]+j) == '.')
              beep(line, dit);
            else
              beep(line, dit*3);

            usleep(dit);
          }
          usleep(dit*2);  // We waited 1 dit above
          break;
        }
      }
    }
    if (verbose)
      printf("\n");
  }
 
  gpiod_line_release(line);
  gpiod_chip_close(chip);
   
  return 0;
}
