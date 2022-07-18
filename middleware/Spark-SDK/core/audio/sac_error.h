/** @file  sac_error.h
 *  @brief SPARK Audio Core error codes.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_ERROR_H_
#define SAC_ERROR_H_

/* TYPES **********************************************************************/
/** @brief Audio Errors.
 */
typedef enum sac_error {
    SAC_ERR_NONE = 0,                  /*!< No error occurred */
    SAC_ERR_NOT_ENOUGH_MEMORY,         /*!< Not enough memory is allocated by the application
                                            for a full audio core initialization */
    SAC_ERR_PROC_STAGE_LIMIT_REACHED,  /*!< Maximum number of processing stages for a given audio pipeline
                                            is already reached when trying to add another one */
    SAC_ERR_PRODUCER_Q_FULL,           /*!< Producer's queue is full when trying to produce */
    SAC_ERR_CONSUMER_Q_EMPTY,          /*!< Consumer's queue is empty when trying to consume */
    SAC_ERR_BUFFERING_NOT_COMPLETE,    /*!< Initial buffering is not completed when trying to consume */
    SAC_ERR_NO_SAMPLES_TO_PROCESS,     /*!< Producer's queue is empty when trying to process */
    SAC_ERR_CDC_INIT_FAILURE,          /*!< An error occurred during the clock drift compensation
                                            module initialization */
    SAC_ERR_PIPELINE_CFG_INVALID,      /*!< Pipeline configuration is invalid */
    SAC_ERR_NULL_PTR,                  /*!< A pointer is NULL while it should have been initialized */
    SAC_ERR_MIXER_INIT_FAILURE         /*!< An error occurred during the mixer module initialization */
} sac_error_t;


#endif /* SAC_ERROR_H_ */
