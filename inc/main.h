/**
 * @author  jop6462@thi.de
 * 
 */

#pragma once

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
#include <sys/timerfd.h>

#include "config.h"
#include "ringbuffer.h"

#define MAX_SIGNAL_FREQ     10000           /* MAX Target signal frequency*/
#define SEC_IN_NS           1000000000UL    
#define HALF_PERIOD_NS(freq)     (SEC_IN_NS / ( 2 * freq ))

#define WINDOW_SIZE     100                 /* Samples to show in GNUPlot */
#define WINDOW_REFRESH  200                 /* Refresh GNUPLot every 200ms */

#define RING_BUFFER_SIZE 4096               /* Number of measurement_t elements in the ring buffer */

typedef struct {
    struct gpiod_chip*  chip;
    struct gpiod_line*  line;
} gpio_handle_t;

typedef struct {
    gpio_handle_t*  gpio;
    ring_buffer_t*  rbuffer;
    uint64_t        half_period_ns;
    int             sched_prio;
    int             timer_fd;
    int             core_id;
    bool            killswitch;
    bool            doPlot;
    const char*     outputFile;
} thread_args_t;

typedef struct {
    uint64_t    sampleCount;
    uint64_t    diff;
} measurement_t;


/**
 * Function declarations
 */

extern void* func_data_handler(void* args);
extern void* func_signal_gen(void* args);

extern gpio_handle_t* init_gpio(int gpio_pin, const char* gpio_chip);
extern int stick_thread_to_core(int core_id);
extern int set_thread_priority(int priority);
uint64_t get_clock_gettime_overhead();
extern void parse_user_args(int argc, char* argv[], thread_args_t* targs);

/**
 * @brief Calculate the difference in nanoseconds between two timespecs.
 *
 * @param end The end timespec.
 * @param start The start timespec.
 * @return uint64_t The difference in nanoseconds.
 */
static inline uint64_t timespec_delta_nanoseconds(struct timespec* end, struct timespec* start) {
    return (((end->tv_sec - start->tv_sec) * 1.0e9) + (end->tv_nsec - start->tv_nsec));
}

#endif