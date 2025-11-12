#include "morse_lib.h"

const MorseMap morse_table[] = {
    {'A', ".-"},    {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
    {'E', "."},     {'F', "..-."}, {'G', "--."},  {'H', "...."},
    {'I', ".."},    {'J', ".---"}, {'K', "-.-"},  {'L', ".-.."},
    {'M', "--"},    {'N', "-."},   {'O', "---"},  {'P', ".--."},
    {'Q', "--.-"},  {'R', ".-."},  {'S', "..."},  {'T', "-"},
    {'U', "..-"},   {'V', "...-"}, {'W', ".--"},  {'X', "-..-"},
    {'Y', "-.--"},  {'Z', "--.."},
    {'0', "-----"}, {'1', ".----"},{'2', "..---"},{'3', "...--"},
    {'4', "....-"},{'5', "....."}, {'6', "-...."},{'7', "--..."},
    {'8', "---.."},{'9', "----."},
    {' ', "/"}
}; //array of letter and morse correspective type MorseMap (defined in morse_lib.h)

const size_t MORSE_TABLE_SIZE = sizeof(morse_table)/sizeof(MorseMap);

char* morse_to_string(const char *morse) {
    //save size for result
    char *result = malloc(strlen(morse) * 4 + 1);
    if (!result) return NULL;
    result[0] = '\0';

    char current_symbol[MAX_MORSE_SYMBOL + 1];
    int symbol_index = 0;
    int num_spaces = 0;
    int i = 0;

    //for each character
    while (i <= strlen(morse)) {
        char current_char = morse[i];

        //make symbol
        if (current_char == '.' || current_char == '-') {
            current_symbol[symbol_index] = current_char;
            symbol_index = symbol_index + 1;
            num_spaces = 0;
        }
        else { //move to decoding when space
            current_symbol[symbol_index] = '\0';
            symbol_index = 0;

            if (current_symbol[0] != '\0') {
                char decoded_letter = '?';
                int j = 0;

                //find in table
                for (j = 0; j < MORSE_TABLE_SIZE; j++) {
                    if (strcmp(current_symbol, morse_table[j].morse) == 0) {
                        decoded_letter = morse_table[j].character;
                        break;
                    }
                }

                //add letter to result, if error add ?
                if (decoded_letter == '?') {
                    int result_len = strlen(result);
                    result[result_len] = '?';
                    result[result_len + 1] = '\0';
                } else {
                    int result_len = strlen(result);
                    result[result_len] = decoded_letter;
                    result[result_len + 1] = '\0';
                }
            }

            num_spaces = num_spaces + 1;
            if (num_spaces == TERMINATOR_LENGTH) {
                strcat(result, "[END]");
                break;
            }
        }
        i++;
    }
    return result;
}


char* string_to_morse(const char *str) {
    //save size for result
    char *result = malloc(strlen(str) * MAX_MORSE_SYMBOL + 1 + 5);
    if (!result) return NULL;
    result[0] = '\0';

    //for each character
    int i = 0;
    while (str[i] != '\0') {
        //upper case
        char current_char = str[i];
        char upper_char = toupper(current_char);
        
        int found_char = 0;
        int j = 0;

        //find in table
        for (j = 0; j < MORSE_TABLE_SIZE; j++) {
            if (morse_table[j].character == upper_char) {
                //add symbol
                strcat(result, morse_table[j].morse);
                //add space after tramslation
                strcat(result, " ");
                found_char = 1;
                break;
            }
        }

        //If character not found in table, add ? marker and TERMINATOR_LENGTH spaces
        //dont understand????
        if (found_char == 0) {
            strcat(result, "?");
            for (int s = 0; s < TERMINATOR_LENGTH; s++) {
            strcat(result, " ");
            }
        }
        i++;
    }

    // append five spaces at the end as requested
    strcat(result, "     ");
    return result;
}
