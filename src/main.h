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

#include "ringbuffer.h"

#define CPU_CORE        0                   /* CPU Core to execute signal generation on */
#define SCHED_PRIO      10                  /* Priority of the signal generation Thread */
#define SIGNAL_FREQ     10                  /* Target signal frequency*/
#define GPIO_PIN        17                  /* GPIO PIN number */
#define GPIO_CHIP       "/dev/gpiochip4"    /* GPIO Chip number. Use 'gpioinfo' to get this information */

#define SEC_IN_NS       1000000000L
#define PERIOD_NS       (SEC_IN_NS / ( 2 * SIGNAL_FREQ ))

#define WINDOW_SIZE     100                 /* Samples to show in GNUPlot */
#define WINDOW_REFRESH  200                 /* Refresh GNUPLot every 200ms */

typedef struct {
    struct gpiod_chip*  chip;
    struct gpiod_line*  line;
} gpio_handle_t;

typedef struct {
    gpio_handle_t*  gpio;
    ring_buffer_t*  rbuffer;
    int             timer_fd;
    int             killswitch;
    int             doPlot;
} thread_args_t;

typedef struct {
    uint64_t    sampleCount;
    uint64_t    diff;
} measurement_t;

extern gpio_handle_t* init_gpio(int gpio_pin, const char* gpio_chip);
extern int stick_thread_to_core(uint8_t core_id);
extern int set_thread_priority(uint8_t priority);
extern inline uint64_t timespec_delta_nanoseconds(struct timespec* end, struct timespec* start);
//extern void* worker_plot(void* args);
extern void* worker_data_handler(void* args);
extern void write_to_file(const char* filename, measurement_t* m, size_t num);
extern FILE* setup_gnuplot();
extern void plot_to_gnuplot(measurement_t* m, size_t num, FILE* gp);

#endif