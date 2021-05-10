/*
 * Copyright 2021, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/**
 * @file cy_azure_platform_port.c
 * Implementation of timer functions using Cypress abstraction-rtos
 */

#include <az_platform.h>
#include <az_precondition_internal.h>

#include <_az_cfg.h>

#include "clock.h"
#include "cyabs_rtos.h"
#include "cy_result.h"
#include "cy_az_iot_sdk_port_log.h"

/* Gets the platform clock in milliseconds */
AZ_NODISCARD az_result az_platform_clock_msec( int64_t* out_clock_msec )
{
    _az_PRECONDITION_NOT_NULL( out_clock_msec );

    cy_time_t time_ms;
    cy_rslt_t result;

    result = cy_rtos_get_time( &time_ms );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_rtos_get_time failed with Error = [0x%X]\n", ( unsigned int )result );
        time_ms = 0;
        *out_clock_msec = ( int64_t )time_ms;
        return _az_RESULT_MAKE_ERROR( _az_FACILITY_CORE_PLATFORM, 0 );
    }
    *out_clock_msec = ( int64_t )time_ms;

  return AZ_OK;
}

/* Platform to sleep for a given number of milliseconds */
AZ_NODISCARD az_result az_platform_sleep_msec( int32_t milliseconds )
{
    cy_rslt_t result;

    result = cy_rtos_delay_milliseconds( (uint32_t)milliseconds );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_rtos_delay_milliseconds failed with Error = [0x%X]\n", ( unsigned int )result );
        return _az_RESULT_MAKE_ERROR( _az_FACILITY_CORE_PLATFORM, 0 );
    }

  return AZ_OK;
}
