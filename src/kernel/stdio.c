#include "stdio.h"
#include "arch/i686/io.h"

#include <stdarg.h>
#include <stdbool.h>

const unsigned kScreenWidth = 80;
const unsigned kScreenHeight = 25;
const uint8_t kDefaultColor = 0x7;

uint8_t *screen_buffer_ = (uint8_t *)0xB8000;
int screen_x_ = 0;
int screen_y_ = 0;

void putchr(int x, int y, char c) {
  screen_buffer_[2 * (y * kScreenWidth + x)] = c;
}

void putcolor(int x, int y, uint8_t color) {
  screen_buffer_[2 * (y * kScreenWidth + x) + 1] = color;
}

char getchr(int x, int y) { return screen_buffer_[2 * (y * kScreenWidth + x)]; }

uint8_t getcolor(int x, int y) {
  return screen_buffer_[2 * (y * kScreenWidth + x) + 1];
}

void setcursor(int x, int y) {
  int pos = y * kScreenWidth + x;

  x86_outb(0x3D4, 0x0F);
  x86_outb(0x3D5, (uint8_t)(pos & 0xFF));
  x86_outb(0x03D4, 0x0E);
  x86_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clrscr() {
  for (int y = 0; y < kScreenHeight; y++) {
    for (int x = 0; x < kScreenWidth; x++) {
      putchr(x, y, '\0');
      putcolor(x, y, kDefaultColor);
    }
  }

  screen_x_ = 0;
  screen_y_ = 0;
  setcursor(screen_x_, screen_y_);
}

void scrollback(int lines) {
  for (int y = lines; y < kScreenHeight; y++) {
    for (int x = 0; x < kScreenWidth; x++) {
      putchr(x, y - lines, getchr(x, y));
      putcolor(x, y - lines, getcolor(x, y));
    }
  }

  for (int y = kScreenHeight - lines; y < kScreenHeight; y++) {
    for (int x = 0; x < kScreenWidth; x++) {
      putchr(x, y, '\0');
      putcolor(x, y, kDefaultColor);
    }
  }

  screen_y_ -= lines;
}

void putc(char c) {
  switch (c) {
  case '\n':
    screen_x_ = 0;
    screen_y_++;
    break;

  case '\t':
    for (int i = 0; i < 4 - (screen_x_ % 4); i++) {
      putc(' ');
    }
    break;

  case '\r':
    screen_x_ = 0;
    break;

  default:
    putchr(screen_x_, screen_y_, c);
    screen_x_++;
    break;
  }

  if (screen_x_ >= kScreenWidth) {
    screen_y_++;
    screen_x_ = 0;
  }
  if (screen_y_ >= kScreenHeight) {
    scrollback(1);
  }

  setcursor(screen_x_, screen_y_);
}

void puts(const char *str) {
  while (*str) {
    putc(*str);
    str++;
  }
}

const char kHexChars[] = "0123456789abcdef";

void printf_unsigned(unsigned long long number, int radix) {
  char buffer[32];
  int pos = 0;

  do {
    unsigned long long rem = number % radix;
    number /= radix;
    buffer[pos++] = kHexChars[rem];
  } while (number > 0);

  while (--pos >= 0) {
    putc(buffer[pos]);
  }
}

void printf_signed(long long number, int radix) {
  if (number < 0) {
    putc('-');
    printf_unsigned(-number, radix);
  } else {
    printf_unsigned(number, radix);
  }
}

enum PrintfState {
  kPrintfStateNormal,
  kPrintfStateLength,
  kPrintfStateLengthShort,
  kPrintfStateLengthLong,
  kPrintfStateSpec
};

enum PrintfLength {
  kPrintfLengthDefault,
  kPrintfLengthShortShort,
  kPrintfLengthShort,
  kPrintfLengthLong,
  kPrintfLengthLongLong
};

void printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int state = kPrintfStateNormal;
  int length = kPrintfLengthDefault;
  int radix = 10;
  bool sign = false;
  bool number = false;

  while (*fmt) {
    switch (state) {
    case kPrintfStateNormal:
      switch (*fmt) {
      case '%':
        state = kPrintfStateLength;
        break;
      default:
        putc(*fmt);
        break;
      }
      break;
    case kPrintfStateLength:
      switch (*fmt) {
      case 'h':
        length = kPrintfLengthShort;
        state = kPrintfStateLengthShort;
        break;
      case 'l':
        length = kPrintfLengthLong;
        state = kPrintfStateLengthLong;
        break;
      default:
        goto PRINTF_STATE_SPEC_;
      }
      break;
    case kPrintfStateLengthShort:
      if (*fmt == 'h') {
        length = kPrintfLengthShortShort;
        state = kPrintfStateSpec;
      } else {
        goto PRINTF_STATE_SPEC_;
      }
      break;
    case kPrintfStateLengthLong:
      if (*fmt == 'l') {
        length = kPrintfLengthLongLong;
        state = kPrintfStateSpec;
      } else {
        goto PRINTF_STATE_SPEC_;
      }
      break;
    case kPrintfStateSpec:
    PRINTF_STATE_SPEC_:
      switch (*fmt) {
      case 'c':
        putc((char)va_arg(args, int));
        break;
      case 's':
        puts(va_arg(args, const char *));
        break;
      case '%':
        putc('%');
        break;
      case 'd':
      case 'i':
        radix = 10;
        sign = true;
        number = true;
        break;
      case 'u':
        radix = 10;
        sign = false;
        number = true;
        break;
      case 'X':
      case 'x':
      case 'p':
        radix = 16;
        sign = false;
        number = true;
        break;
      case 'o':
        radix = 8;
        sign = false;
        number = true;
        break;
      default:
        break;
      }

      if (number) {
        if (sign) {
          switch (length) {
          case kPrintfLengthShortShort:
          case kPrintfLengthShort:
          case kPrintfLengthDefault:
            printf_signed(va_arg(args, int), radix);
            break;
          case kPrintfLengthLong:
            printf_signed(va_arg(args, long), radix);
            break;
          case kPrintfLengthLongLong:
            printf_signed(va_arg(args, long long), radix);
            break;
          }
        } else {
          switch (length) {
          case kPrintfLengthShortShort:
          case kPrintfLengthShort:
          case kPrintfLengthDefault:
            printf_unsigned(va_arg(args, unsigned int), radix);
            break;
          case kPrintfLengthLong:
            printf_unsigned(va_arg(args, unsigned long), radix);
            break;
          case kPrintfLengthLongLong:
            printf_unsigned(va_arg(args, unsigned long long), radix);
            break;
          }
        }
      }
      state = kPrintfStateNormal;
      length = kPrintfLengthDefault;
      radix = 10;
      sign = false;
      break;
    }

    fmt++;
  }
  va_end(args);
}
