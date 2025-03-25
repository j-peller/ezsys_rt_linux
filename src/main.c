/**
 * 
 */

#include "main.h"
#include "ringbuffer.h"

/**
 * @brief Worker thread that toggles a GPIO pin and logs the delay into a ring buffer
 *        
 */
void* worker_signal_gen(void* args) {
    thread_args_t* param = (thread_args_t*)args;
    
    /* Fixiate this thread to CPU_CORE */
    stick_thread_to_core(CPU_CORE);

    // Optionally set thread priority -- this requires ROOT privileges!
    // set_thread_priority(SCHED_PRIO);

    int current_state = 0;
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    uint64_t sampleCount = 0;

    /* Until user stops main program */
    while (!param->killswitch) {

        /* get current timestamp */
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        uint64_t diff = timespec_delta_nanoseconds(&now, &start);

        /* Check if a period has passed */
        if (diff >= PERIOD_NS) {
            current_state = !current_state;
            /* toogle GPIO pin */
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




int main() {
    gpio_handle_t *gpio = init_gpio(GPIO_PIN, GPIO_CHIP);
    if (!gpio) {
        fprintf(stderr, "GPIO-Initialisierung fehlgeschlagen\n");
        return EXIT_FAILURE;
    }

    /* Initialize the ring buffer for measurements */
    size_t buffer_size = 10 * 1024 * sizeof(measurement_t);
    char buffer[buffer_size];
    ring_buffer_t ring_buffer;
    ring_buffer_init(&ring_buffer, buffer, buffer_size);

    thread_args_t targs;
    targs.gpio = gpio;
    targs.rbuffer = &ring_buffer;
    targs.killswitch = 0;
    targs.doPlot = 1;

    pthread_t worker, plot_thread;
    int ret = pthread_create(&worker, NULL, &worker_signal_gen, &targs);
    if (ret != 0) {
        fprintf(stderr, "Error spawning Worker-Thread\n");
        return EXIT_FAILURE;
    }

    ret = pthread_create(&plot_thread, NULL, &worker_data_handler, &targs);
    if (ret != 0) {
        fprintf(stderr, "Error spawning Plot-Thread\n");
        return EXIT_FAILURE;
    }

    /* Wait for user input to stop the program */
    getchar();
    targs.killswitch = 1;

    pthread_join(worker, NULL);
    pthread_join(plot_thread, NULL);

    /* Clean up */
    gpiod_chip_close(gpio->chip);
    free(gpio);

    return EXIT_SUCCESS;
}