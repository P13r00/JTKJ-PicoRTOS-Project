#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048

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
uint8t space_counter = 0;        //counter of consecutive spaces
bool receiving1_sending0 = 0     //reciving a message from workstation or sending a message
//bool communicating = 0           //communicating with another Pico Project

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

static void toggle_symbol_detection(uint gpio, uint32_t eventMask) {
    //Change State
}

static void append_to_string(char *message, char symbol) {
    // append symbol to current_message string
    while (*message++);
    *(message - 1) = symbol;
    *message = '\0';
}

//handlers and tasks
TaskHandle_t hsymbolDetectionTask, hdisplayingTask, hmsgToWorkstationTask= NULL;

BaseType_t  result = xTaskCreate(symbolDetectionTask,
            "Symbol Detection",
            DEFAULT_STACK_SIZE,
            NULL,
            2,
            &hSensorTask);
            if(result != pdPASS) {
                printf("Symbol Detection task creation failed\n");
                return 0;
            }

            result = xTaskCreate(displayingTask,
                        "Displaying", 
                        DEFAULT_STACK_SIZE,
                        NULL,
                        2,
                        &hdisplayingTask);
            if(result != pdPASS) {
                printf("Displaying Task creation failed\n");
                return 0;
            }

            result = xTaskCreate(msgToWorkstationTask,  //could be optional and included in the decoding function
                        "Displaying", 
                        DEFAULT_STACK_SIZE,
                        NULL,
                        2,
                        &hmsgToWorkstationTask);
            if(result != pdPASS) {
                printf("Message to Workstation Task creation failed\n");
                return 0;
            }




