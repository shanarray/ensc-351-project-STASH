
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>

#include <gpiod.h>

#include "hal/rotary_encoder.h"


// GPIO configuration 
#define GPIO_CHIP1_PATH "/dev/gpiochip1"
#define GPIO_CHIP2_PATH "/dev/gpiochip2"

#define ENC_A_OFFSET  41  // GPIO22
#define ENC_B_OFFSET  8   // GPIO17 
#define ENC_SW_OFFSET 13  // GPIO15 

#define EVENT_BUFFER_SIZE 16
#define POLL_TIMEOUT_NS 10000000LL


// GPIO resources 
static struct gpiod_chip *chip1;
static struct gpiod_chip *chip2;
static struct gpiod_line_request *request_a;
static struct gpiod_line_request *request_b_sw;
static struct gpiod_edge_event_buffer *event_buffer;

static pthread_t encoder_thread;


// Thread synchronization 
static pthread_mutex_t encoder_mutex = PTHREAD_MUTEX_INITIALIZER;
static atomic_bool stopping = ATOMIC_VAR_INIT(false);


// Encoder state 
static bool initialized;
static bool thread_started;

static int pending_delta;
static bool button_pressed;


// Read a GPIO input. 
static int read_gpio(struct gpiod_line_request *gpio_request, unsigned int offset)
{
    enum gpiod_line_value value =
        gpiod_line_request_get_value(gpio_request, offset);

    if (value == GPIOD_LINE_VALUE_ERROR)
        return -1;

    return value == GPIOD_LINE_VALUE_ACTIVE ? 1 : 0;
}


// Encoder background thread. 
static void *encoder_thread_func(void *arg)
{
    (void)arg;

    while (!atomic_load(&stopping)) {

        
        // Wait for a falling edge on encoder A.
        // The timeout also allows us to sample the button.
        int rec = gpiod_line_request_wait_edge_events(
            request_a, POLL_TIMEOUT_NS
        );

        if (rec < 0) {

            if (atomic_load(&stopping))
                break;

            perror("RotaryEncoder: waiting for edge events failed");
            break;
        }

        if (rec > 0) {

            int num_events = gpiod_line_request_read_edge_events(
                request_a, event_buffer, EVENT_BUFFER_SIZE
            );

            if (num_events < 0) {
                perror("RotaryEncoder: reading edge events failed");
                break;
            }

            for (int i = 0; i < num_events; i++) {

                struct gpiod_edge_event *event =
                    gpiod_edge_event_buffer_get_event(
                        event_buffer, i
                    );

                if (event == NULL)
                    continue;

                if (gpiod_edge_event_get_line_offset(event) == ENC_A_OFFSET) {

                    int value_b = read_gpio(request_b_sw, ENC_B_OFFSET);

                    if (value_b < 0) {
                        perror("RotaryEncoder: reading encoder B failed");
                        continue;
                    }


                    // Determine rotation direction
                    
                    int delta = value_b ? 1 : -1;

                    pthread_mutex_lock(&encoder_mutex);

                    pending_delta += delta;

                    if (pending_delta > 1000)
                        pending_delta = 1000;

                    if (pending_delta < -1000)
                        pending_delta = -1000;

                    pthread_mutex_unlock(&encoder_mutex);
                }
            }
        }

        
        // Sample the push button
        // Assumes the switch pulls SW low when pressed
         
        int sw = read_gpio(request_b_sw, ENC_SW_OFFSET);

        if (sw >= 0) {

            pthread_mutex_lock(&encoder_mutex);

            button_pressed = (sw == 0);

            pthread_mutex_unlock(&encoder_mutex);
        }
    }

    return NULL;
}


// Initialize rotary encoder GPIO. 
bool rotary_encoder_init(void)
{
    if (initialized)
        return true;

    atomic_store(&stopping, false);
    struct gpiod_line_settings *settings_a = NULL;
    struct gpiod_line_settings *settings_b = NULL;
    struct gpiod_line_settings *settings_sw = NULL;

    struct gpiod_line_config *config_a = NULL;
    struct gpiod_line_config *config_b_sw = NULL;

    struct gpiod_request_config *request_config_a = NULL;
    struct gpiod_request_config *request_config_b_sw = NULL;

    chip1 = gpiod_chip_open(GPIO_CHIP1_PATH);

    if (chip1 == NULL) {
        perror("RotaryEncoder: opening gpiochip1 failed");
        goto fail;
    }

    chip2 = gpiod_chip_open(GPIO_CHIP2_PATH);

    if (chip2 == NULL) {
        perror("RotaryEncoder: opening gpiochip2 failed");
        goto fail;
    }

    // Allocate configuration objects
    settings_a = gpiod_line_settings_new();
    settings_b = gpiod_line_settings_new();
    settings_sw = gpiod_line_settings_new();

    config_a = gpiod_line_config_new();
    config_b_sw = gpiod_line_config_new();

    request_config_a = gpiod_request_config_new();
    request_config_b_sw = gpiod_request_config_new();

    if (settings_a == NULL ||
        settings_b == NULL ||
        settings_sw == NULL ||
        config_a == NULL ||
        config_b_sw == NULL ||
        request_config_a == NULL ||
        request_config_b_sw == NULL) {

        fprintf(stderr,
                "RotaryEncoder: allocating GPIO configuration failed\n");

        goto fail;
    }

    // Encoder A: input with falling-edge detection. 
    if (gpiod_line_settings_set_direction(
            settings_a, GPIOD_LINE_DIRECTION_INPUT) < 0 ||
        gpiod_line_settings_set_edge_detection(
            settings_a, GPIOD_LINE_EDGE_FALLING) < 0) {

        perror("RotaryEncoder: configuring encoder A failed");
        goto fail;
    }

    // Encoder B: standard input. 
    if (gpiod_line_settings_set_direction(
            settings_b, GPIOD_LINE_DIRECTION_INPUT) < 0) {

        perror("RotaryEncoder: configuring encoder B failed");
        goto fail;
    }

    // Encoder button: standard input. 
    if (gpiod_line_settings_set_direction(
            settings_sw, GPIOD_LINE_DIRECTION_INPUT) < 0) {

        perror("RotaryEncoder: configuring button failed");
        goto fail;
    }

    // GPIO 22
    unsigned int offset_a = ENC_A_OFFSET;
    if (gpiod_line_config_add_line_settings(config_a, &offset_a, 1, settings_a) < 0) {
        perror("RotaryEncoder adding encoder A settings failed");
        goto fail;
    }

    // Configure GPIO 15 and 17
    unsigned int offset_b = ENC_B_OFFSET;
    unsigned int offset_sw = ENC_SW_OFFSET;
    if (gpiod_line_config_add_line_settings(
        config_b_sw, &offset_b, 1, settings_b) < 0 ||
        gpiod_line_config_add_line_settings(
        config_b_sw, &offset_sw, 1, settings_sw) < 0) {

        perror("RotaryEncoder: adding GPIO15 and 17 settings failed");
        goto fail;
    }

    gpiod_request_config_set_consumer(request_config_a, "study_timer_encoder_a");
    gpiod_request_config_set_consumer(request_config_b_sw, "study_timer_encoder_b_sw");

    // Request GPIO22 from gpiochip1
    request_a = gpiod_chip_request_lines(chip1, request_config_a, config_a);

    if (request_a == NULL) {
        perror("RotaryEncoder: requesting GPIO22 lines failed");
        goto fail;
    }

    // Request GPIO15 and 17 from gpiochip2
    request_b_sw = gpiod_chip_request_lines(chip2, request_config_b_sw, config_b_sw );

    if (request_b_sw == NULL) {
        perror("RotaryEncoder: requesting GPIO15 and 17 lines failed");
        goto fail;
    }

    event_buffer = gpiod_edge_event_buffer_new(EVENT_BUFFER_SIZE);

    if (event_buffer == NULL) {
        perror("RotaryEncoder: creating event buffer failed");
        goto fail;
    }

    // Release temporary configuration objects.
    gpiod_line_settings_free(settings_a);
    gpiod_line_settings_free(settings_b);
    gpiod_line_settings_free(settings_sw);
    
    gpiod_line_config_free(config_a);
    gpiod_line_config_free(config_b_sw);

    gpiod_request_config_free(request_config_a);
    gpiod_request_config_free(request_config_b_sw);


    pthread_mutex_lock(&encoder_mutex);

    pending_delta = 0;
    button_pressed = false;

    pthread_mutex_unlock(&encoder_mutex);

    int result = pthread_create(
        &encoder_thread,
        NULL,
        encoder_thread_func,
        NULL
    );

    if (result != 0) {

        fprintf(stderr,
                "RotaryEncoder: pthread_create failed: %d\n",
                result);

        goto fail;
    }

    thread_started = true;
    initialized = true;

    return true;


fail:

    if (settings_a != NULL)
        gpiod_line_settings_free(settings_a);

    if (settings_b != NULL)
        gpiod_line_settings_free(settings_b);

    if (settings_sw != NULL)
        gpiod_line_settings_free(settings_sw);

    if (config_a != NULL)
        gpiod_line_config_free(config_a);
    
    if (config_b_sw != NULL)
        gpiod_line_config_free(config_b_sw);

    if (request_config_a != NULL)
        gpiod_request_config_free(request_config_a);

    if (request_config_b_sw != NULL)
        gpiod_request_config_free(request_config_b_sw);

    if (event_buffer != NULL) {
        gpiod_edge_event_buffer_free(event_buffer);
        event_buffer = NULL;
    }

    if (request_a != NULL) {
        gpiod_line_request_release(request_a);
        request_a = NULL;
    }

    if (request_b_sw != NULL) {
        gpiod_line_request_release(request_b_sw);
        request_b_sw = NULL;
    }

    if (chip1 != NULL) {
        gpiod_chip_close(chip1);
        chip1 = NULL;
    }
    
    if (chip2 != NULL) {
        gpiod_chip_close(chip2);
        chip2 = NULL;
    }
    
    return false;
}


// Get accumulated encoder rotation. 
int8_t rotary_encoder_get_delta(void)
{
    pthread_mutex_lock(&encoder_mutex);

    int delta = pending_delta;
    pending_delta = 0;

    pthread_mutex_unlock(&encoder_mutex);

    if (delta > 127)
        delta = 127;

    if (delta < -127)
        delta = -127;

    return (int8_t)delta;
}


// Check whether the encoder button is pressed. 
bool rotary_encoder_is_button_pressed(void)
{
    pthread_mutex_lock(&encoder_mutex);

    bool pressed = button_pressed;

    pthread_mutex_unlock(&encoder_mutex);

    return pressed;
}


// Stop the encoder thread and release GPIO resources. 
void rotary_encoder_cleanup(void)
{
    if (!initialized)
        return;

    atomic_store(&stopping, true);

    if (thread_started) {
        pthread_join(encoder_thread, NULL);
        thread_started = false;
    }

    if (event_buffer != NULL) {
        gpiod_edge_event_buffer_free(event_buffer);
        event_buffer = NULL;
    }

    if (request_a != NULL) {
        gpiod_line_request_release(request_a);
        request_a = NULL;
    }

    
    if (request_b_sw != NULL) {
        gpiod_line_request_release(request_b_sw);
        request_b_sw = NULL;
    }

    if (chip1 != NULL) {
        gpiod_chip_close(chip1);
        chip1 = NULL;
    }

    if (chip2 != NULL) {
        gpiod_chip_close(chip2);
        chip2 = NULL;
    }

    initialized = false;
}


