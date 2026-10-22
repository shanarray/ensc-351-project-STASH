#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "hal/gpio.h" // Only requires the low-level GPIO library

// Hardware pin definitions matching your setup
#define PIN_ENC_A 23
#define PIN_ENC_B 24

// Volume tracking variable (Starts at 50%)
static int audio_volume = 50;

int main(void) {
    printf("===========================================\n");
    printf("     LCD-FREE ROTARY ENCODER TEST          \n");
    printf("===========================================\n");
    printf("Initializing GPIO pins...\n");

    // Initialize only the low-level GPIO module
    GPIO_init();
    
    // Read the starting baseline state of Pin A
    bool last_state_a = GPIO_read(PIN_ENC_A);
    
    printf("Initialization successful!\n");
    printf("Current volume baseline: %d%%\n", audio_volume);
    printf("Spin the knob to see direction and value changes...\n\n");

    // Continuous polling loop 
    while (true) {
        bool current_state_a = GPIO_read(PIN_ENC_A);

        // Check if Pin A has transitioned (flipped from true to false or vice versa)
        if (current_state_a != last_state_a) {
            
            // Read Pin B to determine the rotation vector
            if (GPIO_read(PIN_ENC_B) != current_state_a) {
                audio_volume += 5; // Step volume up by 5%
                if (audio_volume > 100) audio_volume = 100;
                printf("[CW  ->] Clockwise | Volume updated to: %d%%\n", audio_volume);
            } else {
                audio_volume -= 5; // Step volume down by 5%
                if (audio_volume < 0) audio_volume = 0;
                printf("[CCW <-] Counter-Clockwise | Volume updated to: %d%%\n", audio_volume);
            }
        }

        // Lock in the state for the next check iteration
        last_state_a = current_state_a;

        // Sample every 5 milliseconds to capture fast manual spins accurately
        usleep(5000); 
    }

    // Cleanup (though loop runs infinitely)
    GPIO_cleanup();
    return 0;
}

