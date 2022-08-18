/** @file  wps_phy.c
 *  @brief The wps_phy module control the physical layer for the single radio mode.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_phy.h"

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
void wps_phy_init(wps_phy_t *wps_phy, wps_phy_cfg_t *cfg)
{
    phy_init(wps_phy, cfg);
}
#include "fsl_debug_console.h"
void wps_phy_connect(wps_phy_t *wps_phy)
{
    uwb_set_radio_actions(wps_phy->radio, SET_RADIO_ACTIONS(RADIO_ACTIONS_CLEAR));
    uwb_transfer_blocking(wps_phy->radio);

    while (!(*wps_phy->pwr_status_cmd & BIT_AWAKE)) {
        wps_phy->pwr_status_cmd = uwb_read_register_8(wps_phy->radio, REG_PWRSTATUS);
        uwb_transfer_blocking(wps_phy->radio);
    }

    uwb_set_timer_config(wps_phy->radio, SET_TIMER_CFG(TIMER_CFG_CLEAR,
                                                       AUTOWAKE_UP_ENABLE,
                                                       WAKE_UP_ONCE_ENABLE));
    //PRINTF("before phy_connect()");
    phy_connect(wps_phy);
    //PRINTF("after phy_connect()");
    wps_phy->radio->radio_hal->context_switch();
    
    //PRINTF("after context_switch()");
}

void wps_phy_disconnect(wps_phy_t *wps_phy)
{
    phy_disconnect(wps_phy);
}

phy_output_signal_t wps_phy_get_main_signal(wps_phy_t *wps_phy)
{
    return phy_get_main_signal(wps_phy);
}

phy_output_signal_t wps_phy_get_auto_signal(wps_phy_t *wps_phy)
{
    return phy_get_auto_signal(wps_phy);
}

void wps_phy_set_main_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer)
{
    xlayer->config.rx_constgain = link_gain_loop_get_gain_value(xlayer->config.gain_loop);
    phy_set_main_xlayer(wps_phy, xlayer);
}

void wps_phy_set_auto_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer)
{
    if (xlayer != NULL) {
        xlayer->config.rx_constgain = link_gain_loop_get_gain_value(xlayer->config.gain_loop);
    }
    phy_set_auto_xlayer(wps_phy, xlayer);
}

void wps_phy_end_process(wps_phy_t *wps_phy)
{
    link_gain_loop_update(wps_phy->xlayer_main->frame.frame_outcome,
                            wps_phy->xlayer_main->config.rssi_raw,
                            wps_phy->xlayer_main->config.gain_loop);
}

void wps_phy_write_register(wps_phy_t *wps_phy, uint8_t starting_reg, uint8_t data)
{
    phy_write_register(wps_phy, starting_reg, data);
}

void wps_phy_read_register(wps_phy_t *wps_phy, uint8_t target_register, uint8_t *rx_buffer, bool *xfer_cmplt)
{
    phy_read_register(wps_phy, target_register, rx_buffer, xfer_cmplt);
}