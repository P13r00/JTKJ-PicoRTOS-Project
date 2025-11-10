#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
#define THRESHOLD          100

//states
enum state { WAITING=0,             //await input from sensor or workstation
             SYMBOL_DETECTION=1,    //read symbols from sensor
             DATA_READY=2,          //string successfully created
             DECODING=3,            //from Morse -> Characters
             ENCODING=4,            //from Characters -> Morse
             DISPLAYING=5,          //displaying the message, providing audio feedback
             MSG_TO_WORKSTATION=6  //??notneeded?? sending the message to the workstation
             //MSG_TO_FRIEND=7,       //sending a message to another project
             //MSG_FROM_FRIEND=8      //receiving a message from another project
            };
enum state programState = WAITING;

//global variables
char current_message[256] = {0}; //current message
uint8_t space_counter = 0;        //counter of consecutive spaces
bool receiving1_sending0 = 0;     //reciving a message from workstation or sending a message
//bool communicating = 0           //communicating with another Pico Project
float ax = 0.0, ay = 0.0, az = 0.0, gx = 0.0, gy = 0.0, gz = 0.0, t = 0.0;
static float last_printed_gx = 0.0f;
static int first_gx = 0;

/*
main loop
while True:
    if SENDING:
        SYMBOL_DETECTION
        DATA_READY
        DECODING
        DISPLAYING
        MSG_TO_WORKSTATION

    else if RECEIVING:
        DECODING
        DISPLAYING
        MSG_TO_WORKSTATION

    else if COMMUNICATING:
        if SENDING:
            SYMBOL_DETECTION
            DATA_READY
            MSG_TO_FRIEND
            DECODING
            DISPLAYING
            MSG_TO_WORKSTATION

        else if RECEIVING:
            MSG_FROM_FRIEND
            DECODING
            DISPLAYING
            MSG_TO_WORKSTATION

    WAITING
*/

static void toggle_symbol_detection(uint gpio, uint32_t eventMask);
static void append_to_string(char *message, char symbol);
static void space_key(uint gpio, uint32_t eventMask);
static void symbolDetectionTask(void *pvParameters);
static void displayingTask(void *pvParameters);
static void msgToWorkstationTask(void *pvParameters);

static void toggle_symbol_detection(uint gpio, uint32_t eventMask) {
    //Change State
    programState = SYMBOL_DETECTION;
}

static void append_to_string(char *message, char symbol) {
    // append symbol to current_message string
    while (*message++);
    *(message - 1) = symbol;
    *message = '\0';
}

static void space_key(uint gpio, uint32_t eventMask) {
    // key to add space
    append_to_string(current_message, ' ');
}

static void symbolDetectionTask(void *pvParameters) {
    while(1){
        if(programState==SYMBOL_DETECTION){
            for (;;) {
                if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) != 0) {
                    printf("Sensor read failed:");
                    sleep_ms(200);
                    continue;
                }
                if (!(gx >= THRESHOLD || gx <= -THRESHOLD)) {
                gx = 0;
                }
                float diff = gx - last_printed_gx;
                if (diff < 0.0f) diff = -diff;
                if (first_gx || diff >= 20.0f) {
                    if(gx >= THRESHOLD) {
                        append_to_string(current_message, '.'); //down
                    } else if (gx <= -THRESHOLD) {
                        append_to_string(current_message, '-'); //up
                    }
                    last_printed_gx = gx;
                    first_gx = 0;
            }
            sleep_ms(100);
            }
        }
    }
}

static void displayingTask(void *pvParameters) {
    while(1){}
}

static void msgToWorkstationTask(void *pvParameters) {
    while(1){
        if(current_message!=0){
            ("Message: %s", current_message);
        }
    }
}

int main(){
    stdio_init_all();
    init_hat_sdk();
    sleep_ms(20000);
    init_sw1();
    init_sw2();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, toggle_symbol_detection);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_RISE, true, space_key);

    init_ICM42670();
    ICM42670_start_with_default_values();
    printf("Init successful");   
    sleep_ms(100);
    TaskHandle_t hsymbolDetectionTask, hdisplayingTask, hmsgToWorkstationTask;

    BaseType_t  result = xTaskCreate(symbolDetectionTask,
        "Symbol Detection",
        DEFAULT_STACK_SIZE,
        NULL,
        2,
        &hsymbolDetectionTask);

        result = xTaskCreate(displayingTask,
                    "Displaying", 
                    DEFAULT_STACK_SIZE,
                    NULL,
                    2,
                    &hdisplayingTask);

        result = xTaskCreate(msgToWorkstationTask,  //could be optional and included in the decoding function
                    "Sending to workstation", 
                    DEFAULT_STACK_SIZE,
                    NULL,
                    2,
                    &hmsgToWorkstationTask);

    
    

    vTaskStartScheduler();
    // stdio_init_all();
    // sleep_ms(20000);  // Wait for serial
    // printf("=== Starting Program ===\n");
    
    // init_hat_sdk();
    // printf("HAT SDK initialized\n");
    
    // // Check IMU initialization
    // int imu_result = init_ICM42670();
    // printf("IMU init result: %d\n", imu_result);
    
    // if (imu_result == 0) {
    //     int start_result = ICM42670_start_with_default_values();
    //     printf("IMU start result: %d\n", start_result);
    // }
    
    // printf("Initialization complete\n");

    // return 0;

}
