/** @file fixed_point.h
 *  @brief SR1000 Fixed point library for basic operation (+, -, *, /) on QX.Y
 *         number format. User have control over the integer value bits (2^X) and the
 *         precision bits (2^-Y).
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SR1000_FP_H_
#define SR1000_FP_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
typedef int32_t q_num_t;

/** @brief Fixed point parameters structure.
 */
typedef struct {
    uint8_t precision;    /**< Number of precision bits, between 1 and 31 */
    uint8_t integer_bits; /**< Number of bits for the integer */
} fp_format_t;

/** @brief Fixed point mean parameters structure.
 */
typedef struct {
    uint16_t max_mean_size;          /**< Maximum mean size */
    uint16_t mean_index;             /**< Current mean value index */
    int64_t  mean_accumulated_value; /**< Current mean accumulated value */
    uint8_t  mean_precision_bits;    /**< Fixed point precision for division calculation in mean */
} fp_mean_struct_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the Fixed point library.
 *
 *  @param[in] precision_bits  Number of bits for the precision part of the Q format(2^-X).
 *  @param[in] integer_bits    Number of bits for the integer value part of the Q format(2^Y).
 *  @return Fixed point parameters instance.
 */
fp_format_t sr1000_fp_initialization(uint8_t precision_bits, uint8_t integer_bits);

/** @brief Convert float number to Q representation.
 *
 *  This function convert a number represented using the
 *  floating point representation to the QX.Y format, where
 *  X is the integer part and Y is the precision. Theses are setup
 *  with sr1000_fp_initialization().
 *
 *  @param[in] fp_param_struct  Fixed point parameters instance.
 *  @param[in] reel_number      Number to be converted, in float.
 *  @return Number in QX.Y format.
 */
q_num_t sr1000_fp_float_to_q_conv(fp_format_t *fp_param_struct, float reel_number);

/** @brief Convert Q representation number to float number.
 *
 *  @param[in] fp_param_struct  Fixed point parameters instance.
 *  @param[in] q_number         Number to be converted, in QX.Y format.
 *  @return Converted number, in floating point representation.
 */
float sr1000_fp_q_to_float_conv(fp_format_t *fp_param_struct, q_num_t q_number);

/** @brief Convert Q representation number to 32 bits number.
 *
 *  @param[in] fp_param_struct  Fixed point parameters instance.
 *  @param[in] q_number         Number to be converted, in QX.Y format.
 *  @return Converted integer number.
 */
int32_t sr1000_fp_q_to_int_conv(fp_format_t *fp_param_struct, q_num_t q_number);

/** @brief Convert 32 bit number to Q representation number.
 *
 *  @param[in] fp_param_struct  Fixed point parameters instance.
 *  @param[in] reel_number      Reel integer number.
 *  @return Converted QX.Y format number.
 */
q_num_t sr1000_fp_int_to_q_conv(fp_format_t *fp_param_struct, int32_t reel_number);

/** @brief Add two Q represented number together.
 *
 *  @note Result is 32 bits saturated.
 *
 *  @param[in] q_num1  First QX.Y format number to be added.
 *  @param[in] q_num2  Second QX.Y format number to be added.
 *  @return Sum, in QX.Y format.
 */
q_num_t sr1000_fp_add(q_num_t q_num1, q_num_t q_num2);

/** @brief Subtract two Q represented number.
 *
 *  @note Result is 32 bits saturated.
 *
 *  @param[in] q_num1  Minuend, in QX.Y format.
 *  @param[in] q_num2  Subtrahend, in QX.Y format.
 *  @return Difference, in QX.Y format.
 */
q_num_t sr1000_fp_sub(q_num_t q_num1, q_num_t q_num2);

/** @brief Multiply two Q represented number.
 *
 *  @note Result is 32 bits saturated.
 *
 *  @param[in] fp_param_struct  Fixed point parameters instance.
 *  @param[in] q_num1           First factor, in QX.Y format.
 *  @param[in] q_num2           Second factor, in QX.Y format.
 *  @return Product, in QX.Y format.
 */
q_num_t sr1000_fp_multiply(fp_format_t *fp_param_struct, q_num_t q_num1, q_num_t q_num2);

/** @brief Divided two Q represented number.
 *
 *  @note Result is 32 bits saturated.
 *
 *  @param[in] fp_param_struct  Fixed point parameters instance.
 *  @param[in] q_num1           Dividend, in QX.Y format.
 *  @param[in] q_num2           Divisor, in QX.Y format.
 *  @return Quotient, in QX.Y format.
 */
q_num_t sr1000_fp_division(fp_format_t *fp_param_struct, q_num_t q_num1, q_num_t q_num2);

/** @brief Get the precision bits value.
 *
 *  Get the precision bits value in QX.Y format
 *  of the current initialize fixed point library.
 *
 *  @param[in] fp_param_struct  Fixed point parameters instance.
 *  @return Precision, in QX.Y format.
 */
q_num_t sr1000_fp_get_precision_q(fp_format_t *fp_param_struct);

/** @brief Initialize the Fixed point arithmetic mean .
 *
 *  @param[in] fp_param_struct  Fixed point parameters instance.
 *  @param[in] max_mean_size    Maximum mean size for the application.
 *  @return Fixed point mean instance.
 */
fp_mean_struct_t sr1000_fp_mean_init(fp_format_t *fp_param_struct, uint16_t mean_size);

/** @brief Add one element to the already initialized mean.
 *
 *  @param[in] fp_mean_struct  Fixed point mean instance.
 *  @param[in] reel_number     Number to be added, in QX.Y format.
 *  @return Accumulated mean value.
 */
int64_t sr1000_fp_mean_add(fp_mean_struct_t *fp_mean_struct, q_num_t reel_number);

/** @brief Reset the mean for another calculation.
 *
 *  @note This should be call after every sr1000_fp_mean_calculate()
 *        to reset the accumulated mean value and the current index.
 *
 *  @param[in] fp_mean_struct  Fixed point mean instance.
 */
void sr1000_fp_mean_reset(fp_mean_struct_t *fp_mean_struct);

/** @brief Calculate the mean based on previously added values.
 *
 *  @param[in] fp_mean_struct  Fixed point mean instance.
 *  @param[in] size            Size of the mean, 0 for the size define in sr1000_fp_mean_init().
 *  @return Mean result, in QX.Y format.
 */
q_num_t sr1000_fp_mean_calculate(fp_mean_struct_t *fp_mean_struct, uint16_t size);

#ifdef __cplusplus
}
#endif


#endif /* SR1000_FP_H_ */
