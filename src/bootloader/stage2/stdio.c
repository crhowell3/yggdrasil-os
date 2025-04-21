#include "stdio.h"
#include "x86.h"

void putc(char c) {
    x86_Video_WriteCharTeletype(c, 0);
}

void puts(const char* str) {
    while(*str) {
        putc(*str);
        str++;
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

int* printf_number(int* argp, int length, bool sign, int radix);

void _cdecl printf(const char* fmt, ...) {
    int* argp = (int*)&fmt;
    int state = kPrintfStateNormal;
    int length = kPrintfLengthDefault;
    int radix = 10;
    bool sign = false;

    argp++;

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
                switch (*fmt)
                {
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
                        putc((char)*argp);
                        argp++;
                        break;
                    case 's': 
                        puts(*(char**)argp);
                        argp++;
                        break;
                    case '%':
                        putc('%');
                        break;
                    case 'd':
                    case 'i':
                        radix = 10;
                        sign = true;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    case 'u':
                        radix = 10;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    case 'X':
                    case 'x':
                    case 'p':
                        radix = 16;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    case 'o':
                        radix = 8;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    default:
                        break;
                }
                state = kPrintfStateNormal;
                length = kPrintfLengthDefault;
                radix = 10;
                sign = false;
                break;
        }
        fmt++;
    }
}

const char kHexChars[] = "0123456789abcdef";

int* printf_number(int* argp, int length, bool sign, int radix) {
    char buffer[32];
    unsigned long long number;
    int number_sign = 1;
    int pos = 0;

    switch (length) {
        case kPrintfLengthShortShort:
        case kPrintfLengthShort:
        case kPrintfLengthDefault:
            if (sign) {
                int n = *argp;
                if (n < 0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            } else {
                number = *(unsigned int*)argp;
            }
            argp++;
            break;
        case kPrintfLengthLong:
            if (sign) {
                long int n = *(long int*)argp;
                if (n < 0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            } else {
                number = *(unsigned long int*)argp;
            }
            argp += 2;
            break;
        case kPrintfLengthLongLong:
                    if (sign) {
                long long int n = *(long long int*)argp;
                if (n < 0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            } else {
                number = *(unsigned long long*)argp;
            }
            argp += 2;
            break;
    }

    // Convert number to ASCII
    do {
        uint32_t rem;
        x86_div64_32(number, radix, &number, &rem);
        buffer[pos++] = kHexChars[rem];
    } while (number > 0);

    if (sign && number_sign < 0) {
        buffer[pos++] = '-';
    }

    while (--pos >= 0) {
        putc(buffer[pos]);
    }

    return argp;
}
