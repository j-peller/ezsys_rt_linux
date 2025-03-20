#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <gpiod.h>
#include <sched.h>

#define CPU_CORE        0       /* */
#define SCHED_PRIO      10
#define SIGNAL_FREQ     500
#define GPIO_PIN        17

#define SEC_IN_NS       1000000000L
#define PERIOD_NS       (SEC_IN_NS / ( 2 * SIGNAL_FREQ ))

typedef struct {
    struct gpiod_chip*  chip;
    struct gpiod_line*  line;
} gpio_handle_t;

typedef struct {
    gpio_handle_t*  gpio;
    int             killswitch;
} thread_args_t;


// Initialisiert den gewünschten GPIO-Port und gibt einen Handle zurück
gpio_handle_t* init_gpio(int gpio_pin, const char* gpio_chip) {
    gpio_handle_t* handle = malloc(sizeof(gpio_handle_t));
    if (!handle) {
        perror("Fehler bei malloc");
        return NULL;
    }

    if (gpio_chip == NULL) {
        perror("Fehler, kein GPIO Chip gegeben. Nutze gpioinfo, um herauszufinden welchen chip du benötigst");
        free(handle);
        return NULL;
    }

    handle->chip = gpiod_chip_open(gpio_chip);
    if (!handle->chip) {
        perror("Fehler beim Öffnen des GPIO-Chips");
        free(handle);
        return NULL;
    }
    handle->line = gpiod_chip_get_line(handle->chip, gpio_pin);
    if (!handle->line) {
        perror("Fehler beim Abrufen der GPIO-Leitung");
        gpiod_chip_close(handle->chip);
        free(handle);
        return NULL;
    }
    if (gpiod_line_request_output(handle->line, "RPiSignal", 0) < 0) {
        perror("Fehler bei der Konfiguration der GPIO-Leitung als Ausgang");
        gpiod_chip_close(handle->chip);
        free(handle);
        return NULL;
    }
    return handle;
}

/**
 * 
 */
int stick_thread_to_core(uint8_t core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if(ret != 0) {
        perror("Fehler beim Setzen der CPU-Affinität\n");
    }

    return ret;
}

/**
 * 
 */
int set_thread_priority(uint8_t priority) {
    struct sched_param schedParam;
    schedParam.sched_priority = priority;
    int ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam);
    if(ret != 0) {
        perror("Fehler beim Setzen des Echtzeit-Schedulings: %s\n");
    }
    return ret;
}

uint64_t timespec_delta_nanoseconds(struct timespec* end, struct timespec* start)
{
    return ( ((end->tv_sec - start->tv_sec)*1.0e9) + (end->tv_nsec - start->tv_nsec) );
}

void* worker_signal_gen(void* args) {
    thread_args_t* param = (thread_args_t*)args;

    stick_thread_to_core(CPU_CORE);

    set_thread_priority(SCHED_PRIO);

    int current_state = 0;

    /* Start der Messung erfassen */
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    while (!param->killswitch)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);

        /* toggle den GPIO pin */
        uint64_t diff = timespec_delta_nanoseconds(&now, &start);
        if (diff  >= PERIOD_NS) {
            current_state = !current_state;
            gpiod_line_set_value(param->gpio->line, current_state);
        }
    }

    pthread_exit(NULL);
}

int main() {
    printf("Hello World\n");

    return EXIT_SUCCESS;
}