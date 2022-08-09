/** @file link_random_datarate_offset.c
 *  @brief Random datarate offset algorithm.
 *
 *  This algorithm is use for the concurrency to delay the sync value
 *  between device. It is use by the WPS Layer 2 internal connection.
 *  The output value of this algorithm is sent between device WPS.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_random_datarate_offset.h"

/* CONSTANTS ******************************************************************/
#define DEFAULT_ROLLOVER 15

/* PRIVATE FUNCTIONS PROTOTYPES ***********************************************/
static void update_offset(link_rdo_t *link_rdo);

/* PUBLIC FUNCTIONS ***********************************************************/
void link_rdo_init(link_rdo_t *link_rdo, uint16_t target_rollover_value)
{
    link_rdo->offset = 0;
    link_rdo->enabled = true;

    if (target_rollover_value == 0) {
        link_rdo->rollover_n = DEFAULT_ROLLOVER;
    } else {
        link_rdo->rollover_n = target_rollover_value;
    }
}

void link_rdo_send_offset(void *link_rdo, uint8_t *buffer_to_send)
{
    link_rdo_t *rdo_inst = (link_rdo_t *)link_rdo;

    update_offset(rdo_inst);
    if (buffer_to_send != NULL) {
        rdo_inst->offset_u8[0] = (rdo_inst->offset >> 8) & 0x00FF;
        rdo_inst->offset_u8[1] = (rdo_inst->offset) & 0x00FF;
        memcpy(buffer_to_send, rdo_inst->offset_u8, sizeof(uint16_t));
    }
}

void link_rdo_set_offset(void *link_rdo, uint8_t *buffer_to_received)
{
    link_rdo_t *rdo_inst = (link_rdo_t *)link_rdo;

    update_offset(rdo_inst);
    if (buffer_to_received != NULL) {
        memcpy(&rdo_inst->offset_u8, buffer_to_received, sizeof(uint16_t));
        rdo_inst->offset = (rdo_inst->offset_u8[0] << 8) | (rdo_inst->offset_u8[1]);
    }
}

uint16_t link_rdo_get_offset(link_rdo_t *link_rdo)
{
    return link_rdo->enabled ? link_rdo->offset : 0;
}

/* PRIVATE FUNCTIONS  *********************************************************/
/** @brief Update the RDO offset value.
 *
 *  @note This increment the offset value by 1
 *        and reset to 0 when the rollover value
 *        is met.
 *
 *  @param[in] link_rdo  RDO module instance.
 */
static void update_offset(link_rdo_t *link_rdo)
{
    link_rdo->offset = (link_rdo->offset + 1) % link_rdo->rollover_n;
}
