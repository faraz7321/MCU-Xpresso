/** @file  sac_api.h
 *  @brief SPARK Audio Core Application Programming Interface.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_API_H_
#define SAC_API_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "audio_mixer_module.h"
#include "crc4_itu.h"
#include "mem_pool.h"
#include "queue.h"
#include "resampling.h"
#include "sac_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define SAC_MAX_CHANNEL_COUNT          2 /*!< Maximum of audio channels supported in audio core */
#define SAC_NO_ARG                     0 /*!< Definition to use when no arguments are used if audio core calls */
#define SAC_NODE_PAYLOAD_SIZE_OFFSET   0 /*!< Position of the audio payload in the audio packet */
#define SAC_NODE_PAYLOAD_SIZE_VAR_SIZE sizeof(uint16_t) /*!< Size of the audio payload variable. */
#define SAC_PACKET_HEADER_OFFSET       (SAC_NODE_PAYLOAD_SIZE_OFFSET + SAC_NODE_PAYLOAD_SIZE_VAR_SIZE) /*!< Position of the audio header in the audio packet */
#define SAC_PACKET_DATA_OFFSET         (SAC_PACKET_HEADER_OFFSET + sizeof(sac_header_t)) /*!< Position of the packet data in the audio packet */
#define SAC_PRODUCER_QUEUE_SIZE        3 /*!< Queue size to use when initializing a producer audio endpoint */
#define SAC_TXQ_ARR_LEN                3 /*!< Array size holding tx queue len values to determine a rolling average */

/* MACROS *********************************************************************/
#define sac_node_get_payload_size(node) (*((uint16_t *)(queue_get_data_ptr(node, SAC_NODE_PAYLOAD_SIZE_OFFSET)))) /*!< Get the audio payload size in the audio packet */
#define sac_node_set_payload_size(node, payload_size) ((*((uint16_t *)(queue_get_data_ptr(node, SAC_NODE_PAYLOAD_SIZE_OFFSET)))) = (payload_size)) /*!< Set the audio payload size in the audio packet */
#define sac_node_get_header(node) ((sac_header_t *)(queue_get_data_ptr(node, SAC_PACKET_HEADER_OFFSET))) /*!< Get a pointer to the audio header in the audio packet */
#define sac_node_get_data(node) ((uint8_t *)(queue_get_data_ptr(node, SAC_PACKET_DATA_OFFSET))) /*!< Get a pointer to the packet data in the audio packet */
#define sac_align_data_size(current_size, type_to_align) (sizeof(type_to_align) - (current_size % sizeof(type_to_align))) /*!< Return an array size aligned on a specific type */

/* TYPES **********************************************************************/
/** @brief Audio Bit Depth.
 */
typedef enum sac_bit_depth {
    AUDIO_16BITS = 16, /*!< 16-bit PCM samples */
    AUDIO_20BITS = 20, /*!< 20-bit PCM samples */
    AUDIO_24BITS = 24  /*!< 24-bit PCM samples */
} sac_bit_depth_t;

/** @brief Audio Word Size.
 */
typedef enum sac_word_size {
    AUDIO_16BITS_BYTE = 2, /*!< 2-byte word for 16-bit PCM samples */
    AUDIO_20BITS_BYTE = 4, /*!< 4-byte word for 20-bit PCM samples */
    AUDIO_24BITS_BYTE = 4, /*!< 4-byte word for 24-bit PCM samples */
    AUDIO_32BITS_BYTE = 4  /*!< 4-byte word for 32-bit PCM samples */
} sac_word_size_t;

/** @brief CDC Instance.
 */
typedef struct sac_cdc_instance {
    resampling_instance_t resampling_instance; /*!< An instance of the resampling */
    uint8_t  size_of_buffer_type;              /*!< Number of bytes per audio sample */
    uint8_t  *avg_arr;                         /*!< An circular array of tx queue lengths used for averaging */
    uint32_t avg_sum;                          /*!< Rolling average of the avg_arr */
    uint32_t avg_val;                          /*!< Normalized average of avg_sum to increase resolution */
    uint32_t count;                            /*!< Used to ensure a minimum number of queue length samples before determining
                                                    a resampling action */
    uint16_t avg_idx;                          /*!< Index into the avg_arr */
    uint8_t  max_queue_offset;                 /*!< Trigger level to determine whether to take a resampling action */
    uint32_t normal_queue_size;                /*!< Normalized queue size to increase trigger resolution */
    uint16_t queue_avg_size;                   /*!< Size of the averaging array avg_arr */
    bool wait_for_queue_full;                  /*!< Set due to feedback from audio source that its tx queue is full. This will pause
                                                    any resampling activity until the audio source tx queue has emptied */
} sac_cdc_instance_t;

/** @brief Audio Header.
 */
typedef struct sac_header {
    uint8_t tx_queue_level_high:1; /*!< For clock drift compensation. Used by the
                                        recorder to notify the player that its TX audio buffer is filling up */
    uint8_t user_data_is_valid:1;  /*!< Indicates if the audio packet trailing byte is a valid user data byte */
    uint8_t fallback:1;            /*!< Indicates a fallback packet */
    uint8_t reserved:1;            /*!< Reserved for future use */
    uint8_t crc4:4;                /*!< CRC4 of the header */
    uint8_t payload_size;          /*!< Size of the payload (audio samples) expressed in bytes */
} sac_header_t;

/** @brief Processing Interface.
 */
typedef struct sac_processing_interface {
    void (*init)(void *instance, mem_pool_t *mem_pool); /*!< Function the audio core uses to execute any processing
                                                             stage initialization sequence */
    void (*deinit)(void *instance); /*!< Function the audio core uses to execute any processing
                                         stage de-initialization sequence */
    uint32_t (*ctrl)(void *instance, uint8_t cmd, uint32_t args); /*!< Function the audio application uses to interact with the
                                                                       processing stage */
    uint16_t (*process)(void *instance, sac_header_t *header,
                        uint8_t *data_in, uint16_t size, uint8_t *data_out); /*!< Function the audio core uses to do
                                                                                  processing on audio samples */
    bool (*gate)(void *instance,  sac_header_t *header,
                        uint8_t *data_in, uint16_t size); /*!< Function called by process_samples prior to process to
                                                               to determine if process will be executed or not */
} sac_processing_interface_t;

/** @brief Audio Processing.
 */
typedef struct sac_processing {
    void *instance;                      /*!< Pointer to the processing stage's specific instance */
    const char *name;                    /*!< Character string describing the processing stage */
    sac_processing_interface_t iface;    /*!< Interface the processing stage must comply to */
    struct sac_processing *next_process; /*!< Pointer to the next processing state */
} sac_processing_t;

/** @brief Endpoint Interface.
 */
typedef struct sac_endpoint_interface {
    uint16_t (*action)(void *instance, uint8_t *samples, uint16_t size); /*!< Function the audio core uses to send or receive audio
                                                                              samples depending if the endpoint produces or consumes */
    void (*start)(void *instance); /*!< Function the audio core uses to execute any endpoint startup sequence */
    void (*stop)(void *instance);  /*!< Function the audio core uses to stop any endpoint operations */
} sac_endpoint_interface_t;

/** @brief Add Audio Mixer's specific options when using pipelines to mix packets.
 */
typedef struct sac_mixer_option {
    bool input_mixer_pipeline;  /*!< True if it is the input pipeline of the mixing stage */
    bool output_mixer_pipeline; /*!< True if it is the output pipeline of the mixing stage */
} sac_mixer_option_t;

/** @brief Audio Endpoint Configuration.
 */
typedef struct audio_endpoint_cfg {
    bool use_encapsulation;      /*!< True if the endpoint produces or consumes audio packets (audio header + audio payload),
                                      False for only audio payloads (audio samples) */
    bool delayed_action;         /*!< True if the endpoint requires a complete cycle to produce or consume data.
                                      False if the endpoint produces or consumes instantly */
    uint8_t channel_count;       /*!< 1 if the endpoint produces or consumes mono audio payloads and 2 for interleaved stereo */
    sac_bit_depth_t bit_depth;   /*!< Bit depth of samples the endpoint produces or consumes */
    uint16_t audio_payload_size; /*!< Size in bytes of the audio payload */
    uint8_t queue_size;          /*!< Size in number of audio packets the endpoint's queue can contain */
} audio_endpoint_cfg_t;

/** @brief Audio Endpoint.
 */
typedef struct sac_endpoint {
    void *instance;                     /*!< Pointer to endpoint's specific instance */
    const char *name;                   /*!< Character string describing the endpoint */
    sac_endpoint_interface_t iface;     /*!< Interface the endpoint must comply to */
    audio_endpoint_cfg_t cfg;           /*!< Audio endpoint configuration */
    struct sac_endpoint *next_endpoint; /*!< Pointer to the next endpoint */
    queue_t *_queue;                    /*!< Internal: queue the endpoint will use to store or retrieve audio packets */
    queue_t *_free_queue;               /*!< Internal: Pointer to the free queue the endpoint will retrieve free nodes from */
    queue_node_t *_current_node;        /*!< Internal: pointer to the queue node the endpoint is working with at the moment */
    bool _buffering_complete;           /*!< Internal: Whether or not the initial audio buffering has been completed */
} sac_endpoint_t;

/** @brief Audio Pipeline Configuration.
 */
typedef struct sac_pipeline_cfg {
    bool cdc_enable;                 /*!< Enable the clock drift compensation on the pipeline */
    uint16_t cdc_resampling_length;  /*!< Amount of samples used when resampling. Can be ignored if CDC is not enabled. */
    uint16_t cdc_queue_avg_size;     /*!< Amount of measurements used when averaging the consumer queue size.
                                          Can be ignored if CDC is not enabled. */
    bool do_initial_buffering;       /*!< Wait for the consumer queue (TX audio buffer) to be
                                          full before starting to consume */
    bool user_data_enable;           /*!< Set to true if using User Data processing, false otherwise */
    sac_mixer_option_t mixer_option; /*!< Configure the pipeline with mixer's specific options */
} sac_pipeline_cfg_t;

/** @brief Audio Statistics.
 */
typedef struct sac_statistics {
    uint32_t producer_buffer_load; /*!< Number audio packets currently in the producer queue */
    uint16_t producer_buffer_size; /*!< Maximum number audio packets the producer queue can hold */
    uint32_t consumer_buffer_load; /*!< Number audio packets currently in the consumer queue */
    uint16_t consumer_buffer_size; /*!< Maximum number audio packets the consumer queue can hold */
    uint32_t consumer_buffer_overflow_count;   /*!< Number of times the consumer queue has overflowed */
    uint32_t consumer_buffer_underflow_count;  /*!< Number of times the consumer queue has underflowed */
    uint32_t producer_packets_corrupted_count; /*!< Number of corrupted packets received from the coord */
} sac_statistics_t;

/** @brief Audio Pipeline.
 */
typedef struct sac_pipeline {
    const char *name;                  /*!< Name of the pipeline */
    sac_endpoint_t *producer;          /*!< Pointer to the audio endpoint that will produce audio samples
                                            to this audio pipeline */
    sac_processing_t *process;         /*!< List of processing stages that will sequentially be
                                            used on the produced samples before they are consumed */
    sac_endpoint_t *consumer;          /*!< Pointer to the audio endpoint that will consume audio samples
                                            from this audio pipeline */
    sac_pipeline_cfg_t cfg;            /*!< Audio pipeline configuration */
    sac_statistics_t _statistics;      /*!< Audio-specific statistics */
    uint8_t _buffering_threshold;      /*!< Internal: The number of audio packets to buffer before considering
                                            the initial buffering complete */
    uint8_t _user_data_size;           /*!< Internal: Set to 0 or 1 depending on cfg->enable_user_data */
    sac_cdc_instance_t *_cdc_instance; /*!< Internal: CDC instance for this pipeline */
} sac_pipeline_t;

/** @brief Audio Fallback Module Configuration.
 */
typedef struct sac_fallback_module_cfg {
    uint32_t link_margin_threshold;            /*!< Default lm threshold */
    uint32_t link_margin_threshold_hysteresis; /*!< Threshold hysteresis */
    sac_pipeline_t *pipeline;                  /*!< Fallback pipeline */
    uint32_t txq_max_size;                     /*!< Max TX queue size */
    uint32_t audio_packets_per_second;         /*!< Number of audio packets per second */
} sac_fallback_module_cfg_t;

/** @brief Fallback State.
 */
typedef enum sac_fallback_state {
    FB_STATE_NORMAL,             /*!< Normal state, monitor txq for switch to wait threshold */
    FB_STATE_WAIT_THRESHOLD,     /*!< Link degrading, measure link margin for ~1/4 sec to determine return from fallback threshold */
    FB_STATE_FALLBACK,           /*!< Fallback mode due to degraded link, monitor link margin for return to normal mode */
    FB_STATE_FALLBACK_DISCONNECT /*!< Fallback mode due to disconnected link, threshold set to fixed value,
                                      monitor link margin for return to normal mode */
} sac_fallback_state_t;

/** @brief  TX Queue Metrics.
 */
typedef struct sac_txq_metrics {
    uint8_t  txq_len_arr[SAC_TXQ_ARR_LEN]; /*!< TX queue length averaging array */
    uint8_t  txq_len_arr_idx;              /*!< TX queue size averaging array index */
    uint16_t txq_len_sum;                  /*!< TX queue len rolling average sum */
    uint8_t  txq_len_avg;                  /*!< Tx queue len rolling average */
} sac_txq_metrics_t;

/** @brief Link Margin Metrics.
 */
typedef struct sac_link_margin_metrics {
    int32_t  link_margin_accumulator;       /*!< Accumulation of link margin values over a nominal 1/4 sec */
    uint16_t link_margin_accumulator_count; /*!< Number of link margin values accumulated */
    int16_t  link_margin_acc_avg;           /*!< Link margin average calculated from accumulator */
    uint8_t  link_margin_count;             /*!< Number of link margin values above the threshold */
} sac_link_margin_metrics_t;

/** @brief Audio Fallback Module.
 */
typedef struct sac_fallback_module {
    uint16_t fallback_threshold;                   /*!< Current link margin threshold to return to normal */
    uint16_t fallback_threshold_default;           /*!< Default link margin threshold */
    uint16_t fallback_threshold_hysteresis;        /*!< Link margin threshold hysteresis */
    uint16_t fallback_packets_per_250ms;           /*!< Number of audio packets per 250 ms */
    sac_fallback_state_t fallback_state;           /*!< Fallback state */
    uint32_t fallback_count;                       /*!< Number of times fallback was triggered */
    sac_pipeline_t *pipeline;                      /*!< Pipeline where fallback will be applied */
    uint32_t txq_max_size;                         /*!< Max size of the tx queue*/
    bool fallback_flag;                            /*!< Fallback mode flag */
    bool rx_link_margin_data_valid;                /*!< Flag indicating that  rx_link_margin_data_rx data is valid*/
    int16_t rx_link_margin_data_rx;                /*!< Link margin data from the node */
    sac_txq_metrics_t txq_metrics;                 /*!< TX queue metrics */
    sac_link_margin_metrics_t link_margin_metrics; /*!< Link margin metrics */
} sac_fallback_module_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the Audio Core.
 *
 *  @param[in] queue_critical     Critical section functions.
 *  @param[in] queue_memory       Pointer to allocated queue memory.
 *  @param[in] queue_memory_size  Size of allocated queue memory.
 */
void sac_init(queue_critical_cfg_t queue_critical, uint8_t *queue_memory, uint32_t queue_memory_size);

/** @brief Initialize the Audio Mixer Module.
 *
 *  @note Only call this initialization when the Mixer Module is needed.
 *
 *  @param[in]  cfg  Configuration of the Audio Mixer Module.
 *  @param[out] err  Error code.
 */
void sac_mixer_module_init(audio_mixer_module_cfg_t cfg, sac_error_t *err);

/** @brief Initialize the Audio Fallback Module.
 *
 *  @note Only call this initialization when the Fallback Module is needed.
 *
 *  @param[in]  cfg  Configuration of the Audio Fallback Module.
 *  @param[out] err  Error code.
 */
void sac_fallback_module_init(sac_fallback_module_cfg_t cfg, sac_error_t *err);

/** @brief Initialize an audio pipeline.
 *
 *  @param[in]  name      Name of the pipeline.
 *  @param[in]  producer  Producer endpoint.
 *  @param[in]  cfg       Pipeline configuration.
 *  @param[in]  consumer  Consumer endpoint.
 *  @param[out] err       Error code.
 *  @return Reference to the initialized audio pipeline.
 */
sac_pipeline_t *sac_pipeline_init(const char *name, sac_endpoint_t *producer,
                                  sac_pipeline_cfg_t cfg, sac_endpoint_t *consumer, sac_error_t *err);

/** @brief Initialize an audio endpoint.
 *
 *  @param[in]  instance  Endpoint instance.
 *  @param[in]  name      Name of the endpoint.
 *  @param[in]  iface     Interface of the endpoint.
 *  @param[in]  cfg       Endpoint configuration.
 *  @param[out] err       Error code.
 *  @return Reference to the initialized endpoint.
 */
sac_endpoint_t *sac_endpoint_init(void *instance, const char *name, sac_endpoint_interface_t iface, audio_endpoint_cfg_t cfg, sac_error_t *err);

/** @brief Link the queue of a consumer endpoint with a producer endpoint.
 *
 *  @note This feature is used for audio mixing.
 *
 *  @param[in]  consumer  Consumer endpoint instance to be linked.
 *  @param[in]  producer  Producer endpoint instance to be linked.
 *  @param[out] err       Error code.
 */
void sac_endpoint_link(sac_endpoint_t *consumer, sac_endpoint_t *producer, sac_error_t *err);

/** @brief Initialize an audio processing stage.
 *
 *  @param[in]  instance  Processing stage instance.
 *  @param[in]  name      Name of the processing stage.
 *  @param[in]  iface     Interface of the processing stage.
 *  @param[out] err       Error code.
 *  @return Reference to the initialized processing stage.
 */
sac_processing_t *sac_processing_stage_init(void *instance, const char *name, sac_processing_interface_t iface, sac_error_t *err);

/** @brief Add a processing stage to the pipeline.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @param[in] process   Pointer to processing structure.
 */
void sac_pipeline_add_processing(sac_pipeline_t *pipeline, sac_processing_t *process);

/** @brief Add an extra consumer endpoint to the pipeline.
 *
 *  @param[in] pipeline       Pipeline instance.
 *  @param[in] next_consumer  Extra consumer endpoint.
 */
void sac_pipeline_add_extra_consumer(sac_pipeline_t *pipeline, sac_endpoint_t *next_consumer);

/** @brief Add an extra producer endpoint to the pipeline.
 *
 *  @note This feature is used for Audio Mixing.
 *
 *  @param[in] pipeline       Pipeline instance.
 *  @param[in] next_producer  Extra producer endpoint.
 */
void sac_pipeline_add_extra_producer(sac_pipeline_t *pipeline, sac_endpoint_t *next_producer);

/** @brief Setup the audio pipeline.
 *
 *  This makes the pipeline ready to use. It must be called last,
 *  after every other initialization functions.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] err       Error code.
 */
void sac_pipeline_setup(sac_pipeline_t *pipeline, sac_error_t *err);

/** @brief Start the audio pipeline.
 *
 *  @param[in] pipeline  Pipeline instance.
 */
void sac_pipeline_start(sac_pipeline_t *pipeline);

/** @brief Stop the audio pipeline.
 *
 *  @param[in] pipeline  Pipeline instance.
 */
void sac_pipeline_stop(sac_pipeline_t *pipeline);

/** @brief Execute process specific control.
 *
 *  @param[in] audio_processing  Audio processing structure.
 *  @param[in] cmd               Command specific to the processing stage.
 *  @param[in] arg               Argument specific to the processing stage.
 *  @return A value specific to the control function.
 */
uint32_t sac_processing_ctrl(sac_processing_t *audio_processing, uint8_t cmd, uint32_t arg);

/** @brief Execute the audio processing stages.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] err       Error code.
 */
void sac_pipeline_process(sac_pipeline_t *pipeline, sac_error_t *err);

/** @brief Execute the produce endpoint.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] err       Error code.
 */
void sac_pipeline_produce(sac_pipeline_t *pipeline, sac_error_t *err);

/** @brief Execute the consumer endpoint(s).
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] err       Error code.
 */
void sac_pipeline_consume(sac_pipeline_t *pipeline, sac_error_t *err);

/** @brief Execute all the consume endpoints that contain data in their queue.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] err       Error code.
 */
void sac_pipeline_consume_all(sac_pipeline_t *pipeline, sac_error_t *err);

/** @brief Get the audio stats.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @return Pointer to audio stats structure.
 */
sac_statistics_t *sac_pipeline_get_stats(sac_pipeline_t *pipeline);

/** @brief Get producer buffer load.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @return buffer producer load value
 */
uint32_t sac_pipeline_get_producer_buffer_load(sac_pipeline_t *pipeline);

/** @brief Get consumer buffer load.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @return Consumer buffer load value.
 */
uint32_t sac_pipeline_get_consumer_buffer_load(sac_pipeline_t *pipeline);

/** @brief Get consumer buffer overflow count.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @return Consumer buffer overflow counter.
 */
uint32_t sac_pipeline_get_consumer_buffer_overflow_count(sac_pipeline_t *pipeline);

/** @brief Get consumer buffer underflow count.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @return Consumer buffer underflow counter.
 */
uint32_t sac_pipeline_get_consumer_buffer_underflow_count(sac_pipeline_t *pipeline);

/** @brief Get producer packets corrupted.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @return Producer packets corrupted.
 */
uint32_t sac_pipeline_get_producer_packets_corrupted_count(sac_pipeline_t *pipeline);

/** @brief Reset the audio stats.
 *
 *  @param[in] pipeline  Pipeline instance.
 */
void sac_pipeline_reset_stats(sac_pipeline_t *pipeline);

/** @brief Get the number of bytes allocated in the memory pool.
 *
 *  @return Number of bytes allocated in the memory pool.
 */
uint32_t sac_get_allocated_bytes(void);


#ifdef __cplusplus
}
#endif

#endif /* SAC_API_H_ */
