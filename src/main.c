/**
 * 
 */

#include "../inc/main.h"
#include "../inc/ringbuffer.h"

/**
 * 
 * @brief Worker thread that shall toggle a GPIO pin at a specified frequency while logging
 *        the time difference between two consequtive toggles.
 * 
 */
void* func_signal_gen(void* args) {
    thread_args_t* param = (thread_args_t*)args;
    
    /* Stick this thread to specific cpu core */
    stick_thread_to_core(param->core_id);

    /* Set thread priority - only if configured */
    if (param->sched_prio >= 1) {
        set_thread_priority(param->sched_prio);
    }

    /* Store measured time difference as nanoseconds */
    uint64_t time_diff_ns = 0;

    /**
     * 
     * Your Code goes here...
     * 
     */



    /* Main loop for signal generation and time measurement. */
    while (!param->killswitch) {


        /**
         * 
         * Your Code goes here... 
         * 
         */


        /* Write measured time difference to ringbuffer */
        WRITE_TO_RINGBUFFER(param->rbuffer, time_diff_ns);
    }

    pthread_exit(NULL);
}


/**
 * @brief Main. 
 */
int main(int argc, char** argv) {

    thread_args_t targs;
    parse_user_args(argc, argv, &targs);

    /* initialize GPIO Port with default from config.h */
    if (targs.gpio == NULL) {
        targs.gpio = init_gpio(GPIO_PIN, GPIO_CHIP);
    }

    /* Initialize ringbuffer for storing time measurement results */
    size_t buffer_size = RING_BUFFER_SIZE * sizeof(uint64_t);
    uint8_t buffer[buffer_size];
    ring_buffer_t ring_buffer;
    ring_buffer_init(&ring_buffer, buffer, buffer_size);

    /* configure thread arguments */
    targs.rbuffer = &ring_buffer;
    targs.killswitch = 0;

    /* Create and start worker threads */
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
    printf("Press Enter to stop...\n");
    getchar();
    targs.killswitch = 1;

    pthread_join(worker_signal_gen, NULL);
    pthread_join(worker_data_handler, NULL);

    /* Clean up */
    gpiod_chip_close(targs.gpio->chip);
    free(targs.gpio);

    return EXIT_SUCCESS;
}