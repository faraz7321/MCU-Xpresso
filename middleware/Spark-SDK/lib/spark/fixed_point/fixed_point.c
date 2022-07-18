/** @file fixed_point.c
 *  @brief SR1000 Fixed point library for basic operation (+, -, *, /) on QX.Y
 *         number format. User have control over the precision bits (Y) and the
 *         integer value bits(X).
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "fixed_point.h"

/* CONSTANTS ******************************************************************/
#define FP_TOTAL_NUMBER_OF_BITS 32
#define FP_DEFAULT_PRECISION    16
#define FP_DEFAULT_INTEGER_BITS 15
#define FP_MAX_32_BITS_VALUE    (int64_t)((1U << (32U - 1U)) - 1U)
#define FP_MIN_32_BITS_VALUE    -1 - FP_MAX_32_BITS_VALUE

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool no_bits_defined(uint8_t bits_value1, uint8_t bits_value2);
static bool not_enough_fp_bits(uint8_t total_nb_bits);
static void apply_default_configuration(fp_format_t *fp_param);
static float saturate_value_float(int32_t interger_bits, float value);
static int64_t saturate_value32(int64_t value);
static int32_t clip64_to_32(int64_t x);

/* PUBLIC FUNCTIONS ***********************************************************/
fp_format_t sr1000_fp_initialization(uint8_t precision_bits, uint8_t interger_bits)
{
    fp_format_t fp_param_struct;

    if ((no_bits_defined(precision_bits, interger_bits)) || not_enough_fp_bits(precision_bits + interger_bits + 1)) {
        apply_default_configuration(&fp_param_struct);
    } else if (interger_bits == 0) {
        fp_param_struct.precision    = precision_bits;
        fp_param_struct.integer_bits = FP_TOTAL_NUMBER_OF_BITS - precision_bits - 1;
    } else if (precision_bits == 0) {
        fp_param_struct.precision    = FP_TOTAL_NUMBER_OF_BITS - interger_bits - 1;
        fp_param_struct.integer_bits = interger_bits;
    } else {
        fp_param_struct.precision    = precision_bits;
        fp_param_struct.integer_bits = interger_bits;
    }

    return fp_param_struct;
}

q_num_t sr1000_fp_float_to_q_conv(fp_format_t *fp_param_struct, float real_number)
{
    q_num_t conv_result = 0;

    real_number = saturate_value_float(fp_param_struct->integer_bits, real_number);

    conv_result = real_number * (1 << fp_param_struct->precision);

    return conv_result;
}

q_num_t sr1000_fp_int_to_q_conv(fp_format_t *fp_param_struct, int32_t real_number)
{
    q_num_t conv_result = 0;

    conv_result = real_number << fp_param_struct->precision;

    return conv_result;
}

float sr1000_fp_q_to_float_conv(fp_format_t *fp_param_struct, q_num_t q_number)
{
    float conv_result = 0;

    conv_result = (float)q_number / (1 << fp_param_struct->precision);

    return conv_result;
}

int32_t sr1000_fp_q_to_int_conv(fp_format_t *fp_param_struct, q_num_t q_number)
{
    int32_t conv_result = 0;

    conv_result = q_number >> fp_param_struct->precision;

    return conv_result;
}

q_num_t sr1000_fp_add(q_num_t q_num1, q_num_t q_num2)
{
    q_num_t result;
    int64_t result_tmp;

    result_tmp = clip64_to_32((int64_t)q_num1 + (int64_t)q_num2);

    result = (q_num_t)result_tmp;

    return result;
}

q_num_t sr1000_fp_sub(q_num_t q_num1, q_num_t q_num2)
{
    return sr1000_fp_add(q_num1, -q_num2);
}

q_num_t sr1000_fp_multiply(fp_format_t *fp_param_struct, q_num_t q_num1, q_num_t q_num2)
{
    q_num_t result;
    int64_t result_tmp;

    result_tmp = (int64_t)q_num1 * (int64_t)q_num2;

    result_tmp = result_tmp >> fp_param_struct->precision;

    result_tmp = saturate_value32(result_tmp);

    result = (q_num_t)result_tmp;

    return result;
}

q_num_t sr1000_fp_division(fp_format_t *fp_param_struct, q_num_t q_num1, q_num_t q_num2)
{
    int32_t result;
    int64_t nominator_scale;
    int64_t result_tmp;

    nominator_scale = (int64_t)q_num1 << fp_param_struct->precision;

    result_tmp = nominator_scale / (int64_t)q_num2;

    result_tmp = saturate_value32(result_tmp);

    result = (int32_t)result_tmp;

    return result;
}

fp_mean_struct_t sr1000_fp_mean_init(fp_format_t *fp_param_struct, uint16_t mean_size)
{
    fp_mean_struct_t fp_mean_struct;

    fp_mean_struct.max_mean_size          = mean_size;
    fp_mean_struct.mean_accumulated_value = 0;
    fp_mean_struct.mean_index             = 0;
    fp_mean_struct.mean_precision_bits    = fp_param_struct->precision;

    return fp_mean_struct;
}

int64_t sr1000_fp_mean_add(fp_mean_struct_t *fp_mean_struct, q_num_t real_number)
{
    fp_mean_struct->mean_index++;

    if (fp_mean_struct->mean_index <= fp_mean_struct->max_mean_size) {
        fp_mean_struct->mean_accumulated_value += real_number;
    }
    return fp_mean_struct->mean_accumulated_value;
}

void sr1000_fp_mean_reset(fp_mean_struct_t *fp_mean_struct)
{
    fp_mean_struct->mean_accumulated_value = 0;
    fp_mean_struct->mean_index             = 0;
}

q_num_t sr1000_fp_mean_calculate(fp_mean_struct_t *fp_mean_struct, uint16_t size)
{
    q_num_t result;
    int64_t nominator_scale;
    int64_t result_tmp;
    q_num_t mean_size_scale = 0;

    if (size != 0) {
        mean_size_scale = (q_num_t)size << fp_mean_struct->mean_precision_bits;
    } else {
        mean_size_scale = (q_num_t)fp_mean_struct->max_mean_size << fp_mean_struct->mean_precision_bits;
    }

    nominator_scale = fp_mean_struct->mean_accumulated_value << fp_mean_struct->mean_precision_bits;

    result_tmp = nominator_scale / (int64_t)mean_size_scale;

    result_tmp = saturate_value32(result_tmp);

    result = (q_num_t)result_tmp;

    return result;
}

q_num_t sr1000_fp_get_precision_q(fp_format_t *fp_param_struct)
{
    return 1 << fp_param_struct->precision;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Check if one or more bits is defined in 2 given value .
 *
 *  @param[in] bits_value1  First value to check.
 *  @param[in] bits_value2  Second value to check.
 *  @retval True   Both value have no bits set
 *  @retval False  One or both value have one or more bits set
 */
static bool no_bits_defined(uint8_t bits_value1, uint8_t bits_value2)
{
    if (bits_value1 == 0 && bits_value2 == 0) {
        return 1;
    }
    return 0;
}

/** @brief Check if the user set parameter are valid.
 *
 *  This will check if the sum of the precision_bits, the
 *  interger_bits and the signed representation is greater
 *  than 32.
 *
 *  @param[in] total_nb_bits  The sum, in bits, of the 3 parameters.
 *  @retval True   Sum is higher than 32.
 *  @retval False  Sum is lower of equal to 32.
 */
static bool not_enough_fp_bits(uint8_t total_nb_bits)
{
    if (total_nb_bits > FP_TOTAL_NUMBER_OF_BITS) {
        return 1;
    }
    return 0;
}

/** @brief Apply the default configuration.
 *
 *  @note This will apply the following parameters :
 *        - Precision bits : 16 -> 0.000015259.
 *        - Integer value  : 15 -> 32768.
 *
 *  @param[in] fp_param  Fixed point initialization instance.
 */
static void apply_default_configuration(fp_format_t *fp_param)
{
    fp_param->integer_bits = FP_DEFAULT_INTEGER_BITS;
    fp_param->precision    = FP_DEFAULT_PRECISION;
}

/** @brief Saturate given float value.
 *
 *  @note Value are saturate using 32-bit max signed value.
 *
 *  @param[in] interger_bits  Number of bits for max value.
 *  @param[in] value          Value to clip, in float.
 *  @return Saturated value.
 */
static float saturate_value_float(int32_t interger_bits, float value)
{
    const int32_t max = (int32_t)(((1U << interger_bits) - 1U));
    const int32_t min = -1 - max;

    if (value > max) {
        return max;

    } else if (value < min) {
        return min;
    }

    return value;
}

/** @brief Saturate given value.
 *
 *  @note Value are saturate using 32-bit max signed value.
 *
 *  @param[in] value  64-bit value to clip.
 *  @return Saturated value.
 */
static int64_t saturate_value32(int64_t value)
{
    if (value > FP_MAX_32_BITS_VALUE) {
        return FP_MAX_32_BITS_VALUE;
    } else if (value < FP_MIN_32_BITS_VALUE) {
        return FP_MIN_32_BITS_VALUE;
    }
    return value;
}

/** @brief Clip 64-bit value to 32-bit max value (Optimized).
 *
 *  @param[in] value  64-bit value to clip.
 *  @return Clipped value.
 */
static int32_t clip64_to_32(int64_t x)
{
    if ((int32_t) (x >> 32) != ((int32_t) x >> 31)) {
        return (0x7FFFFFFF ^ ((int32_t) (x >> 63)));
    } else {
        return (int32_t) x;
    }
}
