/** @file  wps_phy.c
 *  @brief The wps_phy module controle the physical layer for the dual radio mode.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_phy.h"

/* CONSTANTS ******************************************************************/
#define MULTI_RADIO_RSSI_HYSTERESIS 30
#define MULTI_RADIO_AVG_SAMPLE      4

/* PUBLIC GLOBALS *************************************************************/
wps_phy_multi_t wps_phy_multi;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool is_frame_done(phy_output_signal_t output_signal);
static bool is_frame_processing(phy_output_signal_t output_signal);

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_multi_radio_init(wps_multi_cfg_t multi_cfg)
{
    memset(&wps_phy_multi, 0, sizeof(wps_phy_multi));
    wps_phy_multi.timer_start      = multi_cfg.timer_start;
    wps_phy_multi.timer_stop       = multi_cfg.timer_stop;
    wps_phy_multi.timer_set_period = multi_cfg.timer_set_period;
}

void wps_phy_init(wps_phy_t *wps_phy, wps_phy_cfg_t *cfg)
{
    phy_init(wps_phy, cfg);
    wps_phy_multi.multi_radio.radios_lqi          = wps_phy_multi.lqi;
    wps_phy_multi.multi_radio.radio_count         = WPS_RADIO_COUNT;
    wps_phy_multi.multi_radio.hysteresis_tenth_db = MULTI_RADIO_RSSI_HYSTERESIS;
    wps_phy_multi.multi_radio.avg_sample_count    = MULTI_RADIO_AVG_SAMPLE;
}

void wps_phy_connect(wps_phy_t *wps_phy)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        uwb_set_timer_config(wps_phy->radio, SET_TIMER_CFG(TIMER_CFG_CLEAR,
                                                           AUTOWAKE_UP_DISABLE,
                                                           WAKE_UP_ONCE_ENABLE));
        uwb_set_radio_actions(wps_phy->radio, SET_RADIO_ACTIONS(RADIO_ACTIONS_CLEAR,
                                                                GO_TO_SLEEP,
                                                                INIT_TIMER_RESET_BOTH_WAKE_UP_TIMER));
        phy_connect(&wps_phy[i]);
    }

    wps_phy_multi.timer_start();
    wps_phy->radio->radio_hal->context_switch();
}

void wps_phy_disconnect(wps_phy_t *wps_phy)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_disconnect(&wps_phy[i]);
    }
    wps_phy_multi.timer_stop();
}

phy_output_signal_t wps_phy_get_main_signal(wps_phy_t *wps_phy)
{
    phy_output_signal_t leading_signal   = PHY_SIGNAL_YIELD;
    phy_output_signal_t following_signal = PHY_SIGNAL_YIELD;
    uint8_t radio_idx                    = wps_phy_multi.current_radio_idx;
    uint8_t leading_radio_idx            = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);

    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        if (i == leading_radio_idx) {
            leading_signal = phy_get_main_signal(&wps_phy[i]);
        } else {
            following_signal = phy_get_main_signal(&wps_phy[i]);
        }
    }

    if (is_frame_done(phy_get_main_signal(&wps_phy[radio_idx]))) {
        wps_phy[radio_idx].radio->radio_hal->disable_radio_dma_irq();
    }

    if (radio_idx == leading_radio_idx) {
        /* send the signal to process callback when the leading radio have finished to prepare the next frame */
        if (leading_signal == PHY_SIGNAL_PREPARE_DONE) {
            return leading_signal;
            /* send the end of frame signal when both radio frame processing is done.*/
        } else if (!is_frame_processing(leading_signal) && !is_frame_processing(following_signal)) {
            return leading_signal;
            /* Yield if one of both radio is still processing the frame. */
        } else {
            return PHY_SIGNAL_YIELD;
        }
    } else {
        /* Yield if one of both radio is still processing the frame. */
        if (is_frame_processing(leading_signal) || is_frame_processing(following_signal)) {
            return PHY_SIGNAL_YIELD;
        } else {
            return leading_signal;
        }
    }
    return leading_signal;
}

phy_output_signal_t wps_phy_get_auto_signal(wps_phy_t *wps_phy)
{
    uint8_t leading_radio_idx = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);

    return phy_get_auto_signal(&wps_phy[leading_radio_idx]);
}

void wps_phy_set_main_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer)
{
    uint8_t leading_radio_idx = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);

    wps_phy_multi.following_main_xlayer = *xlayer;

    if (xlayer->config.source_address == wps_phy->source_address) {
        wps_phy_multi.following_main_xlayer.frame.header_memory       = NULL;
        wps_phy_multi.following_main_xlayer.frame.header_begin_it     = NULL;
        wps_phy_multi.following_main_xlayer.frame.header_end_it       = NULL;
        wps_phy_multi.following_main_xlayer.frame.header_memory_size  = 0;
        wps_phy_multi.following_main_xlayer.frame.payload_memory      = NULL;
        wps_phy_multi.following_main_xlayer.frame.payload_begin_it    = NULL;
        wps_phy_multi.following_main_xlayer.frame.payload_end_it      = NULL;
        wps_phy_multi.following_main_xlayer.frame.payload_memory_size = 0;
    } else {
        wps_phy_multi.following_main_xlayer.config.expect_ack = false;
    }

    rf_channel_t *channel = xlayer->config.channel;

    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        if (i == leading_radio_idx) {
            xlayer->config.channel      = &channel[i];
            xlayer->config.rx_constgain = link_gain_loop_get_gain_value(&xlayer->config.gain_loop[i]);
            phy_set_main_xlayer(&wps_phy[i], xlayer);
        } else {
            wps_phy_multi.following_main_xlayer.config.channel = &channel[i];
            wps_phy_multi.following_main_xlayer.config.rx_constgain =
                link_gain_loop_get_gain_value(&xlayer->config.gain_loop[i]);
            phy_set_main_xlayer(&wps_phy[i], &wps_phy_multi.following_main_xlayer);
        }
    }
}

void wps_phy_set_auto_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer)
{
    if (xlayer == NULL) {
        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            phy_set_auto_xlayer(&wps_phy[i], NULL);
        }
        return;
    }

    uint8_t leading_radio_idx = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);

    if (xlayer->config.source_address == wps_phy->source_address) {
        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            if (i == leading_radio_idx) {
                phy_set_auto_xlayer(&wps_phy[i], xlayer);
            } else {
                phy_set_auto_xlayer(&wps_phy[i], NULL);
            }
        }
    } else {
        wps_phy_multi.following_auto_xlayer                   = *xlayer;
        wps_phy_multi.following_auto_xlayer.config.expect_ack = false;

        rf_channel_t *channel = xlayer->config.channel;

        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            if (i == leading_radio_idx) {
                xlayer->config.channel      = &channel[i];
                xlayer->config.rx_constgain = link_gain_loop_get_gain_value(&xlayer->config.gain_loop[i]);
                phy_set_auto_xlayer(&wps_phy[i], xlayer);
            } else {
                wps_phy_multi.following_auto_xlayer.config.channel = &channel[i];
                wps_phy_multi.following_auto_xlayer.config.rx_constgain =
                    link_gain_loop_get_gain_value(&xlayer->config.gain_loop[i]);
                phy_set_auto_xlayer(&wps_phy[i], &wps_phy_multi.following_auto_xlayer);
            }
        }
    }
}

void wps_phy_end_process(wps_phy_t *wps_phy)
{
    for(int radio_idx = 0; radio_idx < WPS_RADIO_COUNT; radio_idx++){
        uint8_t gain_index = link_gain_loop_get_gain_index(wps_phy[radio_idx].xlayer_main->config.gain_loop);

        link_lqi_update(&wps_phy_multi.multi_radio.radios_lqi[radio_idx],
                        gain_index,
                        wps_phy[radio_idx].xlayer_main->frame.frame_outcome,
                        wps_phy[radio_idx].xlayer_main->config.rssi_raw,
                        wps_phy[radio_idx].xlayer_main->config.rnsi_raw,
                        wps_phy[radio_idx].xlayer_main->config.phase_offset);

        /* Update gain loop */
        link_gain_loop_update(wps_phy[radio_idx].xlayer_main->frame.frame_outcome,
                            wps_phy[radio_idx].xlayer_main->config.rssi_raw,
                            wps_phy[radio_idx].xlayer_main->config.gain_loop);
    }

    link_multi_radio_update(&wps_phy_multi.multi_radio);
}

void wps_phy_multi_process_radio_timer(wps_phy_t *wps_phy)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        wps_phy[i].cfg.radio_actions |= SET_RADIO_ACTIONS(INIT_TIMER_RESET_BOTH_WAKE_UP_TIMER);
        uwb_set_radio_actions(wps_phy[i].radio, wps_phy[i].cfg.radio_actions);
    }
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        uwb_transfer_blocking(wps_phy[i].radio);
    }
}

void wps_phy_write_register(wps_phy_t *wps_phy, uint8_t starting_reg, uint8_t data)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_write_register(&wps_phy[i], starting_reg, data);
    }
}

void wps_phy_read_register(wps_phy_t *wps_phy, uint8_t target_register, uint8_t *rx_buffer, bool *xfer_cmplt)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_read_register(&wps_phy[i], target_register, rx_buffer, xfer_cmplt);
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
static bool is_frame_done(phy_output_signal_t output_signal)
{
    return (output_signal > PHY_SIGNAL_PREPARE_DONE);
}

static bool is_frame_processing(phy_output_signal_t output_signal)
{
    return (output_signal < PHY_SIGNAL_PREPARE_DONE);
}
