/** @file  evk_dbg.h
 *
 *  @brief This module controls debug io of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_DBG_H_
#define EVK_DBG_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Toggle io.
 *
 *  @param[in] io  io id number
 */
void evk_dbg_on(evk_dbg_t io);

/** @brief Toggle io.
 *
 *  @param[in] io  io id number
 */
void evk_dbg_off(evk_dbg_t io);

/** @brief Toggle io.
 *
 *  @param[in] io  io id number
 */
void evk_dbg_toggle(evk_dbg_t io);

/** @brief Pulse io.
 *
 *  @param[in] io  io id number
 */
void evk_dbg_pulse(evk_dbg_t io);


#ifdef __cplusplus
}
#endif

#endif /* EVK_DBG_H_ */

