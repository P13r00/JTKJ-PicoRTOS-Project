#ifndef MORSE_LIB_H
#define MORSE_LIB_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_MORSE_SYMBOL 6 //long lenght for safety
#define TERMINATOR_LENGTH 5 //length of terminator "[END]"

typedef struct {
    char character;
    const char *morse;
} MorseMap;

// Morse alphabet table
extern const MorseMap morse_table[];
extern const size_t MORSE_TABLE_SIZE;

// Function prototypes
char* morse_to_string(const char *morse);
char* string_to_morse(const char *str);

#endif
