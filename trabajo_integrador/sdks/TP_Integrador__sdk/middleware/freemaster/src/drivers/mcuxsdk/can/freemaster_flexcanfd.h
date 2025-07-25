/*
 * Copyright 2025 NXP
 *
 * License: NXP LA_OPT_Online Code Hosting NXP_Software_License
 *
 * NXP Proprietary. This software is owned or controlled by NXP and may
 * only be used strictly in accordance with the applicable license terms.
 * By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that
 * you have read, and that you agree to comply with and are bound by,
 * such license terms.  If you do not agree to be bound by the applicable
 * license terms, then you may not retain, install, activate or otherwise
 * use the software.
 *
 * FreeMASTER Communication Driver - FlexCAN low-level driver for for CAN-FD
 */

#ifndef __FREEMASTER_FLEXCANFD_H
#define __FREEMASTER_FLEXCANFD_H

/******************************************************************************
 * Required header files include check
 ******************************************************************************/
#ifndef __FREEMASTER_H
#error Please include the freemaster.h master header file before the freemaster_flexcanfd.h
#endif

/* This low-level driver uses the MCUXpresso SDK peripheral structure types. */
#include "fsl_device_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Types definition
 ******************************************************************************/

/******************************************************************************
 * inline functions
 ******************************************************************************/

/******************************************************************************
 * Global API functions
 ******************************************************************************/

void FMSTR_CanSetBaseAddress(CAN_Type *base);
void FMSTR_CanIsr(void);

#ifdef __cplusplus
}
#endif

#endif /* __FREEMASTER_FLEXCANFD_H */
