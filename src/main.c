#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "tkjhat/sdk.h"

#define DEFAULT_STACK_SIZE 2048
// #define CDC_ITF_TX      1 what does it do?

enum state { WAITING=0, SYMBOL_DETECTION=1, DATA_READY=2};
enum state programState = WAITING;

char current_message[256] = {0};
// ...existing code...;

/* 
if button 1:
    state = 1
    while true:
        detect symbol
        add symbol to string
        if space counter = 3:
            state = 2
            break
    send data to workstation
    state = 0
    */

static void toggle_symbol_detection(uint gpio, uint32_t eventMask) {
    programState = SYMBOL_DETECTION;
    // gpio_set_irq_enabled_with_callback(BUTTON1, GPIO_IRQ_EDGE_RISE, true, btn_fxn);
}

static void append_to_string(char *message, char symbol) {
    // append symbol to current_message string
    while (*message++);
    *(message - 1) = symbol;
    *message = '\0';
}

static void space_key(uint gpio, uint32_t eventMask) {
    // key to add space
    printf(" ");
    //append_to_string(current_message, ' ');
}

float ax = 0.0, ay = 0.0, az = 0.0, gx = 0.0, gy = 0.0, gz = 0.0, t = 0.0;
static float last_printed_gx = 0.0f;
static int first_gx = 0;

int main() {
    stdio_init_all();
    init_hat_sdk();
    sleep_ms(300);
    init_button2();
    gpio_set_irq_enabled_with_callback(BUTTON2, GPIO_IRQ_EDGE_RISE, true, space_key);

    // Set up sensor
    if (init_ICM42670() != 0) {
        printf("Failed to initialize ICM42670\n");
    } else {
        printf("ICM42670 initialized successfully\n");
        if (ICM42670_start_with_default_values() != 0) {
            printf("ICM42670 start-with-defaults failed\n");
        } else {
            printf("ICM42670 started with default values\n");
        }

    for (;;) {
        if (ICM42670_read_sensor_data(&ax, &ay, &az, &gx, &gy, &gz, &t) != 0) {
            printf("Sensor read failed:");
            sleep_ms(500);
            continue;
        }

        // print gx only when it changes significantly


        const float threshold = 30.0f;

        if (!(gx >= 230.0f || gx <= -230.0f)) {
           gx = 0;
        }

        float diff = gx - last_printed_gx;
        if (diff < 0.0f) diff = -diff;
        if (first_gx || diff >= threshold) {
            if(gx >= 230.0f) {
                printf("."); //down
                //append_to_string(current_message, '.');
            } else if (gx <= -230.0f) {
                printf("-"); //up
                //append_to_string(current_message, '-');
            }
            last_printed_gx = gx;
            first_gx = 0;
        }
        
        sleep_ms(100);
    }
        sleep_ms(100);
    }
    return 0;
}

