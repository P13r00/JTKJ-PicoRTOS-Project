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
//float ax, ay, az, gx, gy, gz, t;
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
static void starting_initializer();
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
/*
static void starting_initializer(){
    stdio_init_all();
    init_hat_sdk();
    sleep_ms(300);
    init_button1();
    init_button2();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, toggle_symbol_detection);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_RISE, true, space_key);

    if (init_ICM42670() != 0) {
        printf("Failed to initialize ICM42670\n");
    } else {
        printf("ICM42670 initialized successfully\n");
        if (ICM42670_start_with_default_values() != 0) {
            printf("ICM42670 start-with-defaults failed\n");
        } else {
            printf("ICM42670 started with default values\n");
        }
    }
    sleep_ms(100);
} */

static void symbolDetectionTask(void *pvParameters) { 
    
    printf("Symbol Detection Task started\n");
    
    (void)pvParameters;
    float ax, ay, az, gx, gy, gz, t;
    
    init_i2c_default();

    init_ICM42670();
    ICM42670_start_with_default_values();

    // Set up sensor
    if (init_ICM42670() == 0) {
        printf("ICM42670 initialized successfully\n");
        if (ICM42670_start_with_default_values() == 0) {
            printf("ICM42670 started with default values\n");
        } else {
            printf("ICM42670 start-with-defaults failed\n");
        }

    for (;;) {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) != 0) {
            printf("Sensor read failed:");
            sleep_ms(5000);
            continue;
        }
        printf("gx: %.2f", gx);
        sleep_ms(500);}}

    /*for(;;){
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) == 0) {
            
            printf("Accel: X=%f, Y=%f, Z=%f | Gyro: X=%f, Y=%f, Z=%f| Temp: %2.2f°C\n", ax, ay, az, gx, gy, gz, t);

        }
        sleep_ms(500);
    }*/

    //if(programState==SYMBOL_DETECTION){
        /* for(;;) {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) != 0) {
            printf("Sensor read failed\n");
            sleep_ms(5000);
            continue;
        } else {
        printf("ax: %.2f ay: %.2f az: %.2f | gx: %.2f gy: %.2f gz: %.2f | t: %.2f\n",
               ax, ay, az, gx, gy, gz, t);
               /*
        if (!(gx >= THRESHOLD || gx <= -THRESHOLD)) {
           gx = 0;
        }
        float diff = gx - last_printed_gx;
        if (diff < 0.0f) diff = -diff;
        if (first_gx || diff >= 20.0f) {
            if(gx >= THRESHOLD) {
                printf("Detected Downward Motion: %gx\n", gx);
                append_to_string(current_message, '.'); //down
            } else if (gx <= -THRESHOLD) {
                printf("Detected Upward Motion: %gx\n", gx);
                append_to_string(current_message, '-'); //up
            }
            last_printed_gx = gx;
            first_gx = 0;
        } 
        sleep_ms(500);
        }*/
    //}
    //} 
}

static void displayingTask(void *pvParameters) {
}

static void msgToWorkstationTask(void *pvParameters) {
    if(current_message!=0){
        ("Message: %s", current_message);
    }
}

int main(){

    stdio_init_all();
    init_hat_sdk();
    sleep_ms(300);
    init_button1();
    init_button2();
    gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, toggle_symbol_detection);
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_RISE, true, space_key);
    TaskHandle_t hsymbolDetectionTask, hdisplayingTask, hmsgToWorkstationTask= NULL;

    BaseType_t  result = xTaskCreate(symbolDetectionTask,
        "Symbol Detection",
        DEFAULT_STACK_SIZE,
        NULL,
        2,
        &hsymbolDetectionTask);
        if(result != pdPASS) {
            printf("Symbol Detection task creation failed\n");
            return 0;
        }

        /* result = xTaskCreate(displayingTask,
                    "Displaying", 
                    DEFAULT_STACK_SIZE,
                    NULL,
                    2,
                    &hdisplayingTask);
        if(result != pdPASS) {
            printf("Displaying Task creation failed\n");
            return 0;
        } */

        /* result = xTaskCreate(msgToWorkstationTask,  //could be optional and included in the decoding function
                    "Sending to workstation", 
                    DEFAULT_STACK_SIZE,
                    NULL,
                    2,
                    &hmsgToWorkstationTask);
        if(result != pdPASS) {
            printf("Message to Workstation Task creation failed\n");
            return 0;
        } */


    vTaskStartScheduler();

    return 0;

}
