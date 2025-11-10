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
             //DATA_READY=2,          //string successfully created
             //DECODING=3,            //from Morse -> Characters
             //ENCODING=4,            //from Characters -> Morse
             DISPLAYING=3,          //displaying the message, providing audio feedback
             MSG_FROM_WORKSTATION=2  //??notneeded?? sending the message to the workstation
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

static void button_function(uint gpio, uint32_t eventMask);
static void append_to_string(char *message, char symbol);
static void space_key(uint gpio, uint32_t eventMask);
static void symbolDetectionTask(void *pvParameters);
static void displayingTask(void *pvParameters);
static void msgToWorkstationTask(void *pvParameters);
static void state_change();

static void button_function(uint gpio, uint32_t eventMask) {
    if (gpio == BUTTON1) {
        if(programState == WAITING){
            programState = SYMBOL_DETECTION;
            printf("Current State: %d\n", programState);
        } else {
            state_change();
        }
    } else if (gpio == BUTTON2) {
        if(programState == WAITING){
            printf("Changing state to MSG_FROM_WORKSTATION\n");
            programState = MSG_FROM_WORKSTATION;
            printf("Current State: %d\n", programState);
        } else {
            state_change();
        }
    }
}

static void state_change(){
    if(programState == SYMBOL_DETECTION||programState == MSG_FROM_WORKSTATION){
        programState = DISPLAYING;
        printf("Changing state to DISPLAYING\n");
    } else if(programState == DISPLAYING){
        current_message[0] = '\0';
        programState = WAITING;
        printf("Changing state to WAITING\n");
    }
    //printf("Current State: %d\n", programState);
}

static void append_to_string(char *message, char symbol) {
    // append symbol to current_message string
    while (*message++);
    *(message - 1) = symbol;
    *message = '\0';
}

static void space_key(uint gpio, uint32_t eventMask) {
    append_to_string(current_message, ' ');
}

static void symbolDetectionTask(void *pvParameters) {
    while(1){
        if(programState == SYMBOL_DETECTION){
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
    /*(void)pvParameters;

    init_display();
    printf("Initializing display\n");
    static int counter = 0; 

    while(1) {
        clear_display();
        char buf[5]; //Store a number of maximum 5 figures 
        sprintf(buf,"%d",counter++);
        write_text(buf);
        vTaskDelay(pdMS_TO_TICKS(4000));
    }*/
   while(1){}

}

static void msgToWorkstationTask(void *pvParameters) {
    while(1){
        if(programState == DISPLAYING){
            printf("Message: %s\n", current_message);
            state_change(&programState);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

static void msgFromWorkstationTask(void *pvParameters) {
    while(1){
        if(programState == MSG_FROM_WORKSTATION){
            printf("Write your message to send to the Pico: ");
            /* wait until a non-empty message has been received */
            current_message[0] = '\0';
            while (current_message[0] == '\0') {
                if (scanf("%255s", current_message) == 1){ // implement i2c_read 
                    break;
                };
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            state_change(&programState);
        }
    }
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
    printf("Init successful\n");   
    sleep_ms(100);
    printf("Start by pressing Button1 (right button)\n");
    TaskHandle_t hsymbolDetectionTask, hdisplayingTask, hmsgToWorkstationTask, hmsgFromWorkstationTask = NULL;

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

        result = xTaskCreate(msgFromWorkstationTask,
                    "Receiving from workstation", 
                    DEFAULT_STACK_SIZE,
                    NULL,
                    2,
                    &hmsgFromWorkstationTask);

    
    vTaskStartScheduler();
    
    return 0;
}