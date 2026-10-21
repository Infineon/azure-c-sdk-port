/*
 * (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
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
