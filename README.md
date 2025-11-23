# Raspberry Pi Pico project for the course Computer Systems 521286A-3006

## Group Members:
## Hoang Bach
## Pedro Setti
## Piero Cianciotta


![State Machine][https://github.com/P13r00/JTKJ-PicoRTOS-Project/blob/main/state_machine_diagram.jpg]


## H2 main.c Function Documentation


**static void append_to_string(char \*message, char symbol)**
Appends a single character (symbol) to the end of a C-string stored in message.

Parameters:
message – pointer to a null-terminated char buffer
symbol – the character to append


**static void hsv_to_rgb(float h, float s, float v, uint8_t \*out_r, uint8_t \*out_g, uint8_t \*out_b)**
Converts a color in HSV space to RGB (0–255 range). Used for cycling LED rainbow colors.

Parameters:
h hue in degrees [0,360]
s saturation [0,1]
v brightness [0,1]
out_r, out_g, out_b – output pointers to store the RGB values


**static void button_function(uint gpio, uint32_t eventMask)**
GPIO interrupt callback for BUTTON1 and BUTTON2.
Changes the global programState depending on current state and which button was pressed.
Parameters:
gpio – which button triggered
eventMask – edge type (rising edge)


**static void symbolDetectionTask(void \*pvParameters)**
task that reads accelerometer/gyroscope data and translates movements into Morse symbols:
gx > threshold → dot (.)
gx < -threshold → dash (-)
gy movement → space ( )
three consecutive spaces → end of message & switch to DISPLAYING

Modifies current_message
Uses simple filtering on gx and gy
Exits loop when state changes


**static void displayTask(void \*pvParameters)**
Handles both displaying messages and converting Morse ↔ text
Initialize display + I2C (super_init() + init_display())
If message begins with . or -: decode Morse
Otherwise: encode text into Morse
Display the translated result
Play a notification sound
Reset sensors and return to WAITING

Uses morse_to_string() and string_to_morse()
Frees allocated translation buffer
Clears display afterwards
Handles workstation messages and sensor messages equally


**static void msgFromWorkstationTask(void \*pvParameters)**
Reads characters sent over USB (stdio), assembles them into a message until newline, and triggers displaying.

Reads characters via getchar_timeout_us()
Stores input into current_message
On newline (\n) → switch to DISPLAYING state

Includes buffer overflow handling
Clears message at the start of state


**static void rgbledTask(void \*pvParameters)**
Controls the RGB LED based on system state:
State	LED Behavior
WAITING	Rainbow cycle / animation
SYMBOL_DETECTION	Blue
MSG_FROM_WORKSTATION	Yellow
DISPLAYING	Green


## morse_lib.c and buzdata.c Function Documentation

**void sing(int notes[][2]);**
Plays a sequence of musical tones using the buzzer.
Each entry in notes contains:
notes[j][0]: frequency
notes[j][1]: duration
The sequence ends when a note has duration 0.


**char\* morse_to_string(const char \*morse);**
Converts a Morse-coded string into normal text.

Allocates a dynamically sized output buffer with malloc.
Reads dots (.), dashes (-), and spaces to identify symbols.
Matches each Morse symbol with entries in morse_table.
Unrecognized patterns become '?'.
If TERMINATOR_LENGTH consecutive spaces are detected, appends [END] and stops.
Returns a heap-allocated string (must be freed by the caller).


**char\* string_to_morse(const char \*str);**
Converts a normal ASCII string to Morse code.
Allocates a result buffer based on maximum Morse symbol length.
Converts letters to uppercase before lookup.
Looks up each character in the morse_table array.
Adds a space after each encoded symbol.
If a character is not found, appends: "?" TERMINATOR_LENGTH spaces
Adds five trailing spaces at the end (protocol terminator).
Returns a heap-allocated string (must be freed by the caller).
