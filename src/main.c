#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include "tkjhat/sdk.h"

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

static void button_function(uint gpio, uint32_t eventMask); //-piero: REMEMBBER TO ADD ALL PROTOTYPES
static void append_to_string(char *message, char symbol);
static void space_key(uint gpio, uint32_t eventMask);
static void symbolDetectionTask(void *pvParameters);
static void displayingTask(void *pvParameters);
static void msgToWorkstationTask(void *pvParameters);
static void super_init();
static void state_change();
static void reset_screen();

/*static void reset_screen(){
    memset(current_message, 0, INPUT_BUFFER_SIZE); //clear the message buffer
    //reset other variables
    i2c_deinit(i2c_default);
}*/


static void button_function(uint gpio, uint32_t eventMask) { //-piero: removed change state function, everything is working directly
    if (gpio == BUTTON1) {
        if(programState == WAITING){
            programState = SYMBOL_DETECTION;
            printf("Current State: SYMBOL_DETECTION\n");
        } else {
            programState = DISPLAYING;
            printf("Current State: DISPLAYING\n");
        }
    } else if (gpio == BUTTON2) {
        if(programState == WAITING){
            programState = MSG_FROM_WORKSTATION;
            printf("Current State: MSG_FROM_WORKSTATION\n");
        } else {
            programState = DISPLAYING;
            printf("Current State: DISPLAYING\n");
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
    while(1){
        if(programState == SYMBOL_DETECTION){
            memset(current_message, 0, INPUT_BUFFER_SIZE); //clear the message buffer
            for (;;) {
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
                    } else if (gx <= -THRESHOLD) {
                        append_to_string(current_message, '-'); //up
                        printf("-\n");
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

            super_init();
            init_display();
            
            if (current_message[0] == '\0') {
                printf("No message detected\n");
                programState = WAITING;
                printf("Current State: WAITING\n");
            }

            printf("Message: %s\n", current_message);

            vTaskDelay(pdMS_TO_TICKS(100));
            write_text_xy(0, 0, current_message);
            sleep_ms(2000); //Display for 2 seconds

            memset(current_message, 0, INPUT_BUFFER_SIZE); //clear the message buffer

            programState = WAITING;
            printf("Current State: WAITING\n");
        }
    }
}

static void msgFromWorkstationTask(void *pvParameters) { //-piero: implemented from code in examples, allocated right stack size, TODO: more error Handling
    (void)pvParameters;
    memset(current_message, 0, INPUT_BUFFER_SIZE); //clear the message buffer
    size_t index = 0;

    while (1){
        if(programState == MSG_FROM_WORKSTATION){
            // take one char per time and store it in line array, until reeceived the \n
            // The application should instead play a sound, or blink a LED. 
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
    sleep_ms(5000);
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

    TaskHandle_t hsymbolDetectionTask, hdisplayTask, hmsgToWorkstationTask, hmsgFromWorkstationTask = NULL;

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