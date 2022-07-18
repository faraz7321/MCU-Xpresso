/** @file  audio_src_cmsis.c
 *  @brief Sampling rate converter processing stage using the CMSIS DSP software library.
 *
 *  @note This processing stage requires an Arm Cortex-M processor based device.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "audio_src_cmsis.h"
#include <string.h>

/* CONSTANTS ******************************************************************/
#define FIR_NUMTAPS 24 /* Must be dividable by all audio_src_factor since fir phaseLength is NumTaps / ratio */

/* Third of initial sampling frequency */
static const q15_t fir_n24_c0_20_w_hamming[FIR_NUMTAPS] = {
    58, 29, -49, -223, -454, -577, -333, 495, 1933, 3725, 5388, 6391,
    6391, 5388, 3725, 1933, 495, -333, -577, -454, -223, -49, 29, 58
};
/* Same filter with coefficient multiplied by a factor of 3
 * Used to compensate for the gain loss due to the interpolation zero-stuffing.
 */
static const q15_t fir_n24_c0_20_w_hamming_x3_gain[FIR_NUMTAPS] = {
    174, 87, -147, -669, -1362, -1731, -999, 1485, 5799, 11175, 16164, 19173,
    19173, 16164, 11175, 5799, 1485, -999, -1731, -1362, -669, -147, 87, 174
};
/* Sixth of initial sampling frequency */
static const q15_t fir_n24_c0_10_w_hamming[FIR_NUMTAPS] = {
    -36, -17, 28, 139, 358, 707, 1185, 1760, 2368, 2930, 3363, 3599,
    3599, 3363, 2930, 2368, 1760, 1185, 707, 358, 139, 28, -17, -36
};
/* Same filter with coefficient multiplied by a factor of 6
 * Used to compensate for the gain loss due to the interpolation zero-stuffing.
 */
static const q15_t fir_n24_c0_10_w_hamming_x6_gain[FIR_NUMTAPS] = {
    -216, -102, 168, 834, 2148, 4242, 7110, 10560, 14208, 17580, 20178, 21594,
    21594, 20178, 17580, 14208, 10560, 7110, 4242, 2148, 834, 168, -102, -216
};

/* PUBLIC FUNCTIONS ***********************************************************/
void audio_src_cmsis_init(void *instance, mem_pool_t *mem_pool)
{
    int16_t *fir_state;
    const int16_t *fir_coeff;
    const int16_t *fir_coeff_interpolation;
    src_cmsis_instance_t *src_instance = (src_cmsis_instance_t *)instance;
    uint32_t block_size = src_instance->cfg.payload_size / (src_instance->cfg.bit_depth / 8);

    fir_state = mem_pool_malloc(mem_pool, sizeof(int16_t) * (FIR_NUMTAPS + block_size));

    switch (src_instance->cfg.ratio) {
    case AUDIO_SRC_SIX:
        fir_coeff = fir_n24_c0_10_w_hamming;
        fir_coeff_interpolation = fir_n24_c0_10_w_hamming_x6_gain;
        break;
    case AUDIO_SRC_THREE:
        fir_coeff = fir_n24_c0_20_w_hamming;
        fir_coeff_interpolation = fir_n24_c0_20_w_hamming_x3_gain;
        break;
    default:
        /* Invalid ratio */
        return;
    }

    if (src_instance->cfg.is_divider) {
        arm_fir_decimate_init_q15((arm_fir_decimate_instance_q15 *)(&src_instance->fir_instance),
                                  FIR_NUMTAPS, src_instance->cfg.ratio, fir_coeff, fir_state, block_size);
    } else {
        arm_fir_interpolate_init_q15((arm_fir_interpolate_instance_q15 *)(&src_instance->fir_instance),
                                     src_instance->cfg.ratio, FIR_NUMTAPS, fir_coeff_interpolation, fir_state, block_size);
    }
}

void audio_src_cmsis_deinit(void *instance)
{
    (void)instance;
}

uint32_t audio_src_cmsis_ctrl(void *instance, uint8_t cmd, uint32_t arg)
{
    (void)instance;
    (void)cmd;
    (void)arg;

    return 0;
}

uint16_t audio_src_cmsis_process(void *instance, sac_header_t *header,
                                 uint8_t *data_in, uint16_t bytes_count, uint8_t *data_out)
{
    (void)header;
    uint16_t sample_count_out          = 0;
    uint16_t sample_count_in           = bytes_count / sizeof(q15_t);
    src_cmsis_instance_t *src_instance = (src_cmsis_instance_t *)instance;

    if (src_instance->cfg.is_divider) {
        arm_fir_decimate_q15((arm_fir_decimate_instance_q15 *)(&src_instance->fir_instance),
                             (q15_t *)data_in, (q15_t *)data_out, bytes_count / sizeof(q15_t));
        sample_count_out = sample_count_in / src_instance->cfg.ratio;
    } else {
        arm_fir_interpolate_q15((arm_fir_interpolate_instance_q15 *)(&src_instance->fir_instance),
                                (q15_t *)data_in, (q15_t *)data_out, bytes_count / sizeof(q15_t));
        sample_count_out = sample_count_in * src_instance->cfg.ratio;
    }

    return (sample_count_out * sizeof(q15_t));
}
