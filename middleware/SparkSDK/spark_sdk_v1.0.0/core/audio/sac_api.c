/** @file  audio_api.c
 *  @brief SPARK Audio Core Application Programming Interface.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_api.h"
#include <string.h>
#include "audio_cdc_module.h"
#include "audio_fallback_module.h"

/* CONSTANTS ******************************************************************/
#define CDC_QUEUE_DATA_SIZE_INFLATION      (SAC_MAX_CHANNEL_COUNT * AUDIO_32BITS_BYTE)
#define PROD_QUEUE_SIZE_MIN_WHEN_ENQUEUING 0
#define TX_QUEUE_HIGH_LEVEL                2

/* PRIVATE GLOBALS ************************************************************/
static mem_pool_t mem_pool;
static audio_mixer_module_t *audio_mixer_module;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_audio_queues(sac_pipeline_t *pipeline, sac_error_t *err);
static void init_audio_free_queue(sac_endpoint_t *endpoint, const char *queue_name,
                                  uint16_t queue_data_size, uint8_t queue_size, sac_error_t *err);
static void move_audio_packet_to_consumer_queue(sac_pipeline_t *pipeline, queue_node_t *node);
static queue_node_t *process_samples(sac_pipeline_t *pipeline, queue_node_t *node);
static void enqueue_producer_node(sac_endpoint_t *producer, sac_error_t *err);
static uint16_t produce(sac_pipeline_t *pipeline, sac_error_t *err);
static uint16_t consume(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_error_t *err);
static void consume_no_delay(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_error_t *err);
static void consume_delay(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_error_t *err);
static void validate_pipeline_config(sac_pipeline_t *pipeline, sac_error_t *err);
static queue_node_t *start_mixing_process(sac_pipeline_t *pipeline, queue_node_t *node);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_init(queue_critical_cfg_t queue_critical, uint8_t *queue_memory, uint32_t queue_memory_size)
{
    queue_init(queue_critical);
    mem_pool_init(&mem_pool, queue_memory, (size_t)queue_memory_size);
}

void sac_mixer_module_init(audio_mixer_module_cfg_t cfg, sac_error_t *err)
{
    *err = SAC_ERR_NONE;

    audio_mixer_module = audio_mixer_module_init(cfg, &mem_pool, err);
}

void sac_fallback_module_init(sac_fallback_module_cfg_t cfg, sac_error_t *err)
{
    *err = SAC_ERR_NONE;

    audio_fallback_module_init(cfg, &mem_pool, err);
}

sac_pipeline_t *sac_pipeline_init(const char *name, sac_endpoint_t *producer,
                                  sac_pipeline_cfg_t cfg, sac_endpoint_t *consumer, sac_error_t *err)
{
    *err = SAC_ERR_NONE;

    sac_pipeline_t *pipeline = (sac_pipeline_t *)mem_pool_malloc(&mem_pool, sizeof(sac_pipeline_t));
    if (pipeline == NULL) {
        *err = SAC_ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    pipeline->name = name;
    pipeline->producer = producer;
    pipeline->consumer = consumer;
    pipeline->cfg = cfg;

    return pipeline;
}

sac_endpoint_t *sac_endpoint_init(void *instance, const char *name, sac_endpoint_interface_t iface, audio_endpoint_cfg_t cfg, sac_error_t *err)
{
    *err = SAC_ERR_NONE;

    sac_endpoint_t *endpoint = (sac_endpoint_t *)mem_pool_malloc(&mem_pool, sizeof(sac_endpoint_t));
    if (endpoint == NULL) {
        *err = SAC_ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    queue_t *queue = (queue_t *)mem_pool_malloc(&mem_pool, sizeof(queue_t));
    if (queue == NULL) {
        *err = SAC_ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    endpoint->instance = instance;
    endpoint->name = name;
    endpoint->iface = iface;
    endpoint->cfg = cfg;
    endpoint->next_endpoint = NULL;
    endpoint->_queue = queue;
    endpoint->_free_queue = NULL;
    endpoint->_current_node = NULL;
    endpoint->_buffering_complete = false;

    return endpoint;
}

void sac_endpoint_link(sac_endpoint_t *consumer, sac_endpoint_t *producer, sac_error_t *err)
{
    *err = SAC_ERR_NONE;

    if (consumer == NULL || producer == NULL) {
        *err = SAC_ERR_NULL_PTR;
    } else {
        producer->_queue = consumer->_queue;
        producer->_free_queue = consumer->_free_queue;
    }
}

sac_processing_t *sac_processing_stage_init(void *instance, const char *name, sac_processing_interface_t iface, sac_error_t *err)
{
    *err = SAC_ERR_NONE;

    sac_processing_t *process = (sac_processing_t *)mem_pool_malloc(&mem_pool, sizeof(sac_processing_t));
    if (process == NULL) {
        *err = SAC_ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    process->instance = instance;
    process->name = name;
    process->iface = iface;

    return process;
}

void sac_pipeline_add_processing(sac_pipeline_t *pipeline, sac_processing_t *process)
{
    sac_processing_t *current_process = pipeline->process;

    if (current_process == NULL) {
        pipeline->process = process;
        return;
    }

    /* Find the last processing stage in the chain */
    while (current_process->next_process != NULL) {
        current_process = current_process->next_process;
    }

    current_process->next_process = process;
}

void sac_pipeline_add_extra_consumer(sac_pipeline_t *pipeline, sac_endpoint_t *next_consumer)
{
    sac_endpoint_t *current_consumer = pipeline->consumer;

    /* Find the last consumer in the chain */
    while (current_consumer->next_endpoint != NULL) {
        current_consumer = current_consumer->next_endpoint;
    }

    current_consumer->next_endpoint = next_consumer;
}

void sac_pipeline_add_extra_producer(sac_pipeline_t *pipeline, sac_endpoint_t *next_producer)
{
    sac_endpoint_t *current_producer = pipeline->producer;

    /* Find the last producer in the chain */
    while (current_producer->next_endpoint != NULL) {
        current_producer = current_producer->next_endpoint;
    }

    current_producer->next_endpoint = next_producer;
}

void sac_pipeline_setup(sac_pipeline_t *pipeline, sac_error_t *err)
{
    sac_processing_t *process = pipeline->process;
    sac_endpoint_t *consumer = pipeline->consumer;
    sac_endpoint_t *producer = pipeline->producer;

    pipeline->_user_data_size = (pipeline->cfg.user_data_enable) ? 1 : 0;
    validate_pipeline_config(pipeline, err);
    if (*err != SAC_ERR_NONE) {
        return;
    }

    /* Initialize processing stages */
    while (process != NULL) {
        if (process->iface.init != NULL) {
            process->iface.init(process->instance, &mem_pool);
        }
        process = process->next_process;
    }

    /* Initialize audio queues */
    init_audio_queues(pipeline, err);
    if (*err != SAC_ERR_NONE) {
        return;
    }

    /* Initialize stats */
    pipeline->_statistics.producer_buffer_size = queue_get_limit(producer->_queue);
    pipeline->_statistics.consumer_buffer_size = queue_get_limit(consumer->_queue);
    sac_pipeline_reset_stats(pipeline);

    /* Initialize CDC */
    if (pipeline->cfg.cdc_enable) {
        audio_cdc_module_init(pipeline, &mem_pool, err);
        if (*err != SAC_ERR_NONE) {
            return;
        }
    }
}

void sac_pipeline_produce(sac_pipeline_t *pipeline, sac_error_t *err)
{
    sac_endpoint_t *producer = pipeline->producer;
    uint16_t size;

    *err = SAC_ERR_NONE;

    if (pipeline->producer->cfg.delayed_action) {
        if (producer->_current_node != NULL) {
            /* Enqueue previous node */
            enqueue_producer_node(producer, err);
            if (*err != SAC_ERR_NONE) {
                return;
            }
        }
        /* Start production of next node */
        produce(pipeline, err);
        if (*err != SAC_ERR_NONE) {
            return;
        }
    } else {
        /* Start production of next node */
        size = produce(pipeline, err);
        if (*err != SAC_ERR_NONE) {
            return;
        }
        if (size > 0) {
            /* Endpoint produced the node, so enqueue it */
            enqueue_producer_node(producer, err);
            if (*err != SAC_ERR_NONE) {
                return;
            }
        } else {
            /* Error: producer returned no data, so free the current node */
            queue_free_node(producer->_current_node);
            producer->_current_node = NULL;
            pipeline->_statistics.producer_packets_corrupted_count++;

        }
    }
}

void sac_pipeline_consume(sac_pipeline_t *pipeline, sac_error_t *err)
{
    sac_endpoint_t *consumer = pipeline->consumer;

    if (consumer->cfg.delayed_action) {
        consume_delay(pipeline, consumer, err);
    } else {
        do {
            if (queue_get_length(consumer->_queue) > 0) {
                consume_no_delay(pipeline, consumer, err);
            }
            consumer = consumer->next_endpoint;
        } while (consumer != NULL);
    }
}

sac_statistics_t *sac_pipeline_get_stats(sac_pipeline_t *pipeline)
{
    pipeline->_statistics.producer_buffer_load = pipeline->producer->_queue->length;
    pipeline->_statistics.consumer_buffer_load = pipeline->consumer->_queue->length;

    return &pipeline->_statistics;
}

uint32_t sac_pipeline_get_producer_buffer_load(sac_pipeline_t *pipeline)
{
    return pipeline->producer->_queue->length;
}

uint32_t sac_pipeline_get_consumer_buffer_load(sac_pipeline_t *pipeline)
{
    return pipeline->consumer->_queue->length;
}

uint32_t sac_pipeline_get_consumer_buffer_overflow_count(sac_pipeline_t *pipeline)
{
    return pipeline->_statistics.consumer_buffer_overflow_count;
}

uint32_t sac_pipeline_get_consumer_buffer_underflow_count(sac_pipeline_t *pipeline)
{
    return pipeline->_statistics.consumer_buffer_underflow_count;
}

uint32_t sac_pipeline_get_producer_packets_corrupted_count(sac_pipeline_t *pipeline)
{
    return pipeline->_statistics.producer_packets_corrupted_count;
}

void sac_pipeline_reset_stats(sac_pipeline_t *pipeline)
{
    uint32_t consume_size;
    uint32_t produce_size;

    produce_size = pipeline->_statistics.producer_buffer_size;
    consume_size = pipeline->_statistics.consumer_buffer_size;

    memset(&pipeline->_statistics, 0, sizeof(sac_statistics_t));

    pipeline->_statistics.producer_buffer_size = produce_size;
    pipeline->_statistics.consumer_buffer_size = consume_size;
}

void sac_pipeline_start(sac_pipeline_t *pipeline)
{
    pipeline->_buffering_threshold = (pipeline->cfg.do_initial_buffering) ?
                                      pipeline->consumer->cfg.queue_size : 1;

    /* Start producing samples */
    pipeline->producer->iface.start(pipeline->producer->instance);
}

void sac_pipeline_stop(sac_pipeline_t *pipeline)
{
    sac_endpoint_t *consumer = pipeline->consumer;

    /* Stop endpoints */
    do {
        pipeline->consumer->iface.stop(consumer->instance);
        consumer = consumer->next_endpoint;
    } while (consumer != NULL);

    pipeline->producer->iface.stop(pipeline->producer->instance);

    /* Free current node */
    queue_free_node(pipeline->producer->_current_node);
    pipeline->producer->_current_node = NULL;
}

uint32_t sac_processing_ctrl(sac_processing_t *audio_processing, uint8_t cmd, uint32_t arg)
{
    return audio_processing->iface.ctrl(audio_processing->instance, cmd, arg);
}

void sac_pipeline_process(sac_pipeline_t *pipeline, sac_error_t *err)
{
    queue_node_t *node1, *node2, *temp_node;
    sac_endpoint_t *consumer = pipeline->consumer;
    sac_endpoint_t *producer = pipeline->producer;
    uint8_t crc;

    *err = SAC_ERR_NONE;

    /* Prevent the mixer to make the buffering before the mixing */
    if ((!pipeline->cfg.mixer_option.input_mixer_pipeline) &&
        (!pipeline->cfg.mixer_option.output_mixer_pipeline)) {
        do {
            if (!consumer->_buffering_complete) {
                if (queue_get_length(consumer->_queue) >= (pipeline->_buffering_threshold)) {
                    /* Buffering threshold reached */
                    consumer->_buffering_complete = true;
                    consumer->iface.start(consumer->instance);
                }
            }
            consumer = consumer->next_endpoint;
        } while (consumer != NULL);
    }

    /* If it's a Mixing Pipeline get the mixed packet of all Output Producer Endpoints.
     * Otherwise, get the packet from the single producer endpoints.
     */
    if (pipeline->cfg.mixer_option.output_mixer_pipeline) {
        temp_node = queue_get_free_node(consumer->_free_queue);
        node1 = start_mixing_process(pipeline, temp_node);
    } else {
        /* Get a node with audio samples that need processing from the producer queue */
        node1 = queue_dequeue_node(pipeline->producer->_queue);
        if (node1 == NULL) {
            *err = SAC_ERR_NO_SAMPLES_TO_PROCESS;
            return;
        }
    }

    /*
     * Check if payload size in audio header is what is expected. If not, packet may have
     * been corrupted. In which case, set it to expected value to avoid queue node overflow
     * when using this packet as data source for memcpy().
     */
    if (producer->cfg.use_encapsulation) {
        crc = sac_node_get_header(node1)->crc4;
        sac_node_get_header(node1)->crc4 = 0;
        sac_node_get_header(node1)->reserved = 0;
        if (crc4itu(0, (uint8_t *)sac_node_get_header(node1), sizeof(sac_header_t)) != crc) {
            /* Audio packet is corrupted, set it to a known value */
            sac_node_set_payload_size(node1, producer->cfg.audio_payload_size);
            sac_node_get_header(node1)->fallback = 0;
            sac_node_get_header(node1)->tx_queue_level_high = 0;
            sac_node_get_header(node1)->user_data_is_valid = 0;
            pipeline->_statistics.producer_packets_corrupted_count++;
        }
    }

    if (pipeline->process != NULL) {
        /* Apply all processing stages on audio packet */
        node2 = process_samples(pipeline, node1);
    } else {
        /* No processing to be done */
        node2 = node1;
    }

    if (pipeline->consumer->cfg.use_encapsulation) {
        node1 = node2;
    } else {
        /* Consumer takes only audio data */
        if (pipeline->cfg.cdc_enable) {
            /* Calculate average queue length */
            audio_cdc_module_update_queue_avg(pipeline);
            /* Apply Clock Drift Compensation */
            node1 = audio_cdc_module_process(pipeline, node2, err);
        } else {
            node1 = node2;
        }
    }

    move_audio_packet_to_consumer_queue(pipeline, node1);

    /*
     * Start the Mixer Output Pipeline as soon as the first mixed audio packet is ready.
     * The Mixer Output Pipeline consumer will never be stopped after this point
     * since the mixer will always produce audio packets to be consumed.
     */
    if (pipeline->cfg.mixer_option.output_mixer_pipeline) {
        if (consumer->_buffering_complete == false) {
            consumer->_buffering_complete = true;
            consumer->iface.start(consumer->instance);
        }
    }
    queue_free_node(node1);
}

uint32_t sac_get_allocated_bytes(void)
{
    return mem_pool_get_allocated_bytes(&mem_pool);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize audio queues.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] err       Error code.
 */
static void init_audio_queues(sac_pipeline_t *pipeline, sac_error_t *err)
{
    sac_endpoint_t *consumer = pipeline->consumer;
    sac_endpoint_t *producer = pipeline->producer;
    uint16_t queue_data_inflation_size;
    uint16_t queue_data_size;
    uint8_t queue_size;

    /* Calculate required queue data inflation size  */
    queue_data_inflation_size = SAC_NODE_PAYLOAD_SIZE_VAR_SIZE;
    queue_data_inflation_size += sizeof(sac_header_t);
    queue_data_inflation_size += pipeline->_user_data_size;
    if (pipeline->cfg.cdc_enable) {
        queue_data_inflation_size += CDC_QUEUE_DATA_SIZE_INFLATION;
    }

    /* Calculate producer initial queue data size  */
    if (consumer->cfg.audio_payload_size > producer->cfg.audio_payload_size) {
        /* If consumer queue is bigger than producer queue,
           then the audio processing will require more space than initial size */
        queue_data_size = consumer->cfg.audio_payload_size;
    } else {
        queue_data_size = producer->cfg.audio_payload_size;
    }
    queue_data_size += queue_data_inflation_size;
    queue_data_size += sac_align_data_size(queue_data_size, uint32_t); /* Align nodes on 32bits */

    /* Initialize producer queue.
     * The Mixer Output Pipeline producer queue will be initialized in
     * sac_endpoint_link() as it shares the Mixer Input Pipeline consumer queue.
     */
    if (!pipeline->cfg.mixer_option.output_mixer_pipeline) {
        queue_size = producer->cfg.queue_size;
        init_audio_free_queue(producer, "Processing Free Queue",
                              queue_data_size, queue_size, err);
        if (*err != SAC_ERR_NONE) {
            return;
        }
        if (producer->cfg.delayed_action) {
            /* When using delayed action, one of the node will not be available */
            queue_size--;
        }
        queue_init_queue(producer->_queue, queue_size, "Processing Queue");
    }

    /* Calculate producer initial queue data size  */
    queue_data_size = consumer->cfg.audio_payload_size;
    queue_data_size += queue_data_inflation_size;
    queue_data_size += sac_align_data_size(queue_data_size, uint32_t); /* Align nodes on 32bits */

    /* Initialize consumer free queue */
    queue_size = consumer->cfg.queue_size;
    if (pipeline->cfg.cdc_enable) {
        queue_size += CDC_QUEUE_SIZE_INFLATION;
    }

    /*
     * Since the Mixer Output Pipeline producer and the Mixer Input Pipeline consumer share the
     * same queue, the Mixer Output Pipeline does not have a dedicated producer queue for
     * audio processing. To account for that, their shared free queue has 3 extra nodes used
     * for audio processing.
     */
    if (pipeline->cfg.mixer_option.input_mixer_pipeline) {
        queue_size += 3;
    }

    init_audio_free_queue(consumer, "Audio Buffer Free Queue",
                          queue_data_size, queue_size, err);
    if (*err != SAC_ERR_NONE) {
        return;
    }

    /* Initialize consumer queues */
    do {
        consumer->_free_queue = pipeline->consumer->_free_queue; /* All consumers use the same memory space */
        queue_size = consumer->_free_queue->limit;
        if (consumer->cfg.delayed_action) {
            /* When using delayed action, one of the node will not be available */
            queue_size--;
        }
        queue_init_queue(consumer->_queue, queue_size, "Audio Buffer");
        consumer = consumer->next_endpoint;
    } while (consumer != NULL);
}

/** @brief Initialize an audio free queue.
 *
 *  @param[in]  endpoint         Pointer to the queue's endpoint.
 *  @param[in]  queue_name       Name of the queue.
 *  @param[in]  queue_data_size  Size in bytes of the data in a node.
 *  @param[in]  queue_size       Number of nodes in the queue.
 *  @param[out] err              Error code.
 */
static void init_audio_free_queue(sac_endpoint_t *endpoint, const char *queue_name,
                                  uint16_t queue_data_size, uint8_t queue_size, sac_error_t *err)
{
    uint8_t *pool_ptr;

    *err = SAC_ERR_NONE;

    pool_ptr = mem_pool_malloc(&mem_pool, QUEUE_NB_BYTES_NEEDED(queue_size, queue_data_size));
    if (pool_ptr == NULL) {
        *err = SAC_ERR_NOT_ENOUGH_MEMORY;
        return;
    }
    endpoint->_free_queue = mem_pool_malloc(&mem_pool, sizeof(queue_t));
    if (endpoint->_free_queue == NULL) {
        *err = SAC_ERR_NOT_ENOUGH_MEMORY;
        return;
    }

    queue_init_pool(pool_ptr,
                    endpoint->_free_queue,
                    queue_size,
                    queue_data_size,
                    queue_name);
}

/** @brief Copy data from a node of the producer queue to a node of the consumer queue.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @param[in] node1     Node from the producer queue.
 */
static void move_audio_packet_to_consumer_queue(sac_pipeline_t *pipeline, queue_node_t *node1)
{
    queue_node_t *node2;
    sac_endpoint_t *consumer = pipeline->consumer;

    /* Detect overflow */
    do {
        if ((queue_get_length(consumer->_queue) == queue_get_limit(consumer->_queue)) &&
            (queue_get_length(consumer->_free_queue) == 0)) {
            pipeline->_statistics.consumer_buffer_overflow_count++;
            queue_free_node(queue_dequeue_node(consumer->_queue));
        }
        consumer = consumer->next_endpoint;
    } while (consumer != NULL);

    /* Move audio packet into a consumer node */
    node2 = queue_get_free_node(pipeline->consumer->_free_queue);
    memcpy(node2->data, node1->data, SAC_PACKET_HEADER_OFFSET + sizeof(sac_header_t) +
           sac_node_get_payload_size(node1) + pipeline->_user_data_size);

    /* Enqueue node for all consumers */
    consumer = pipeline->consumer;
    while (1) {
        queue_enqueue_node(consumer->_queue, node2);
        consumer = consumer->next_endpoint;
        if (consumer != NULL) {
            queue_inc_copy_count(node2);
        } else {
            break;
        }
    }
}

/** @brief Apply all processing stages to a producer queue node.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @param[in] node1     Node from the producer queue.
 *  @return Pointer to a producer queue node containing the processed data.
 */
static queue_node_t *process_samples(sac_pipeline_t *pipeline, queue_node_t *node1)
{
    uint16_t rv;
    queue_node_t *node2, *node_tmp;
    sac_processing_t *process = pipeline->process;

    /* Get a process destination node */
    node2 = queue_get_free_node(pipeline->producer->_free_queue);
    do {
        /* Execute gate function if present */
        if ((process->iface.gate == NULL) || process->iface.gate(process->instance, sac_node_get_header(node1),
                                                                 sac_node_get_data(node1),
                                                                 sac_node_get_payload_size(node1))) {
        /* node1 is the source node */
            rv = process->iface.process(process->instance,
                                        sac_node_get_header(node1),
                                        sac_node_get_data(node1),
                                        sac_node_get_payload_size(node1),
                                        sac_node_get_data(node2));
            if (rv != 0) { /* != 0 means processing happened */
                /* Copy the header from the old source */
                memcpy(sac_node_get_header(node2), sac_node_get_header(node1), sizeof(sac_header_t));
                /* Update the size */
                sac_node_set_payload_size(node2, rv);
                /* Swap node1 and node2 */
                node_tmp = node1;
                node1 = node2;
                node2 = node_tmp;
            }
        }
        process = process->next_process;
    } while (process != NULL);
    queue_free_node(node2);

    return node1;
}

/** @brief Enqueue the current producer queue node.
 *
 *  @param[in]  producer  Pointer to the producer endpoint.
 *  @param[out] err       Error code.
 */
static void enqueue_producer_node(sac_endpoint_t *producer, sac_error_t *err)
{
    *err = SAC_ERR_NONE;

    if (producer->cfg.use_encapsulation) {
        /* If produced audio is encapsulated, save the payload size locally */
        sac_node_set_payload_size(producer->_current_node, sac_node_get_header(producer->_current_node)->payload_size);
    }

    /*
     * Before enqueuing a node, make sure that the producer queue is empty.
     * If it's not, the processing hasn't started, so dequeue the node in queue
     * before enqueuing the new one.
     */
    if (queue_get_length(producer->_queue) > PROD_QUEUE_SIZE_MIN_WHEN_ENQUEUING) {
        queue_free_node(queue_dequeue_node(producer->_queue));
    }
    if (!queue_enqueue_node(producer->_queue, producer->_current_node)) {
        queue_free_node(producer->_current_node);
        *err = SAC_ERR_PRODUCER_Q_FULL;
    }
    /* The current node is no longer been used by the producer */
    producer->_current_node = NULL;
}

/** @brief Get a free producer queue node and apply the producer endpoint action.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] err       Error code.
 *  @return The amount of bytes produces.
 */
static uint16_t produce(sac_pipeline_t *pipeline, sac_error_t *err)
{
    sac_endpoint_t *producer = pipeline->producer;
    uint8_t *payload;
    uint16_t payload_size;

    *err = SAC_ERR_NONE;

    producer->_current_node = queue_get_free_node(producer->_free_queue);
    if (producer->_current_node == NULL) {
        *err = SAC_ERR_PRODUCER_Q_FULL;
        return 0;
    }

    payload_size = producer->cfg.audio_payload_size;
    if (producer->cfg.use_encapsulation) {
        payload = (uint8_t *)sac_node_get_header(producer->_current_node);
        payload_size += (sizeof(sac_header_t) + pipeline->_user_data_size);
    } else {
        payload = (uint8_t *)sac_node_get_data(producer->_current_node);
        sac_node_set_payload_size(producer->_current_node, payload_size);
    }

    return producer->iface.action(producer->instance, payload, payload_size);
}

/** @brief Apply the consumer endpoint action on the current node.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  consumer  Pointer to the consumer endpoint.
 *  @param[out] err       Error code.
 *  @return The amount of bytes consumed.
 */
static uint16_t consume(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_error_t *err)
{
    uint8_t *payload;
    uint16_t payload_size, crc;

    *err = SAC_ERR_NONE;

    if (consumer->_current_node == NULL) {
        pipeline->_statistics.consumer_buffer_underflow_count++;
        consumer->_buffering_complete = false;
        *err = SAC_ERR_CONSUMER_Q_EMPTY;
        return 0;
    } else {
        payload_size = sac_node_get_payload_size(consumer->_current_node);
        if (consumer->cfg.use_encapsulation) {
            payload = (uint8_t *)sac_node_get_header(consumer->_current_node);
            /* Update audio header's payload size before sending the packet */
            sac_node_get_header(consumer->_current_node)->payload_size = (uint8_t)payload_size;
            payload_size += (sizeof(sac_header_t) + pipeline->_user_data_size);
            ((sac_header_t *)payload)->tx_queue_level_high = (queue_get_length(consumer->_queue) < TX_QUEUE_HIGH_LEVEL) ? 0 : 1;

            /* Update CRC */
            ((sac_header_t *)payload)->crc4 = 0;
            ((sac_header_t *)payload)->reserved = 0;
            crc = crc4itu(0, payload, sizeof(sac_header_t));
            ((sac_header_t *)payload)->crc4 = crc;
        } else {
            payload = sac_node_get_data(consumer->_current_node);
        }
    }

    return consumer->iface.action(consumer->instance, payload, payload_size);
}

/** @brief Execute the specified not delayed action consumer endpoint.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  consumer  Consumer instance.
 *  @param[out] err       Error code.
 */
static void consume_no_delay(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_error_t *err)
{
    uint16_t size;

    *err = SAC_ERR_NONE;

    if (consumer->_buffering_complete) {
        /* Get the next node, if available, without dequeuing */
        consumer->_current_node = queue_get_node(consumer->_queue);
        /* Start consumption of the node */
        size = consume(pipeline, consumer, err);
        if (size > 0) {
            /* Consumed successfully, so dequeue and free */
            queue_free_node(queue_dequeue_node(consumer->_queue));
        }
        consumer->_current_node = NULL;
    }
}

/** @brief Execute the specified delayed action consumer endpoint.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  consumer  Consumer instance.
 *  @param[out] err       Error code.
 */
static void consume_delay(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_error_t *err)
{
    *err = SAC_ERR_NONE;

    if (consumer->_buffering_complete) {
        /* Free previous node */
        queue_free_node(consumer->_current_node);
        /* Get new node */
        consumer->_current_node = queue_dequeue_node(consumer->_queue);
        /* Start consumption of new node */
        consume(pipeline, consumer, err);
    }
}

/** @brief Validate the user configuration for the pipeline.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] err       Error code.
 */
static void validate_pipeline_config(sac_pipeline_t *pipeline, sac_error_t *err)
{
    if (pipeline->cfg.cdc_enable && pipeline->consumer->cfg.use_encapsulation) {
        *err = SAC_ERR_PIPELINE_CFG_INVALID;
    }
}

/** @brief Mix the producers' audio packet.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @return Pointer to a queue node containing the mixed samples.
 */
static queue_node_t *start_mixing_process(sac_pipeline_t *pipeline, queue_node_t *node)
{
    queue_node_t *temp_node;
    sac_endpoint_t *producer = pipeline->producer;
    uint8_t producer_index = 0;

    /* Loop on all the Output Producer Endpoints and load the Input Samples Queues */
    do {
        /* Loop until there are enough samples to create an audio payload */
        do {
            /* Verify if the producer has a packet */
            if (queue_get_length(producer->_queue) > 0) {
                /* If the queue has enough samples to make a payload no need to enqueue and append */
                if (audio_mixer_module->input_samples_queue[producer_index].current_size < audio_mixer_module->cfg.payload_size) {

                    /* Dequeue the packet and append it to the Input Samples Queue */
                    temp_node = queue_dequeue_node(producer->_queue);
                    audio_mixer_module_append_samples(&audio_mixer_module->input_samples_queue[producer_index],
                                                      sac_node_get_data(temp_node),
                                                      sac_node_get_payload_size(temp_node));
                    queue_free_node(temp_node);
                }
            } else {
                /* If no packet is present, append silence samples to the Input Samples Queue */
                audio_mixer_module_append_silence(&audio_mixer_module->input_samples_queue[producer_index],
                                                  audio_mixer_module->cfg.payload_size);
            }
        } while (audio_mixer_module->input_samples_queue[producer_index].current_size < audio_mixer_module->cfg.payload_size);

        producer_index++;
        producer = producer->next_endpoint;
    } while (producer != NULL);

    /* Once the Input Samples Queues are filled, mix them into the Output Packet Queue */
    audio_mixer_module_mix_packets(audio_mixer_module);

    /* Apply the Output Packet to the node and return it to the processing stage */
    memcpy(sac_node_get_data(node), audio_mixer_module->output_packet_buffer, audio_mixer_module->cfg.payload_size);
    sac_node_set_payload_size(node, audio_mixer_module->cfg.payload_size);

    return node;
}
