/**
 * @author jop6462@thi.de
 * @brief Configuration file for signal generation and measurement
 */

#pragma once

#ifndef CONFIG_H
#define CONFIG_H


#define CPU_CORE        0                   /* Default CPU Core to execute signal generation on */
#define SCHED_PRIO      0                   /* Default priority of the signal generation Thread */
#define SIGNAL_FREQ     10                  /* Default target signal frequency*/


/**
 * Use 'gpioinfo' to get the GPIO chip number.
 * The respective GPIO chip device file is usually found in "/dev/gpiochipX"
 * 
 * For Pin layout search for GPIO pinout on the web.
 */
#define GPIO_PIN        17                  /* GPIO PIN number for frequency measurement with Oscilloscope */
#define GPIO_CHIP       "/dev/gpiochip4"    /* GPIO Chip number. Use 'gpioinfo' to get this information */

#endif