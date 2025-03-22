#ifndef MAIN_H
#define MAIN_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <gpiod.h>
#include <sched.h>

#include "ringbuffer.h"

#define SEC_IN_NS       1000000000L
#define PERIOD_NS       (SEC_IN_NS / ( 2 * SIGNAL_FREQ ))

#define WINDOW_SIZE     100                 /* Samples to show in GNUPlot */
#define WINDOW_REFRESH  200                 /* Refresh GNUPLot every 100ms */

typedef struct {
    struct gpiod_chip*  chip;
    struct gpiod_line*  line;
} gpio_handle_t;

typedef struct {
    gpio_handle_t*  gpio;
    ring_buffer_t*  rbuffer;
    int             killswitch;
} thread_args_t;

typedef struct {
    uint64_t    sampleCount;
    uint64_t    diff;
} measurement_t;

extern gpio_handle_t* init_gpio(int gpio_pin, const char* gpio_chip);
extern int stick_thread_to_core(uint8_t core_id);
extern int set_thread_priority(uint8_t priority);
extern inline uint64_t timespec_delta_nanoseconds(struct timespec* end, struct timespec* start);
extern void* worker_plot(void* args);

#endif