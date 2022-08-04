/** @file  wps_utils.c
 *  @brief WPS utility function.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_utils.h"

/* PUBLIC FUNCTIONS ***********************************************************/
int wps_utils_gcd(int number1, int number2)
{
    int latest_remainder = number2; /* Remainder k - 1 */
    int predecesor       = number1; /* Remainder k - 2 */
    int temp_remainder;

    while (latest_remainder) {
        temp_remainder   = latest_remainder;
        latest_remainder = predecesor % temp_remainder;
        predecesor       = temp_remainder;
    }

    return predecesor;
}
