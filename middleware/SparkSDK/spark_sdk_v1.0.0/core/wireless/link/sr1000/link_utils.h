/** @file link_utils.h
 *  @brief Link utility macros and functions.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_UTILS_H_
#define LINK_UTILS_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "sr1000_def.h"

/* CONSTANTS ******************************************************************/
#define WEAKEST_SIGNAL_CODE             47
#define GAIN_ENTRY_RANGE_TENTH_DB       235

/** @brief Sleep mode enumeration.
 * TODO: Delete, usage should be discontinued. The sleep_lvl_t sr1000_def.h enumeration should be used instead.
 */
typedef enum link_low_sleep_mode {
    SLEEP_MODE_IDLE,    /**< Idle sleep mode */
    SLEEP_MODE_SHALLOW, /**< Shallow sleep mode */
    SLEEP_MODE_DEEP,    /**< Deep sleep mode */
} sleep_mode_t;

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Calculate normalized gain.
 *
 *  @param[in] gain_index  Current gain index.
 *  @param[in] rssi        Receiver signal strength indicator.
 *  @return Normalized gain in tenths of dB.
 */
static inline uint16_t calculate_normalized_gain(uint16_t min_db, uint8_t rssi)
{
    return (min_db + (GAIN_ENTRY_RANGE_TENTH_DB * (WEAKEST_SIGNAL_CODE - rssi) / WEAKEST_SIGNAL_CODE));
}

#endif /* LINK_UTILS_H_ */
