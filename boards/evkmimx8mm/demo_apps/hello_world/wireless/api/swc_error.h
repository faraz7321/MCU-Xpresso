/** @file  swc_error.h
 *  @brief SPARK Wireless Core error codes.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_ERROR_H_
#define SWC_ERROR_H_

/* TYPES **********************************************************************/
/** @brief Wireless API error structure.
 */
typedef enum swc_error {
    SWC_ERR_NONE = 0,         /*!< No error occurred */
    SWC_ERR_NOT_ENOUGH_MEMORY /*!< Not enough memory is allocated by the application
                                   for a full wireless core initialization */
} swc_error_t;


#endif /* SWC_ERROR_H_ */
