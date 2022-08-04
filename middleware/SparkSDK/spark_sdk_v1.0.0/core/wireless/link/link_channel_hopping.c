/** @file link_channel_hopping.c
 *  @brief Channel hopping module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdlib.h>
#include <string.h>
#include "link_channel_hopping.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool is_in_table(uint8_t *table, uint8_t size, uint8_t channel);
static void generate_freq_table(uint8_t *table, uint32_t *channels, uint8_t *channel_count, uint8_t size);
static void generate_random_hop_sequence(uint8_t *table_out, uint8_t *table_in, uint8_t channel_count, uint8_t seed);

/* PUBLIC FUNCTIONS ***********************************************************/
void link_channel_hopping_init(channel_hopping_t  *channel_hopping,
                               channel_sequence_t *channel_sequence,
                               bool                random_sequence_enabled,
                               uint8_t             random_sequence_seed)
{
    uint8_t freq_table1[MAX_CHANNEL_COUNT] = {0xff};
    uint8_t freq_table2[MAX_CHANNEL_COUNT] = {0xff};
    uint8_t channel_count;

    memset(channel_hopping, 0, sizeof(channel_hopping_t));
    channel_hopping->channel_sequence = channel_sequence;

    generate_freq_table(freq_table1, channel_hopping->channel_sequence->channel, &channel_count, channel_hopping->channel_sequence->sequence_size);

    if (random_sequence_enabled) {
        generate_random_hop_sequence(freq_table2, freq_table1, channel_count, random_sequence_seed);
    } else {
        for (uint8_t i = 0; i < channel_count; i++) {
            freq_table2[i] = freq_table1[i];
        }
    }

    generate_freq_table(freq_table1, channel_hopping->channel_sequence->channel, &channel_count, channel_hopping->channel_sequence->sequence_size);

    for (uint8_t i = 0; i < MAX_CHANNEL_COUNT; i++) {
        channel_hopping->channel_lookup_table[freq_table1[i]] = freq_table2[i];
    }
}

void link_channel_hopping_increment_sequence(channel_hopping_t *channel_hopping, uint8_t increment)
{
    for (uint8_t i = 0; i < increment; i++) {
        channel_hopping->hop_seq_index = (channel_hopping->hop_seq_index + 1) % channel_hopping->channel_sequence->sequence_size;
    }
}

void link_channel_hopping_set_seq_index(channel_hopping_t *channel_hopping, uint8_t seq_index)
{
    channel_hopping->hop_seq_index = seq_index;
}

uint8_t link_channel_hopping_get_seq_index(channel_hopping_t *channel_hopping)
{
    return channel_hopping->hop_seq_index;
}

uint32_t link_channel_hopping_get_channel(channel_hopping_t *channel_hopping)
{
    return channel_hopping->channel_lookup_table[channel_hopping->channel_sequence->channel[channel_hopping->hop_seq_index]];
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Determine if the channel provided is in the table.
 *
 *  @param[in] table    The table.
 *  @param[in] size     The size of the table.
 *  @param[in] channel  The channel.
 *  @return is_in_table.
 */
static bool is_in_table(uint8_t *table, uint8_t size, uint8_t channel)
{
    for (uint8_t i = 0; i < size; i++) {
        if (table[i] == channel) {
            return true;
        }
    }

    return false;
}

/** @brief Generate initial frequency table.
 *
 *  @param[in] table          The frequency table to be generated.
 *  @param[in] channels       The channel table.
 *  @param[in] channel_count  The number of channels counted inside the sequence.
 *  @param[in] size           The size of the channel sequence.
 */
static void generate_freq_table(uint8_t *table, uint32_t *channels, uint8_t *channel_count, uint8_t size)
{
    *channel_count = 0;
    for (uint8_t i = 0; i < size; i++) {
        if (!is_in_table(table, MAX_CHANNEL_COUNT, channels[i])) {
            table[*channel_count] = channels[i];
            (*channel_count)++;
        }
    }
}

/** @brief Generate random channel hopping sequence.
 *
 *  @param[in] table_out      The output table.
 *  @param[in] table_in       The input table.
 *  @param[in] channel_count  The number of channels counted inside the sequence.
 *  @param[in] seed           The randomization seed.
 */
static void generate_random_hop_sequence(uint8_t *table_out, uint8_t *table_in, uint8_t channel_count, uint8_t seed)
{
    uint8_t rand_num;

    srand(seed + 2); /* Avoid seed value 1 which reset the seed by adding 2*/
    for (uint8_t i = 0; i < channel_count; i++) {
        rand_num = rand() % (channel_count - i);
        table_out[i] = table_in[rand_num];
        for (uint8_t j = rand_num; j <  channel_count; j++) {
            table_in[j] = table_in[j + 1];
        }
    }
}
