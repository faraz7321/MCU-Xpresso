/** @file link_channel_hopping.h
 *  @brief Channel hopping module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_CHANNEL_HOPPING_H_
#define LINK_CHANNEL_HOPPING_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define MAX_CHANNEL_COUNT 5

/* TYPES **********************************************************************/
typedef struct channel_sequence {
    uint32_t* channel;
    uint32_t  sequence_size;
} channel_sequence_t;

typedef struct channel_hopping {
    uint8_t hop_seq_index;                           /**< The index of the current channel */
    channel_sequence_t *channel_sequence;            /**< The channel hopping sequence */
    uint8_t channel_lookup_table[MAX_CHANNEL_COUNT]; /**< Channel lookup table to randomize channel hopping sequence */
} channel_hopping_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize channel hopping object.
 *
 *  @param[in] channel_hopping          Channel hopping object.
 *  @param[in] channel_sequence         The channel hopping sequence table.
 *  @param[in] random_sequence_enabled  Random sequence enable flag.
 *  @param[in] random_sequence_seed     Random sequence seed.
 */
void link_channel_hopping_init(channel_hopping_t  *channel_hopping,
                               channel_sequence_t *channel_sequence,
                               bool                random_sequence_enabled,
                               uint8_t             random_sequence_seed);

/** @brief Increment channel hopping sequence index.
 *
 *  @param[in] channel_hopping  Channel hopping object.
 */
void link_channel_hopping_increment_sequence(channel_hopping_t *channel_hopping, uint8_t increment);

/** @brief Set current channel hopping sequence index.
 *
 *  @param[in] channel_hopping  Channel hopping object.
 *  @param[in] seq_index        The desired channel hopping sequence index.
 */
void link_channel_hopping_set_seq_index(channel_hopping_t *channel_hopping,
                                        uint8_t            seq_index);

/** @brief Get current channel hopping sequence index.
 *
 *  @param[in] channel_hopping  Channel hopping object.
 *  @return The current channel hopping sequence index.
 */
uint8_t link_channel_hopping_get_seq_index(channel_hopping_t *channel_hopping);

/** @brief Get current channel.
 *
 *  @param[in] channel_hopping  Channel hopping object.
 *  @return The current channel.
 */
uint32_t link_channel_hopping_get_channel(channel_hopping_t *channel_hopping);

#ifdef __cplusplus
}
#endif
#endif /* LINK_CHANNEL_HOPPING_H_ */
