/**
 * 
 */

#include "main.h"
#include "ringbuffer.h"

/**
 * @brief Worker thread that toggles a GPIO pin and logs the delay into a ring buffer
 *        
 */
void* func_signal_gen(void* args) {
    thread_args_t* param = (thread_args_t*)args;
    
    /* Fixiate this thread to CPU_CORE */
    stick_thread_to_core(param->core_id);

    // Optionally set thread priority -- this requires ROOT privileges!
    // set_thread_priority(SCHED_PRIO);
    
    /* calculate clock_gettime overhead */
    uint64_t err = get_clock_gettime_overhead();

    int current_state = 0;
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    uint64_t sampleCount = 0;

    /* Until user stops main program */
    while (!param->killswitch) {

        /* get current timestamp */
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);

        /* calculate time difference, corrected by clock_gettime overhead */
        uint64_t diff = timespec_delta_nanoseconds(&now, &start) - err;

        /* Check if a period has passed */
        if (diff >= param->period_ns) {

            /* toogle GPIO pin */
            current_state = !current_state;
            gpiod_line_set_value(param->gpio->line, current_state);

            /* set start timestamp to current timestamp */
            start = now;

            /* Write time difference to ring buffer */
            measurement_t m;
            m.sampleCount = sampleCount++;
            m.diff = diff;
            ring_buffer_queue_arr(param->rbuffer, (char*)&m, sizeof(measurement_t));
        }
    }
    pthread_exit(NULL);
}


int main(int argc, char** argv) {

    thread_args_t targs;
    parse_user_args(argc, argv, &targs);

    /* initialize GPIO Port for output */
    gpio_handle_t *gpio = init_gpio(GPIO_PIN, GPIO_CHIP);
    if (!gpio) {
        fprintf(stderr, "GPIO-Initialisierung fehlgeschlagen\n");
        return EXIT_FAILURE;
    }

    /* Initialize ring buffer storing measurement results */
    size_t buffer_size = RING_BUFFER_SIZE * sizeof(measurement_t);
    char buffer[buffer_size];
    ring_buffer_t ring_buffer;
    ring_buffer_init(&ring_buffer, buffer, buffer_size);

    /* configure thread arguments */
    targs.gpio = gpio;
    targs.rbuffer = &ring_buffer;
    targs.killswitch = 0;

    pthread_t worker_signal_gen, worker_data_handler;
    int ret = pthread_create(&worker_signal_gen, NULL, &func_signal_gen, &targs);
    if (ret != 0) {
        fprintf(stderr, "Error spawning Worker-Thread\n");
        return EXIT_FAILURE;
    }

    ret = pthread_create(&worker_data_handler, NULL, &func_data_handler, &targs);
    if (ret != 0) {
        fprintf(stderr, "Error spawning Plot-Thread\n");
        return EXIT_FAILURE;
    }

    /* Wait for user input to stop the program */
    getchar();
    targs.killswitch = 1;

    pthread_join(worker_signal_gen, NULL);
    pthread_join(worker_data_handler, NULL);

    /* Clean up */
    gpiod_chip_close(gpio->chip);
    free(gpio);

    return EXIT_SUCCESS;
}