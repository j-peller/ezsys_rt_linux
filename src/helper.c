#include "main.h"

/* Initialize the GPIO port and return a handle */
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

/* Bind the thread to a specific CPU core */
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

/* Set thread priority (if needed) */
int set_thread_priority(uint8_t priority) {
    struct sched_param schedParam;
    schedParam.sched_priority = priority;
    int ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam);
    if(ret != 0) {
        perror("Fehler beim Setzen des Echtzeit-Schedulings");
    }
    return ret;
}

/* Calculate the difference in nanoseconds between two timespecs */
inline uint64_t timespec_delta_nanoseconds(struct timespec* end, struct timespec* start) {
    return (((end->tv_sec - start->tv_sec) * 1.0e9) + (end->tv_nsec - start->tv_nsec));
}

/**
 * @brief Worker thread for plotting the jitter of two consecutive GPIO Pin toggles using GNUPlot
 */
void* worker_plot(void* args) {
    thread_args_t* param = (thread_args_t*)args;
    FILE *gp = popen("gnuplot -persistent", "w");
    if (!gp) {
        perror("Could not open gnuplot pipe");
        pthread_exit(NULL);
    }

    /* Gnuplot setup for jitter */
    fprintf(gp, "set title 'GPIO Toggle Jitter'\n");
    fprintf(gp, "set xlabel 'Sample Count'\n");
    fprintf(gp, "set ylabel 'Jitter (ns)'\n");
    fprintf(gp, "set ytics auto\n");
    fprintf(gp, "set format y '%%.0f'\n");
    fflush(gp);

    measurement_t* all_measurements = NULL;
    size_t all_count = 0, capacity = 0;
    measurement_t m;

    /* Until user stops main program */
    while (!param->killswitch) {

        /* Dequeue measurements from ring buffer into temporary buffer for evaluation */
        while (ring_buffer_dequeue_arr(param->rbuffer, (char*)&m, sizeof(measurement_t)) == sizeof(measurement_t)) {
            if (all_count >= capacity) {
                size_t new_capacity = (capacity == 0) ? 1024 : capacity * 2;
                measurement_t* temp = realloc(all_measurements, new_capacity * sizeof(measurement_t));
                if (!temp) {
                    perror("realloc failed");
                    break;
                }
                all_measurements = temp;
                capacity = new_capacity;
            }
            all_measurements[all_count++] = m;
        }

        /* Need at least 2 measurements to compute jitter */
        if (all_count > 1) {
            size_t start_index = (all_count > WINDOW_SIZE) ? (all_count - WINDOW_SIZE) : 0;
            size_t jitter_count = all_count - start_index - 1;
            uint64_t jitters[WINDOW_SIZE] = {0};
            uint64_t jitter_min, jitter_max;

            /* Compute jitter values and determine min/max in one loop */
            for (size_t i = start_index + 1; i < all_count; i++) {
                uint64_t prev = all_measurements[i - 1].diff;
                uint64_t curr = all_measurements[i].diff;
                uint64_t j = (curr >= prev) ? curr - prev : prev - curr;
                jitters[i - start_index - 1] = j;
                if (i == start_index + 1) {
                    jitter_min = j;
                    jitter_max = j;
                } else {
                    if (j < jitter_min)
                        jitter_min = j;
                    if (j > jitter_max)
                        jitter_max = j;
                }
            }
            /* Add margin (10% of range or at least 1000 ns) */
            uint64_t margin = (jitter_max > jitter_min) ? ((jitter_max - jitter_min) / 10) : 1000;
            if (margin == 0)
                margin = 1000;
            uint64_t yr_min = (jitter_min > margin) ? (jitter_min - margin) : 0;
            uint64_t yr_max = jitter_max + margin;

            /* Determine x-axis range based on sampleCount */
            uint64_t x_min = all_measurements[start_index + 1].sampleCount;
            uint64_t x_max = all_measurements[all_count - 1].sampleCount;
            fprintf(gp, "set xrange [%" PRIu64 ":%" PRIu64 "]\n", x_min, x_max);
            fprintf(gp, "set yrange [%" PRIu64 ":%" PRIu64 "]\n", yr_min, yr_max);

            /* Add labels for min and max jitter on the graph */
            fprintf(gp, "unset label 1\n");
            fprintf(gp, "set label 1 'Max: %" PRIu64 " ns' at graph 0.02, graph 0.9\n", jitter_max);

            /* Plot jitter data */
            fprintf(gp, "plot '-' using 1:2 with linespoints title 'Jitter (ns)'\n");
            for (size_t i = 0; i < jitter_count; i++) {
                uint64_t sample = all_measurements[start_index + i + 1].sampleCount;
                fprintf(gp, "%" PRIu64 " %" PRIu64 "\n", sample, jitters[i]);
            }
            fprintf(gp, "e\n");
            fflush(gp);
        }
        usleep(WINDOW_REFRESH * 1000);  // Update every 500ms
    }
    free(all_measurements);
    pclose(gp);
    pthread_exit(NULL);
}