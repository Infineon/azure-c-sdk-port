/*
 * Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
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
 * @file cy_azure_http_port.c
 * Implementation of port functions for Azure SDK on Cypress platforms
 */

#include <az_http_transport.h>
#include <_az_cfg.h>

#include "cy_http_client_api.h"
#include "cy_az_iot_sdk_port_log.h"
#include "cy_result.h"

/**
 * API to convert cy_rslt_t to az_result
 */
static AZ_NODISCARD az_result _az_http_client_error_code_to_result( cy_rslt_t result )
{
    switch ( result )
    {
      case CY_RSLT_SUCCESS:
        return AZ_OK;

      case CY_RSLT_HTTP_CLIENT_ERROR_BADARG:
        return AZ_ERROR_ARG;

      case CY_RSLT_HTTP_CLIENT_ERROR_NOMEM:
        return AZ_ERROR_NOT_ENOUGH_SPACE;

      case CY_RSLT_HTTP_CLIENT_ERROR_INVALID_RESPONSE:
        return AZ_ERROR_HTTP_RESPONSE_OVERFLOW;

      case CY_RSLT_HTTP_CLIENT_ERROR_INVALID_CHUNK_HEADER:
        return AZ_ERROR_HTTP_CORRUPT_RESPONSE_HEADER;

      default:
        /* All other error codes will return the generic Azure HTTP error */
        return AZ_ERROR_HTTP_ADAPTER;
    }
}

/**
 * API to convert az_http_client_method to cy_http_client_method
 */
static cy_rslt_t _convert_az_method_to_cy_method( az_http_method az_method, cy_http_client_method_t *out_method )
{
    cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "\n azure method : [%s] \n", az_method._internal.ptr );

    if( strcmp( ( char * )az_span_ptr( az_method ), ( char * )az_span_ptr( az_http_method_get() ) ) == 0 )
    {
        *out_method = CY_HTTP_CLIENT_METHOD_GET;
    }
    else if( strcmp( ( char * )az_span_ptr( az_method ), ( char * )az_span_ptr( az_http_method_put() ) ) == 0 )
    {
        *out_method = CY_HTTP_CLIENT_METHOD_PUT;
    }
    else if( strcmp( ( char * )az_span_ptr( az_method ), ( char * )az_span_ptr( az_http_method_post() ) ) == 0 )
    {
        *out_method = CY_HTTP_CLIENT_METHOD_POST;
    }
    else if( strcmp( ( char * )az_span_ptr( az_method ), ( char * )az_span_ptr( az_http_method_head() ) ) == 0 )
    {
        *out_method = CY_HTTP_CLIENT_METHOD_HEAD;
    }
    else if( strcmp( ( char * )az_span_ptr( az_method ), ( char * )az_span_ptr( az_http_method_delete() ) ) == 0 )
    {
        *out_method = CY_HTTP_CLIENT_METHOD_DELETE;
    }
    else if( strcmp( ( char * )az_span_ptr( az_method ), ( char * )az_span_ptr( az_http_method_patch() ) ) == 0 )
    {
        *out_method = CY_HTTP_CLIENT_METHOD_PATCH;
    }
    else
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "\nInvalid method passed : [%s] \n",az_method._internal.ptr );
        return CY_RSLT_HTTP_CLIENT_ERROR_BADARG;
    }

    cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, "\nRequested client method: [%d] \n", *out_method );
    return CY_RSLT_SUCCESS;
}

/**
 * API to convert az_http_client_header to cy_http_client_header
 */
static cy_rslt_t _convert_az_http_headers_to_cy_http_header( az_http_request const *request, cy_http_client_header_t *header_buf )
{
    az_result result;
    az_span   header_name = { 0 };
    az_span   header_value = { 0 };
    int32_t   count;

    cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, " _convert_az_http_headers_to_cy_http_header\n" );

    for ( count = 0; count < az_http_request_headers_count( request ); ++count )
    {
        /* Get each header value and name and update into cy_http_client_header_t */
        result = az_http_request_get_header( request, count, &header_name, &header_value );
        if( result != AZ_OK )
        {
            return CY_RSLT_HTTP_CLIENT_ERROR_PARSER;
        }
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, " header_name \n%.*s\n",az_span_size( header_name ), az_span_ptr( header_name ) );
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_DEBUG, " header_value \n%.*s\n",az_span_size( header_value ), az_span_ptr( header_value ) );

        header_buf->field = ( char * )az_span_ptr( header_name );
        header_buf->field_len = az_span_size( header_name );
        header_buf->value = ( char * )az_span_ptr( header_value );
        header_buf->value_len = az_span_size( header_value );
    }

    return CY_RSLT_SUCCESS;
}

AZ_NODISCARD az_result az_http_client_send_request( az_http_request const* request, az_http_response* ref_response )
{
    cy_http_client_t                 handle;
    cy_http_client_request_header_t  cy_http_request;
    cy_http_client_header_t         *header_obj = NULL;
    cy_http_client_method_t          out_method;
    uint32_t                         num_header;
    cy_http_client_response_t        cy_http_response;
    cy_rslt_t                        result = CY_RSLT_SUCCESS;

    if( request == NULL || ref_response == NULL || az_span_ptr( ref_response->_internal.http_response ) == NULL )
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\n Invalid Arguments \n" );
        return _az_http_client_error_code_to_result( CY_RSLT_HTTP_CLIENT_ERROR_BADARG );
    }

    handle = (cy_http_client_t)request->_internal.context;

    cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, " portlayer handle:[%p], request->_internal.context:[%p] \n",handle, request->_internal.context );

    /* Convert method and assign it to cy_http_client_request_header_t */
    result = _convert_az_method_to_cy_method( request->_internal.method, &out_method );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "_convert_az_method_to_cy_method = [0x%X]\n", ( unsigned int )result );
        return _az_http_client_error_code_to_result( CY_RSLT_HTTP_CLIENT_ERROR_BADARG );
    }
    cy_http_request.method = out_method;

    /* Update the resource path into cy_http_client_request_header_t */
    cy_http_request.resource_path = ( const char * )az_span_ptr( request->_internal.url );

    num_header = (uint32_t)az_http_request_headers_count( request );
    cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, " num_header:[%d]\n", num_header );
    if( num_header == 0 )
    {
        return _az_http_client_error_code_to_result( CY_RSLT_HTTP_CLIENT_ERROR_BADARG );
    }

    header_obj = ( cy_http_client_header_t * )malloc( sizeof( cy_http_client_header_t ) * num_header );
    if( header_obj == NULL )
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "\nMalloc for HTTP client handle failed..!\n" );
        return _az_http_client_error_code_to_result( CY_RSLT_HTTP_CLIENT_ERROR_NOMEM );
    }
    cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, " header_obj:[%p]\n", header_obj );

    /* Read the header from the request->_internal.headers and update to cy_http_client_header_t header */
    result = _convert_az_http_headers_to_cy_http_header( request, header_obj );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "_convert_az_http_headers_to_cy_http_header = [0x%X]\n", ( unsigned int )result );
        free( header_obj );
        return _az_http_client_error_code_to_result( result );
    }

    /* Azure SDK API doesn't allow you to explicitly set the range header; therefore, default values are set,
     * which will not add the range header into the header list. If a range header is required, the caller should update it along with other headers */
    cy_http_request.range_start = -1;
    cy_http_request.range_end = 0;

    /* Assign buffer to send and receive the HTTP client request and response. This buffer must be set by the application. */
    cy_http_request.buffer = az_span_ptr( ref_response->_internal.http_response );
    cy_http_request.buffer_len = az_span_size( ref_response->_internal.http_response );

    /* API to generate the header and assign it to the buffer in cy_http_client_request_header_t */
    result = cy_http_client_write_header( handle, &cy_http_request, header_obj, num_header );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_http_client_write_header = [0x%X]\n", ( unsigned int )result );
        free( header_obj );
        return _az_http_client_error_code_to_result( result );
    }
    free( header_obj );

    cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_INFO, "Sending Request Headers:\n%.*s\n", ( int ) cy_http_request.headers_len, ( char * ) cy_http_request.buffer);

    /* API to send an HTTP client request and receive the response */
    result = cy_http_client_send( handle, &cy_http_request, ( uint8_t * )az_span_ptr(request->_internal.body), az_span_size( request->_internal.body ), &cy_http_response );
    if( result != CY_RSLT_SUCCESS )
    {
        cy_az_log_msg( CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_http_client_send failed with Error = [0x%X]\n", ( unsigned int )result );
    }

    return _az_http_client_error_code_to_result( result );
}
