/** @file link_protocol.c
 *  @brief WPS layer 2 internal connection protocol.
 *
 *  This file is a wrapper use to send/received payload
 *  through the WPS MAC internal connection. Its used to
 *  properly generate a complete packet regrouping one
 *  or multiple information.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
/* INCLUDES *******************************************************************/
#include "link_protocol.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void link_protocol_init(link_protocol_t *link_protocol, link_protocol_init_cfg_t *link_protocol_cfg, link_error_t *err)
{
    *err = LINK_PROTO_NO_ERROR;
    link_protocol->max_buffer_size = link_protocol_cfg->buffer_size;
    link_protocol->current_number_of_protocol = 0;
    link_protocol->current_buffer_offset = 0;
    link_protocol->index = 0;
}

void link_protocol_add(link_protocol_t *link_protocol, link_protocol_cfg_t *protocol_cfg, link_error_t *err)
{
    *err = LINK_PROTO_NO_ERROR;
    if (link_protocol->current_number_of_protocol < MAX_NUMBER_OF_PROTOCOL) {
        if (link_protocol->current_buffer_offset < (link_protocol->max_buffer_size)) {
            link_protocol->current_number_of_protocol++;
            link_protocol->protocol_info[link_protocol->current_number_of_protocol - 1].index   = link_protocol->current_buffer_offset;
            link_protocol->protocol_info[link_protocol->current_number_of_protocol - 1].instance = protocol_cfg->instance;
            link_protocol->protocol_info[link_protocol->current_number_of_protocol - 1].size     = protocol_cfg->size;
            link_protocol->protocol_info[link_protocol->current_number_of_protocol - 1].send     = protocol_cfg->send;
            link_protocol->protocol_info[link_protocol->current_number_of_protocol - 1].receive  = protocol_cfg->receive;
            link_protocol->current_buffer_offset += protocol_cfg->size;

        } else {
            *err = LINK_PROTO_NO_MORE_SPACE;
        }
    } else {
        *err = LINK_PROTO_TOO_MANY_PROTO;
    }
}

void link_protocol_send_buffer(void *link_protocol, uint8_t *buffer_to_send, uint32_t *size)
{
    link_protocol_t *link_proto = (link_protocol_t *)link_protocol;
    uint32_t size_to_send       = 0;

    for (uint8_t i = 0; i < link_proto->current_number_of_protocol; i++) {
        link_proto->protocol_info[i].send(link_proto->protocol_info[i].instance,
                                          buffer_to_send + size_to_send);
        size_to_send += link_proto->protocol_info[i].size;
    }

    *size           = size_to_send;
}

void link_protocol_receive_buffer(void *link_protocol, uint8_t *received_buffer, uint32_t size)
{
    link_protocol_t *link_proto = (link_protocol_t *)link_protocol;
    uint32_t current_proto_size = 0;

    if (current_proto_size <= size) {
        for (uint8_t i = 0; i < link_proto->current_number_of_protocol; i++) {
            link_proto->protocol_info[i].receive(link_proto->protocol_info[i].instance,
                                                received_buffer + current_proto_size);
            current_proto_size += link_proto->protocol_info[i].size;
        }
    }

}
