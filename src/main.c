#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include "tkjhat/sdk.h"
#include "buzdata.h"
#include "morse_lib.h"

#define DEFAULT_STACK_SIZE 2048
#define THRESHOLD          100 //for sensor data filtering
#define INPUT_BUFFER_SIZE 100 //-piero: added as a global variable

//states -piero: update incrementally following new functionalities
enum state { WAITING=0,             //await input from sensor or workstation
             SYMBOL_DETECTION=1,    //read symbols from sensor
             MSG_FROM_WORKSTATION=2,  //??notneeded?? sending the message to the workstation
             DISPLAYING=3,          //displaying the message, providing audio feedback
             //DECODING=3,            //from Morse -> Characters
             //ENCODING=4,            //from Characters -> Morse
            };
enum state programState = WAITING;

//global variables
char current_message[INPUT_BUFFER_SIZE] = {0}; //current message -piero: decreased size
float ax = 0.0, ay = 0.0, az = 0.0, gx = 0.0, gy = 0.0, gz = 0.0, t = 0.0;
static float last_printed_gx = 0.0f;
static int first_gx = 0;
static float last_printed_gy = 0.0f;
static int first_gy = 0;

static int melody[][2] = {
    {NOTE_C6, 200}, {NOTE_E6, 200}, {NOTE_G6, 200}, {NOTE_C7, 400},
    {REST, 0}
};

static int ring[][2] = {
    {NOTE_E6, 100}, {NOTE_D6, 100}, {NOTE_FS5, 200}, {NOTE_GS5, 200}, {REST, 50}, {NOTE_CS6, 100}, {NOTE_B6, 100}, {NOTE_D5, 200}, {NOTE_E5, 200}, {REST, 50}, {NOTE_B6, 100}, {NOTE_A6, 100}, {NOTE_CS5, 200}, {NOTE_E5, 200}, {NOTE_A5, 200},
    {REST, 0}
};

static void button_function(uint gpio, uint32_t eventMask); //-piero: REMEMBBER TO ADD ALL PROTOTYPES
static void append_to_string(char *message, char symbol);
static void space_key(uint gpio, uint32_t eventMask);
static void symbolDetectionTask(void *pvParameters);
static void displayingTask(void *pvParameters);
static void msgToWorkstationTask(void *pvParameters);
static void super_init();
static void state_change();
static void reset_screen();
static void rgbledTask(void *pvParameters);

/*static void reset_screen(){
    memset(current_message, 0, INPUT_BUFFER_SIZE); //clear the message buffer
    //reset other variables
    i2c_deinit(i2c_default);
}*/



// Simple HSV to RGB conversion. h in [0,360), s,v in [0,1]
static void hsv_to_rgb(float h, float s, float v, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b) {
    float c = v * s;
    float hh = h / 60.0f;
    float x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
    float r1 = 0, g1 = 0, b1 = 0;
    if (0.0f <= hh && hh < 1.0f) { r1 = c; g1 = x; b1 = 0; }
    else if (1.0f <= hh && hh < 2.0f) { r1 = x; g1 = c; b1 = 0; }
    else if (2.0f <= hh && hh < 3.0f) { r1 = 0; g1 = c; b1 = x; }
    else if (3.0f <= hh && hh < 4.0f) { r1 = 0; g1 = x; b1 = c; }
    else if (4.0f <= hh && hh < 5.0f) { r1 = x; g1 = 0; b1 = c; }
    else if (5.0f <= hh && hh < 6.0f) { r1 = c; g1 = 0; b1 = x; }
    float m = v - c;
    uint8_t r = (uint8_t)roundf((r1 + m) * 255.0f);
    uint8_t g = (uint8_t)roundf((g1 + m) * 255.0f);
    uint8_t b = (uint8_t)roundf((b1 + m) * 255.0f);
    *out_r = r; *out_g = g; *out_b = b;
}

static void rgbledTask(void *pvParameters){
    //Implement RGB LED feedback for different states, rainbow for waiting state,
    // solid colors for others
    // Bach: should be run according to the states
    static float hue = 0.0f; // 0..360
    //loop
    for(;;){
        if (programState == WAITING) {
            // Cycle rainbow colors by advancing hue
            uint8_t r, g, b;
            hsv_to_rgb(hue, 1.0f, 0.5f, &r, &g, &b); // medium brightness
            rgb_led_write(r, g, b);
            hue += 1.0f; // increment hue each call
            if (hue >= 360.0f) hue -= 360.0f;
        } else if (programState == SYMBOL_DETECTION) {
            // Set LED to blue
            rgb_led_write(0, 0, 255);
        } else if (programState == MSG_FROM_WORKSTATION) {
            // Set LED to yellow
            rgb_led_write(255, 255, 0);
        } else if (programState == DISPLAYING) {
            // Set LED to green
            rgb_led_write(0, 255, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


static void button_function(uint gpio, uint32_t eventMask) { 
    //-piero: removed change state function, everything is working directly
    // bach: added "whatifs" for rgb change, if rgbledtask takes too much time to fix, uncomment rgb_led_write's here to implement leds.
    if (gpio == BUTTON1) {
        if(programState == WAITING){
            programState = SYMBOL_DETECTION;
            // rgb_led_write(0, 0, 255); //blue for symbol detection
            printf("Current State: SYMBOL_DETECTION\n");
        } else {
            programState = DISPLAYING;
            // rgb_led_write(0, 255, 0); //green for displaying state
            printf("Current State: DISPLAYING\n");
        }
    } else if (gpio == BUTTON2) {
        if(programState == WAITING){
            programState = MSG_FROM_WORKSTATION;
            // rgb_led_write(255, 255, 0); //yellow for msg from workstation
            printf("Current State: MSG_FROM_WORKSTATION\n");
        } else {
            programState = DISPLAYING;
            // rgb_led_write(0, 255, 0); //green for displaying state
            //printf("Current State: DISPLAYING\n");
        }
    }
}

static void append_to_string(char *message, char symbol) {
    // append symbol to current_message string
    while (*message++);
    *(message - 1) = symbol;
    *message = '\0';
}

static void symbolDetectionTask(void *pvParameters) { //-piero: ples keep indentation tidy
    int space_count = 0;
    while(1){
        if(programState == SYMBOL_DETECTION){
            memset(current_message, 0, INPUT_BUFFER_SIZE); //clear the message buffer
            space_count = 0;
            for (;;) {
                if (programState != SYMBOL_DETECTION){
                    break;
                }
                if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) != 0) {
                    printf("Sensor read failed:");
                    sleep_ms(200);
                    continue;
                }

                if (!(gx >= THRESHOLD || gx <= -THRESHOLD)) {
                    gx = 0;
                }
                float diff_gx = gx - last_printed_gx;
                if (diff_gx < 0.0f) diff_gx = -diff_gx;
                if (first_gx || diff_gx >= 20.0f) {
                    if(gx >= THRESHOLD) {
                        append_to_string(current_message, '.'); //down
                        printf(".\n");
                        space_count = 0;
                    } else if (gx <= -THRESHOLD) {
                        append_to_string(current_message, '-'); //up
                        printf("-\n");
                        space_count = 0;
                    }
                    last_printed_gx = gx;
                    first_gx = 0;
                }

                if (!(gy >= 150 || gy <= -150)) { //review values???
                    gy = 0;
                }
                float diff_gy = gy - last_printed_gy;
                if (diff_gy < 0.0f) diff_gy = -diff_gy;
                if (first_gy || diff_gy >= 50.0f) {
                    if (gy >= 150 || gy <= -150) {
                        append_to_string(current_message, ' '); //space on gy event
                        printf("Space\n");
                        space_count++;
                    }
                    if (space_count >= 3) {
                        space_count = 0;
                        printf("Detected 3 consecutive spaces: Ending message.\n");
                        programState = DISPLAYING;
                        printf("Current State: DISPLAYING\n");
                        sleep_ms(200);
                        break;
                    }
                    last_printed_gy = gy;
                    first_gy = 0;
                } 
                sleep_ms(100);
            }
        }
    }
}

static void displayTask(void *pvParameters) { //unified displaytask with msg to workstation
    while(1){
        //printf("1\n");
        if(programState == DISPLAYING){
            if (programState != DISPLAYING){
                    break;
                }

            super_init();
            init_display();
            
            if (current_message[0] == '\0') {
                printf("No message detected\n");
                programState = WAITING;
                printf("Current State: WAITING\n");
            } else {

            char *translated_message = morse_to_string(current_message); // Pointer to the dynamically allocated result

            printf("Message: %s\n", translated_message);
            if(current_message[0] != '\0') 
                sing(ring);


            vTaskDelay(pdMS_TO_TICKS(100));
            write_text_xy(0, 0, translated_message);
            sleep_ms(500); //add so that it doesnt break on button press before
            clear_display();

            free(translated_message); // Free the dynamically allocated memory

            memset(current_message, 0, INPUT_BUFFER_SIZE); //clear the message buffer

            super_init(); //reset i2c and display
            init_ICM42670();
            ICM42670_start_with_default_values();
            sleep_ms(100);

            programState = WAITING;
            printf("Current State: WAITING\n");
            }
        }
    }
}

static void msgFromWorkstationTask(void *pvParameters) { //-piero: implemented from code in examples, allocated right stack size, TODO: more error Handling
    (void)pvParameters;
    memset(current_message, 0, INPUT_BUFFER_SIZE); //clear the message buffer
    size_t index = 0;

    while (1){
        if(programState == MSG_FROM_WORKSTATION){
            if (programState != MSG_FROM_WORKSTATION){
                    break;
                }
            // take one char per time and store it in line array, until reeceived the \n
            // The application should instead play a sound.
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT){// I have received a character
                if (c == '\r') continue; // ignore CR, wait for LF if (ch == '\n') { line[len] = '\0';
                if (c == '\n'){
                    // terminate and process the collected line
                    current_message[index] = '\0'; 
                    //printf("__[RX]:\"%s\"__\n", current_message); //Print as debug in the output
                    index = 0;
                    programState = DISPLAYING;
                    printf("Current State: DISPLAYING\n");
                    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
                }
                else if(index < INPUT_BUFFER_SIZE - 1){
                    current_message[index++] = (char)c;
                }
                else { //Overflow: print and restart the buffer with the new character. 
                    current_message[INPUT_BUFFER_SIZE - 1] = '\0';
                    //printf("__[RX]:\"%s\"__\n", current_message);
                    index = 0; 
                    current_message[index++] = (char)c;
                }
            }
            else {
                vTaskDelay(pdMS_TO_TICKS(100)); // Wait for new message
            }
        }
    }
}


static void super_init() {
    i2c_deinit(i2c_default);
    gpio_set_function(DEFAULT_I2C_SDA_PIN, GPIO_FUNC_SIO);
    gpio_set_function(DEFAULT_I2C_SCL_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(DEFAULT_I2C_SDA_PIN, GPIO_IN);
    gpio_set_dir(DEFAULT_I2C_SCL_PIN, GPIO_IN);
    sleep_ms(100);
    init_i2c_default();
}

int main(){
    stdio_init_all();
    init_hat_sdk();
    printf("TKJHAT Morse Code Interpreter Starting...\n");
    sleep_ms(3000);
    printf("Welcome to TKJHAT Morse Code Interpreter!\n");

    init_rgb_led();
    init_buzzer();
   
    printf("now singing\n");
    sing(melody);

    init_sw1();
    init_sw2();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, button_function);
    gpio_set_irq_enabled(BUTTON2, GPIO_IRQ_EDGE_RISE, true);
    
    init_ICM42670();
    ICM42670_start_with_default_values();
    sleep_ms(100);

    //if (super_init() != 0) {
        //printf("Initialization failed!\n");
        //return -1;
    //} else {
        //printf("Initialization successful!\n");
    //}
    TaskHandle_t hrgbledTask, hsymbolDetectionTask, hdisplayTask, hmsgToWorkstationTask, hmsgFromWorkstationTask = NULL;
    
    BaseType_t result = xTaskCreate(rgbledTask,
                    "RGB LED task",
                    DEFAULT_STACK_SIZE,
                    NULL,
                    2,
                    &hrgbledTask);

    BaseType_t  result = xTaskCreate(symbolDetectionTask,
        "Symbol Detection",
        DEFAULT_STACK_SIZE,
        NULL,
        2,
        &hsymbolDetectionTask);

        result = xTaskCreate(displayTask,
                    "Displaying", 
                    DEFAULT_STACK_SIZE,
                    NULL,
                    2,
                    &hdisplayTask);

        result = xTaskCreate(msgFromWorkstationTask,
                    "Receiving from workstation", 
                    DEFAULT_STACK_SIZE,
                    NULL,
                    2,
                    &hmsgFromWorkstationTask);

        

    
    vTaskStartScheduler();
    
    return 0;
}
