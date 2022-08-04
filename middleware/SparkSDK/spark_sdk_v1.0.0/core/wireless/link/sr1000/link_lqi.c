/** @file link_lqi.c
 *  @brief LQI module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_lqi.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void link_lqi_init(lqi_t *lqi, lqi_mode_t mode)
{
    memset(lqi, 0, sizeof(lqi_t));
    lqi->mode = mode;
}

uint8_t link_lqi_get_inst_phase_offset(lqi_t *lqi, uint8_t index)
{
    (void)lqi;
    (void)index;

    return 0;
}

uint32_t link_lqi_get_sent_count(lqi_t *lqi)
{
    return lqi->sent_count;
}

uint32_t link_lqi_get_ack_count(lqi_t *lqi)
{
    return lqi->ack_count;
}

uint32_t link_lqi_get_nack_count(lqi_t *lqi)
{
    return lqi->nack_count;
}

uint32_t link_lqi_get_received_count(lqi_t *lqi)
{
    return lqi->received_count;
}

uint32_t link_lqi_get_rejected_count(lqi_t *lqi)
{
    return lqi->rejected_count;
}

uint32_t link_lqi_get_lost_count(lqi_t *lqi)
{
    return lqi->lost_count;
}

uint32_t link_lqi_get_total_count(lqi_t *lqi)
{
    return lqi->total_count;
}

uint16_t link_lqi_get_avg_rssi_tenth_db(lqi_t *lqi)
{
    if (!lqi->total_count) {
        return 0;
    }
    switch (lqi->mode) {
            case LQI_MODE_0:
                return (lqi->rssi_total_tenth_db / lqi->total_count);
                break;
            case LQI_MODE_1:
                return (lqi->rssi_total_tenth_db / lqi->received_count);
                break;
            default:
                return (lqi->rssi_total_tenth_db / lqi->total_count);
    }
}

uint16_t link_lqi_get_avg_rnsi_tenth_db(lqi_t *lqi)
{
    if (!lqi->total_count) {
        return 0;
    }
    switch (lqi->mode) {
        case LQI_MODE_0:
            return (lqi->rnsi_total_tenth_db / lqi->total_count);
            break;
        case LQI_MODE_1:
            return (lqi->rnsi_total_tenth_db / lqi->received_count);
            break;
        default:
            return (lqi->rnsi_total_tenth_db / lqi->total_count);
    }
}

uint16_t link_lqi_get_inst_rnsi(lqi_t *lqi)
{
    return lqi->inst_rnsi;
}

uint16_t link_lqi_get_inst_rnsi_tenth_db(lqi_t *lqi)
{
    return lqi->inst_rnsi_tenth_db;
}

uint16_t link_lqi_get_inst_rssi(lqi_t *lqi)
{
    return lqi->inst_rssi;
}

uint16_t link_lqi_get_inst_rssi_tenth_db(lqi_t *lqi)
{
    return lqi->inst_rssi_tenth_db;
}
