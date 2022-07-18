/** @file  wps_utils.h
 *  @brief wps_utils brief
 *
 *  wps_utils description
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_UTILS_H_
#define WPS_UTILS_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Find the Greatest Common Divider(GCD) between 2 numbers.
 *
 *  @note For example, with 60 / 100, the GCD is 20. The reduced
 *        fraction is then 3/5.
 *
 *  @param[in] number1  First number.
 *  @param[in] number2  Second number.
 *  @return Greatest common divider.
 */
int wps_utils_gcd(int number1, int number2);


#ifdef __cplusplus
}
#endif

#endif /* WPS_UTILS_H_ */
