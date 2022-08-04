/** @file  audio_user_data.c
 *  @brief Processing stage used to append/extract 1 byte of user data to/from an audio payload.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "audio_user_data.h"

/* PRIVATE FUNCTION PROTOTYPES *************************************************/
static void process_rx_data(audio_user_data_instance_t *instance, sac_header_t *header, uint8_t *data_in, uint16_t size);

static void process_tx_data(audio_user_data_instance_t *instance, sac_header_t *header, uint8_t *data_in, uint16_t size);

/* PUBLIC FUNCTIONS ***********************************************************/
void audio_user_data_init(void *instance, mem_pool_t *mem_pool)
{
    (void)mem_pool;

    audio_user_data_instance_t *inst = (audio_user_data_instance_t *)instance;

    inst->_data_valid = false;
}

void audio_user_data_deinit(void *instance)
{
    (void)instance;
}

uint32_t audio_user_data_ctrl(void *instance, uint8_t cmd, uint32_t arg)
{
    audio_user_data_instance_t *inst = (audio_user_data_instance_t *)instance;

    switch (cmd) {
    case AUDIO_USER_DATA_SEND_BYTE:
        inst->_data = (uint8_t)arg;
        inst->_data_valid = true;
        break;
    }

    return 0;
}

uint16_t audio_user_data_process(void *instance, sac_header_t *header,
                                 uint8_t *data_in, uint16_t size, uint8_t *data_out)
{
    (void)data_out;
    audio_user_data_instance_t *inst = (audio_user_data_instance_t *)instance;

    if (inst->mode == AUDIO_USER_DATA_RX) {
        process_rx_data(inst, header, data_in, size);
    } else { /* AUDIO_USER_DATA_TX */
        process_tx_data(inst, header, data_in, size);
    }

    return 0;
}

/* PRIVATE FUNCTIONS ***********************************************************/
static void process_rx_data(audio_user_data_instance_t *instance, sac_header_t *header, uint8_t *data_in, uint16_t size)
{
    instance->_data = data_in[size];
    if (header->user_data_is_valid && instance->rx_callback) {
        instance->rx_callback(data_in[size]);
    }
}

static void process_tx_data(audio_user_data_instance_t *instance, sac_header_t *header, uint8_t *data_in, uint16_t size)
{
    if (instance->_data_valid) {
        data_in[size] = instance->_data;
        header->user_data_is_valid = 1;
    } else {
        header->user_data_is_valid = 0;
    }

    instance->_data_valid = false;
}
