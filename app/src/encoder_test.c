#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "hal/gpio.h"
#include "hal/rotary_encoder.h"

static int audio_volume = 50; // Tracking variable for volume (0-100%)

int main(void) {
    printf("===========================================\n");
    printf("        HAL ROTARY ENCODER TEST SUITE      \n");
    printf("===========================================\n");

    // 1. Initialize our physical layers
    GPIO_init();
    RotaryEncoder_init();

    printf("Initialization successful!\n");
    printf("Initial volume baseline set to: %d%%\n", audio_volume);
    printf("Spin the knob or press the shaft button to test...\n\n");

    // Baseline tracker to print volume transitions only when they change
    int last_volume = audio_volume;

    // 2. High-speed sampling loop (~200Hz to avoid missing fast steps)
    while (true) {
        // Read the delta movement vector from your HAL logic
        int8_t delta = RotaryEncoder_getDelta();
        
        if (delta != 0) {
            audio_volume += (delta * 5); // Change volume by 5% per click step
            
            // Constrain volume tracking securely between bounds
            if (audio_volume > 100) audio_volume = 100;
            if (audio_volume < 0)   audio_volume = 0;

            if (delta > 0) {
                printf("[CW  ->] Clockwise | Volume updated: %d%%\n", audio_volume);
            } else {
                printf("[CCW <-] Counter-Clockwise | Volume updated: %d%%\n", audio_volume);
            }
        }

        // Check if the built-in push button is currently engaged
        if (RotaryEncoder_isPushButtonPressed()) {
            printf("[BUTTON] Encoder switch is being PRESSED!\n");
            // Simple delay loop to prevent printing a million lines per click
            while (RotaryEncoder_isPushButtonPressed()) {
                usleep(10000); 
            }
            printf("[BUTTON] Encoder switch released.\n");
        }

        // Sleep for 5 milliseconds before polling again
        usleep(5000);
    }

    // Safely unwind drivers if loop breaks
    RotaryEncoder_cleanup();
    GPIO_cleanup();
    return 0;
}
