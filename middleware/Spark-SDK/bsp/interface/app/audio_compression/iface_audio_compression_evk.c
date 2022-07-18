/** @file  iface_audio_compression_evk.c
 *  @brief This file contains the application-specific function implementation
 *         that calls the EVK1.4 BSP functions.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
 #include "iface_audio_compression.h"
 #include "evk.h"
 #include "max98091.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void iface_board_init(void)
{
    evk_init();
}

 void iface_audio_coord_init(void)
{
    evk_sai_config_t sai_config = {
        .rx_sai_mono_stereo = EVK_SAI_MODE_STEREO,
        .sai_bit_depth = EVK_SAI_BIT_DEPTH_16BITS
    };

    max98091_i2c_hal_t hal;
    hal.i2c_addr = MAX98091A_I2C_ADDR;
    hal.read = evk_audio_i2c_read_byte_blocking;
    hal.write = evk_audio_i2c_write_byte_blocking;

    evk_audio_i2c_init();
    evk_audio_sai_configuration(&sai_config);

    max98091_codec_cfg_t cfg = {
        .sampling_rate = MAX98091_AUDIO_48KHZ,
        .word_size = MAX98091_AUDIO_16BITS,
        .record_enabled = true,
        .playback_enabled = false,
        .record_filter_enabled = false,
        .playback_filter_enabled = false
    };
    max98091_init(&hal, &cfg);
}

void iface_audio_node_init(void)
{
    evk_sai_config_t sai_config = {
        .tx_sai_mono_stereo = EVK_SAI_MODE_STEREO,
        .sai_bit_depth = EVK_SAI_BIT_DEPTH_16BITS
    };

    max98091_i2c_hal_t hal;
    hal.i2c_addr = MAX98091A_I2C_ADDR;
    hal.read = evk_audio_i2c_read_byte_blocking;
    hal.write = evk_audio_i2c_write_byte_blocking;

    evk_audio_i2c_init();
    evk_audio_sai_configuration(&sai_config);

    max98091_codec_cfg_t cfg = {
        .sampling_rate = MAX98091_AUDIO_48KHZ,
        .word_size = MAX98091_AUDIO_16BITS,
        .record_enabled = false,
        .playback_enabled = true,
        .record_filter_enabled = false,
        .playback_filter_enabled = false
    };
    max98091_init(&hal, &cfg);
}

void iface_set_sai_complete_callback(void(*tx_callback)(void), void(*rx_callback)(void))
{
    evk_audio_set_sai_tx_dma_cplt_callback((irq_callback)tx_callback);
    evk_audio_set_sai_rx_dma_cplt_callback((irq_callback)rx_callback);
}

void iface_button_handling(void (*button1_callback)(void), void (*button2_callback)(void))
{
    static bool btn1_active;
    static bool btn2_active;

    if (btn1_active) {
        if (!evk_read_btn_state(BTN1)) {
            btn1_active = false;
        }
    }
    if (btn2_active) {
        if (!evk_read_btn_state(BTN2)) {
            btn2_active = false;
        }
    }
    if (!btn1_active && !btn2_active) {
        if (evk_read_btn_state(BTN1)) {
            if (button1_callback != NULL) {
                button1_callback();
            }
            btn1_active = true;
        } else if (evk_read_btn_state(BTN2)) {
            if (button2_callback != NULL) {
                button2_callback();
            }
            btn2_active = true;
        }
    }
}

void iface_tx_conn_status(void)
{
    evk_led_toggle(LED0);
}

void iface_rx_conn_status(void)
{
    evk_led_toggle(LED1);
}

void iface_fallback_status(bool on)
{
    if (on) {
        evk_led_on(LED2);
    } else {
        evk_led_off(LED2);
    }
}


