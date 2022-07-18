/** @file  audio_user_data.h
 *  @brief Processing stage used to append/extract 1 byte of user data to/from an audio payload.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef AUDIO_USER_DATA_H_
#define AUDIO_USER_DATA_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Audio User Data Command.
 */
typedef enum audio_user_data_cmd {
    AUDIO_USER_DATA_SEND_BYTE /*!< Append a data byte to the audio packet  */
} audio_user_data_cmd_t;

/** @brief Audio User Data Mode.
 */
typedef enum audio_user_data_mode {
    AUDIO_USER_DATA_TX, /*!< TX mode */
    AUDIO_USER_DATA_RX  /*!< RX mode */
} audio_user_data_mode_t;

/** @brief RX Audio User Data Instance.
 */
typedef struct rx_audio_user_data_instance {
    audio_user_data_mode_t mode;       /*!< Audio User Data mode */
    void (*rx_callback)(uint8_t data); /*!< Callback function to call when a data byte is received */
    bool _data_valid;                  /*!< Internal: Tells if the byte of data is valid */
    uint8_t _data;                     /*!< Internal: Byte of data */
} audio_user_data_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the user data processing stage.
 *
 *  @param[in] instance  User data instance.
 *  @param[in] mem_pool  Memory pool for memory allocation.
 */
void audio_user_data_init(void *instance, mem_pool_t *mem_pool);

/** @brief Deinitialize the user data processing stage.
 *
 *  @param[in] instance  User data instance.
 */
void audio_user_data_deinit(void *instance);

/** @brief Control the user data processing stage.
 *
 *  @param[in] instance  User data instance.
 *  @param[in] cmd       Control command.
 *  @param[in] arg       Control argument.
 *  @return Audio Compression Status.
 */
uint32_t audio_user_data_ctrl(void *instance, uint8_t cmd, uint32_t arg);

/** @brief Process the user data processing stage.
 *
 *  @param[in]  instance     User data instance.
 *  @param[in]  header       Audio header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed data out.
 *  @return Number of bytes processed.
 */
uint16_t audio_user_data_process(void *instance, sac_header_t *header,
                                 uint8_t *data_in, uint16_t size, uint8_t *data_out);


#ifdef __cplusplus
}
#endif

#endif /* AUDIO_USER_DATA_H_ */
