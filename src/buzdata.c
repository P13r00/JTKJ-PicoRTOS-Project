
#include "buzdata.h"
#include "pico/stdlib.h"
#include "tkjhat/sdk.h"

void sing(int notes[][2]) {
    int j = 0;
    while (notes[j][1] != 0) {
        buzzer_play_tone(notes[j][0], notes[j][1]);
        sleep_ms(15);  // Small delay between notes
        j++;
    }
}
