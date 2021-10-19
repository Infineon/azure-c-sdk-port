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
 * @file mqtt_iot_provisioning_sample.c
 * Implementation of Azure DPS sample application on Cypress platforms
 */

/*
 * Macro value 0 indicates that SAS token-based authentication is not used.
 * Macro value 1 indicates that SAS token-based authentication is used.
 */
#define SAS_TOKEN_AUTH                  0

/*
 * Macro value true indicates that the SAS token needs to be read from the flash memory (from application buffer).
 * Macro value false indicates that the SAS token needs to be read from the secured memory (only for secured platform device).
 */
#define SAS_TOKEN_LOCATION_FLASH        true

#include <stdio.h>
#include "cyhal.h"
#include "cybsp.h"
#include "cy_log.h"
#include <lwip/tcpip.h>
#include <lwip/api.h>
#include <cy_retarget_io.h>
#include "cy_lwip.h"
#include <cybsp_wifi.h>
#include "cy_secure_sockets.h"
#include "cy_nw_helper.h"
#include "cy_wcm.h"
#include <FreeRTOS.h>
#include <task.h>
#include "cyabs_rtos.h"
#include <azure/core/az_http_transport.h>

#include "mqtt_main.h"
#include "cy_mqtt_api.h"
#include <az_core.h>
#include <az_iot.h>
#include "mqtt_iot_sample_common.h"
#include "az_iot_provisioning_client.h"

#ifdef CY_TFM_PSA_SUPPORTED
#include "cy_wdt.h"
#include "tfm_multi_core_api.h"
#include "tfm_ns_interface.h"
#include "tfm_ns_mailbox.h"
#include "psa/protected_storage.h"
#endif

#define test_result_t cy_rslt_t
#define TEST_PASS  CY_RSLT_SUCCESS
#define TEST_FAIL  ( -1 )

#define TEST_DEBUG( x )                       printf x
#define TEST_INFO( x )                        printf x
#define TEST_ERROR( x )                       printf x

/************************************************************
 *                         Constants                        *
 ************************************************************/
static  cy_wcm_config_t         wcm_config;

/* Enable RTOS-aware debugging */
volatile int                   uxTopUsedPriority ;
static cy_mqtt_t               mqtthandle;
static cy_queue_t              dps_operation_query_event_queue = NULL;
volatile bool                  connect_state = false;

static iot_sample_environment_variables      env_vars;
static az_iot_provisioning_client            provisioning_client;
static char                                  mqtt_client_username_buffer[IOT_SAMPLE_APP_BUFFER_SIZE_IN_BYTES];
static char                                  mqtt_endpoint_buffer[IOT_SAMPLE_APP_BUFFER_SIZE_IN_BYTES];

#if SAS_TOKEN_AUTH
static iot_sample_credentials              sas_credentials;
static char                                device_id_buffer[IOT_SAMPLE_APP_BUFFER_SIZE_IN_BYTES];
static char                                sas_token_buffer[IOT_SAMPLE_APP_BUFFER_SIZE_IN_BYTES];
#endif

#ifdef CY_TFM_PSA_SUPPORTED
static struct                              ns_mailbox_queue_t ns_mailbox_queue;
#endif

/**
 * @brief The network buffer must remain valid for the lifetime of the MQTT context.
 */
static uint8_t                             *buffer = NULL;

test_result_t ConnectWifi();

/*-----------------------------------------------------------*/
#ifdef CY_TFM_PSA_SUPPORTED
static void tfm_ns_multi_core_boot(void)
{
    int32_t ret;

    if (tfm_ns_wait_for_s_cpu_ready())
    {
        /* Error sync'ing with secure core */
        /* Avoid undefined behavior after multi-core sync-up failed. */
        for (;;)
        {
        }
    }

    ret = tfm_ns_mailbox_init(&ns_mailbox_queue);
    if (ret != MAILBOX_SUCCESS)
    {
        /* Non-secure mailbox initialization failed. */
        /* Avoid undefined behavior after NS mailbox initialization failed. */
        for (;;)
        {
        }
    }
}
#endif

/*-----------------------------------------------------------*/
test_result_t WCMConnectAP()
{
    printf( "Executing WCM Connect to AP \n" );
    cy_rslt_t res;
    cy_wcm_connect_params_t connect_param;
    cy_wcm_ip_address_t ip_addr;
    int retry_count = 0;

    memset(&connect_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(connect_param.ap_credentials.SSID, WIFI_SSID, strlen(WIFI_SSID) + 1);
    memcpy(connect_param.ap_credentials.password, WIFI_KEY, strlen(WIFI_KEY) + 1);
    connect_param.ap_credentials.security = CY_WCM_SECURITY_WPA2_AES_PSK;

    while (1)
    {
         /*
         * Join Wi-Fi AP
         */
         res = cy_wcm_connect_ap(&connect_param, &ip_addr);

         TEST_INFO(( "CY_RSLT TYPE = %lx MODULE = %lx CODE= %lx\n", CY_RSLT_GET_TYPE(res), CY_RSLT_GET_MODULE(res),
                                                                   CY_RSLT_GET_CODE(res)));
         if (res != CY_RSLT_SUCCESS)
         {
              retry_count++;
              if (retry_count >= MAX_WIFI_RETRY_COUNT)
              {
                  TEST_INFO(( "Exceeded max WiFi connection attempts\n" ));
                  return TEST_FAIL;
              }
              TEST_INFO(( "Connection to WiFi network failed. Retrying...\n" ));
              continue;
         }
         else
         {
              TEST_INFO(( "Successfully joined WiFi network %s \n", connect_param.ap_credentials.SSID ));
              break;
         }
    }

   return TEST_PASS;

}

test_result_t ConnectWifi()
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    wcm_config.interface = CY_WCM_INTERFACE_TYPE_STA;

    res = cy_wcm_init(&wcm_config);
    if(res != CY_RSLT_SUCCESS)
    {
        TEST_INFO(( "cy_wcm_init failed res=%lx \r\n",CY_RSLT_GET_CODE(res)));
        return TEST_FAIL;
    }

    if (WCMConnectAP() != TEST_PASS)
    {
        TEST_INFO(( "WCM connect to AP failed \r\n" ));
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t send_operation_query_message( az_iot_provisioning_client_register_response const* register_response )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    int rc;
    size_t topic_len;
    cy_mqtt_publish_info_t pub_msg;

    /* Get the Query Status topic to publish the query status request. */
    char query_topic_buffer[256];

    memset( &query_topic_buffer, 0x00, sizeof(query_topic_buffer) );
    memset( &pub_msg, 0x00, sizeof( cy_mqtt_publish_info_t ) );

    rc = az_iot_provisioning_client_query_status_get_publish_topic( &provisioning_client,
                                                                    register_response->operation_id,
                                                                    query_topic_buffer,
                                                                    sizeof(query_topic_buffer),
                                                                    &topic_len );
    if( az_result_failed(rc) )
    {
        IOT_SAMPLE_LOG_ERROR( "Unable to get query status publish topic: az_result return code 0x%08x.", rc );
        return ( (cy_rslt_t)TEST_FAIL );
    }

    /* IMPORTANT: Wait for the recommended retry-after number of seconds before query. */
    IOT_SAMPLE_LOG( "Querying after %u seconds...", (unsigned int)register_response->retry_after_seconds );
    vTaskDelay( register_response->retry_after_seconds );

    /* Publish the query status request .*/

    pub_msg.qos = (cy_mqtt_qos_t)CY_MQTT_QOS0;
    pub_msg.topic = (const char *)&query_topic_buffer;
    pub_msg.topic_len = topic_len;
    pub_msg.payload = NULL;
    pub_msg.payload_len = 0;

    result = cy_mqtt_publish( mqtthandle, &pub_msg );
    if( result == CY_RSLT_SUCCESS )
    {
        TEST_INFO(( "\r\ncy_mqtt_publish completed........\n\r" ));
    }
    else
    {
        TEST_INFO(( "\r\nFailed to publish query status request with Error : [0x%X] ", (unsigned int)result ));
    }
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t handle_device_registration_status_message( az_iot_provisioning_client_register_response const* register_response,
                                                            bool* ref_is_operation_complete )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    *ref_is_operation_complete = az_iot_provisioning_client_operation_complete( register_response->operation_status );

    /* If the operation is not complete, send query. On return, will loop to receive a new operation
       message. */
    if ( !*ref_is_operation_complete )
    {
        IOT_SAMPLE_LOG( "Operation is still pending." );

        TEST_INFO(( "\n\rPushing to dps operation query queue...." ));
        result = cy_rtos_put_queue( &dps_operation_query_event_queue, (void *)&register_response, 500, false );
        if( result != CY_RSLT_SUCCESS )
        {
            TEST_INFO(( "\n\r Pushing to dps operation query queue failed with Error : [0x%X] ", (unsigned int)result ));
        }
    }
    else /* Operation is complete. */
    {
        if( register_response->operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED ) /* Successful assignment. */
        {
            IOT_SAMPLE_LOG_SUCCESS( "Device provisioned:" );
            IOT_SAMPLE_LOG_AZ_SPAN( "Hub Hostname:", register_response->registration_state.assigned_hub_hostname );
            IOT_SAMPLE_LOG_AZ_SPAN( "Device Id:", register_response->registration_state.device_id );
            IOT_SAMPLE_LOG( " " ); // Formatting
         }
        else /* Unsuccessful assignment (unassigned, failed, or disabled states) .*/
        {
            IOT_SAMPLE_LOG_ERROR( "Device provisioning failed:" );
            IOT_SAMPLE_LOG( "Registration state: %d", register_response->operation_status );
            IOT_SAMPLE_LOG( "Last operation status: %d", register_response->status );
            IOT_SAMPLE_LOG_AZ_SPAN( "Operation ID:", register_response->operation_id );
            IOT_SAMPLE_LOG( "Error code: %u", (unsigned int)register_response->registration_state.extended_error_code );
            IOT_SAMPLE_LOG_AZ_SPAN( "Error message:", register_response->registration_state.error_message );
            IOT_SAMPLE_LOG_AZ_SPAN( "Error timestamp:", register_response->registration_state.error_timestamp );
            IOT_SAMPLE_LOG_AZ_SPAN( "Error tracking ID:", register_response->registration_state.error_tracking_id );
            IOT_SAMPLE_LOG( "extended_error_code: %u",(unsigned int)register_response->registration_state.extended_error_code );
            result =  (cy_rslt_t)TEST_FAIL;
        }
    }
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t parse_device_registration_status_message( char* topic, int topic_len,
                                                           cy_mqtt_publish_info_t *message,
                                                           az_iot_provisioning_client_register_response* out_register_response)
{
    az_result rc;
    az_span const topic_span = az_span_create( (uint8_t*)topic, topic_len );
    az_span const message_span = az_span_create( (uint8_t*)message->payload, message->payload_len );

    /* Parse the message and retrieve the register_response info. */
    rc = az_iot_provisioning_client_parse_received_topic_and_payload( &provisioning_client, topic_span,
                                                                      message_span, out_register_response );
    if( az_result_failed(rc) )
    {
        IOT_SAMPLE_LOG_ERROR( "Message from unknown topic: az_result return code 0x%08x.", (unsigned int)rc );
        IOT_SAMPLE_LOG_AZ_SPAN( "Topic:", topic_span );
        return ( (cy_rslt_t)TEST_FAIL );
    }
    IOT_SAMPLE_LOG_SUCCESS( "Client received a valid topic response:" );
    IOT_SAMPLE_LOG_AZ_SPAN( "Topic:", topic_span );
    IOT_SAMPLE_LOG_AZ_SPAN( "Payload:", message_span );
    IOT_SAMPLE_LOG( "Status: %d", out_register_response->status );
    return CY_RSLT_SUCCESS;
}

/*--------------------------------------------------------------------------------------------------------*/
void mqtt_event_cb( cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *arg )
{
    cy_mqtt_received_msg_info_t *received_msg = NULL;
    bool is_operation_complete = false;
    az_iot_provisioning_client_register_response *register_response = NULL;

    TEST_INFO(( "\r\nMQTT App callback with handle : %p \n", mqtt_handle ));
    switch( event.type )
    {
        case CY_MQTT_EVENT_TYPE_DISCONNECT :
            if( event.data.reason == CY_MQTT_DISCONN_TYPE_BROKER_DOWN )
            {
                TEST_INFO(( "\r\nCY_MQTT_DISCONN_TYPE_BROKER_DOWN .....\n" ));
            }
            else
            {
                TEST_INFO(( "\r\nCY_MQTT_DISCONN_REASON_NETWORK_DISCONNECTION .....\n" ));
            }
            connect_state = false;
            break;

        case CY_MQTT_EVENT_TYPE_PUBLISH_RECEIVE :
            received_msg = &(event.data.pub_msg.received_message);
            register_response = (az_iot_provisioning_client_register_response *)malloc( sizeof( az_iot_provisioning_client_register_response ) );
            if( register_response == NULL )
            {
                IOT_SAMPLE_LOG( "Memory not available for register_response." );
            }
            else
            {
                /* Parse the registration status message. */
                parse_device_registration_status_message( (char*)received_msg->topic, received_msg->topic_len,
                                                          received_msg, register_response );
                IOT_SAMPLE_LOG_SUCCESS( "Client parsed registration status message." );
                handle_device_registration_status_message( register_response, &is_operation_complete );
            }
            break;

        default :
            TEST_INFO(( "\r\nUNKNOWN EVENT .....\n" ));
            break;
    }
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t register_device_with_provisioning_service( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    int rc;
    size_t topic_len;
    cy_mqtt_publish_info_t pub_msg;

    /* Get the Register topic to publish the register request. */
    char register_topic_buffer[128];

    memset( &register_topic_buffer, 0x00, sizeof(register_topic_buffer) );
    memset( &pub_msg, 0x00, sizeof( cy_mqtt_publish_info_t ) );

    rc = az_iot_provisioning_client_register_get_publish_topic( &provisioning_client,
                                                                register_topic_buffer,
                                                                sizeof(register_topic_buffer), (size_t *)&topic_len );
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR( "Failed to get the Register topic: az_result return code 0x%08x.", rc );
        return ( (cy_rslt_t)TEST_FAIL );
    }

    pub_msg.qos = (cy_mqtt_qos_t)CY_MQTT_QOS1;
    pub_msg.topic = (const char *)&register_topic_buffer;
    pub_msg.topic_len = topic_len;
    pub_msg.payload = NULL;
    pub_msg.payload_len = 0;

    result = cy_mqtt_publish( mqtthandle, &pub_msg );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\r\ncy_mqtt_publish completed........\n\r" ));
    }
    else
    {
        TEST_INFO(( "\r\ncy_mqtt_publish failed with Error : [0x%X] ", (unsigned int)result ));
        return result;
    }
    TEST_INFO(( "\r\nSent publish message to place register request" ));
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t subscribe_mqtt_client_to_provisioning_service_topics( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_mqtt_subscribe_info_t sub_msg[1];

    memset( &sub_msg, 0x00, sizeof( cy_mqtt_subscribe_info_t ) );
    sub_msg[0].qos = (cy_mqtt_qos_t)CY_MQTT_QOS1;
    sub_msg[0].topic = AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC;
    sub_msg[0].topic_len = ( uint16_t ) ( sizeof( AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC ) - 1 );

    result = cy_mqtt_subscribe( mqtthandle, &sub_msg[0], 1 );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\ncy_mqtt_subscribe ------------------------ Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\ncy_mqtt_subscribe ------------------------ Fail \n" ));
    }

    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t disconnect_mqtt_client_from_provisioning_service( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    result = cy_mqtt_disconnect( mqtthandle );
    if( result == CY_RSLT_SUCCESS )
    {
        TEST_INFO(( "\n\rcy_mqtt_disconnect ----------------------- Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\n\rcy_mqtt_disconnect ----------------------- Fail \n" ));
    }
    connect_state = false;

    result = cy_mqtt_delete( mqtthandle );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\n\rcy_mqtt_delete --------------------------- Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\n\rcy_mqtt_delete --------------------------- Fail \n" ));
    }

    result = cy_mqtt_deinit();
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\n\rcy_mqtt_deinit --------------------------- Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\n\rcy_mqtt_deinit --------------------------- Fail \n" ));
    }

    /* Free the network buffer */
    if( buffer != NULL )
    {
        free( buffer );
        buffer = NULL;
    }

    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t connect_mqtt_client_to_provisioning_service(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    int rc;
    size_t username_len = 0, client_id_len = 0;
    cy_mqtt_connect_info_t connect_info;

    /* Get the MQTT client ID used for the MQTT connection */
    char mqtt_client_id_buffer[128];

    memset( &connect_info, 0x00, sizeof( cy_mqtt_connect_info_t ) );
    memset( &mqtt_client_id_buffer, 0x00, sizeof( mqtt_client_id_buffer ) );

    rc = az_iot_provisioning_client_get_client_id( &provisioning_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), &client_id_len );
    if( az_result_failed(rc) )
    {
        TEST_INFO(( "\r\nFailed to get MQTT client id: az_result return code 0x%08x.", rc ));
        return TEST_FAIL;
    }

    /* Get the MQTT client user name. */
    rc = az_iot_provisioning_client_get_user_name( &provisioning_client, mqtt_client_username_buffer,
                                                   sizeof(mqtt_client_username_buffer), &username_len );
    if( az_result_failed(rc) )
    {
        TEST_INFO(( "\r\nFailed to get MQTT client username: az_result return code 0x%08x.", rc ));
        return TEST_FAIL;
    }

    connect_info.client_id = mqtt_client_id_buffer;
    connect_info.client_id_len = client_id_len;
    connect_info.keep_alive_sec = MQTT_KEEP_ALIVE_INTERVAL_SECONDS;
    connect_info.clean_session = false;
    connect_info.will_info = NULL;

#if SAS_TOKEN_AUTH
    connect_info.username = mqtt_client_username_buffer;
    connect_info.password = (char *)sas_credentials.sas_token;
    connect_info.username_len = username_len;
    connect_info.password_len = sas_credentials.sas_token_len;
#else
    connect_info.username = mqtt_client_username_buffer;
    connect_info.password = NULL;
    connect_info.username_len = username_len;
    connect_info.password_len = 0;
#endif

    result = cy_mqtt_connect( mqtthandle, &connect_info );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\r\ncy_mqtt_connect -------------------------- Pass \n" ));
        connect_state = true;
    }
    else
    {
        TEST_INFO(( "\r\ncy_mqtt_connect -------------------------- Fail \n" ));
        return TEST_FAIL;
    }

    return CY_RSLT_SUCCESS;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t create_and_configure_mqtt_client( void )
{
    int rc;
    uint16_t ep_size = 0;
    cy_rslt_t result = TEST_PASS;
    cy_awsport_ssl_credentials_t credentials;
    cy_awsport_ssl_credentials_t *security = NULL;
    cy_mqtt_broker_info_t broker_info;

    memset( &mqtt_endpoint_buffer, 0x00, sizeof(mqtt_endpoint_buffer) );

    ep_size = iot_sample_create_mqtt_endpoint( CY_MQTT_IOT_PROVISIONING, &env_vars,
                                     mqtt_endpoint_buffer,
                                     sizeof(mqtt_endpoint_buffer) );

    /* Initialize the provisioning client with the provisioning global endpoint and the default connection options */
    rc = az_iot_provisioning_client_init( &provisioning_client,
                                          az_span_create_from_str(mqtt_endpoint_buffer),
                                          env_vars.provisioning_id_scope,
                                          env_vars.provisioning_registration_id, NULL );
    if( az_result_failed( rc ) )
    {
        TEST_INFO(( "\r\nFailed to initialize hub client: az_result return code 0x%08x.", (unsigned int)rc ));
        return TEST_FAIL;
    }

    /* Allocate the network buffer */
    buffer = (uint8_t *) malloc( sizeof(uint8_t) * NETWORK_BUFFER_SIZE );
    if( buffer == NULL )
    {
        TEST_INFO(( "\r\nNo memory is available for network buffer..! \n" ));
        return TEST_FAIL;
    }

    memset( &broker_info, 0x00, sizeof( cy_mqtt_broker_info_t ) );
    memset( &credentials, 0x00, sizeof( cy_awsport_ssl_credentials_t ) );

    result = cy_mqtt_init();
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\ncy_mqtt_init ----------------------------- Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\ncy_mqtt_init ----------------------------- Fail \n" ));
        return TEST_FAIL;
    }

#if SAS_TOKEN_AUTH
    credentials.client_cert = (const char *)NULL;
    credentials.client_cert_size = 0;
    credentials.private_key = (const char *)NULL;
    credentials.private_key_size = 0;
    credentials.root_ca = (const char *) NULL;
    credentials.root_ca_size = 0;
    /* For SAS token based auth mode, RootCA verification is not required. */
    credentials.root_ca_verify_mode = CY_AWS_ROOTCA_VERIFY_NONE;
    /* Set cert and key location. */
    credentials.cert_key_location = CY_AWS_CERT_KEY_LOCATION_RAM;
    credentials.root_ca_location = CY_AWS_CERT_KEY_LOCATION_RAM;
#else
    credentials.client_cert = (const char *) &azure_client_cert;
    credentials.client_cert_size = IOT_AZURE_CLIENT_CERT_LENGTH;
    credentials.private_key = (const char *) &azure_client_key;
    credentials.private_key_size = IOT_AZURE_CLIENT_KEY_LENGTH;
    credentials.root_ca = (const char *) &azure_root_ca_certificate;
    credentials.root_ca_size = IOT_AZURE_ROOT_CA_LENGTH;
#endif
    broker_info.hostname = (const char *)&mqtt_endpoint_buffer;
    broker_info.hostname_len = (uint16_t)ep_size;
    broker_info.port = IOT_DEMO_PORT_AZURE_S;
    security = &credentials;

    result = cy_mqtt_create( buffer, NETWORK_BUFFER_SIZE,
                             security, &broker_info,
                             (cy_mqtt_callback_t)mqtt_event_cb, NULL,
                             &mqtthandle );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\r\ncy_mqtt_create ----------------------------- Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\r\ncy_mqtt_create ----------------------------- Fail \n" ));
        return TEST_FAIL;
    }
#if 0
    /* Generate the shared access signature (SAS) key here for SAS-based authentication */
#endif
    return CY_RSLT_SUCCESS;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t configure_hub_environment_variables( void )
{
    cy_rslt_t result = TEST_PASS;
#if SAS_TOKEN_AUTH
    env_vars.provisioning_sas_key._internal.ptr = (uint8_t*)sas_credentials.sas_token;
    env_vars.provisioning_sas_key._internal.size = (int32_t)sas_credentials.sas_token_len;
#else
    env_vars.provisioning_sas_key._internal.ptr = NULL;
    env_vars.provisioning_sas_key._internal.size = 0;
#endif
    env_vars.hub_hostname._internal.ptr = (uint8_t*)IOT_DEMO_SERVER_AZURE;
    env_vars.hub_hostname._internal.size = strlen(IOT_DEMO_SERVER_AZURE);
    env_vars.provisioning_registration_id._internal.ptr = (uint8_t*)IOT_AZURE_DPS_REGISTRATION_ID;
    env_vars.provisioning_registration_id._internal.size = (uint8_t)IOT_AZURE_DPS_REGISTRATION_ID_LEN;
    env_vars.provisioning_id_scope._internal.ptr = (uint8_t*)IOT_AZURE_ID_SCOPE;
    env_vars.provisioning_id_scope._internal.size = (uint8_t)IOT_AZURE_ID_SCOPE_LEN;
    env_vars.sas_key_duration_minutes = 240;
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static void azure_dps_app_task( void *arg )
{
    cy_rslt_t TestRes = TEST_PASS ;
    uint8_t Failcount = 0, Passcount = 0;
    uint32_t time_ms = 0;
    az_iot_provisioning_client_register_response *register_response = NULL;
#ifdef CY_TFM_PSA_SUPPORTED
    psa_status_t uxStatus = PSA_SUCCESS;
    size_t read_len = 0;
    (void)uxStatus;
    (void)read_len;
#endif

    cy_log_init( CY_LOG_ERR, NULL, NULL );

    TEST_INFO(( "\r\n###################DPS Application Start################### \n\n" ));

    TestRes = ConnectWifi();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\nConnectWifi ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\nConnectWifi ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    /*--------------------------------------------------------------------*/
    /*
     * Initialize the queue for hub method events
     */
    TestRes = cy_rtos_init_queue( &dps_operation_query_event_queue, 10, sizeof(az_iot_provisioning_client_register_response *) );
    if( TestRes == CY_RSLT_SUCCESS )
    {
        TEST_INFO(( "\r\n cy_rtos_init_queue ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n cy_rtos_init_queue ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    /*--------------------------------------------------------------------*/
#if SAS_TOKEN_AUTH
    (void)device_id_buffer;
    (void)sas_token_buffer;
#if ( (defined CY_TFM_PSA_SUPPORTED) && ( SAS_TOKEN_LOCATION_FLASH == false ) )
    /* Read Device ID from Secured memory */
    uxStatus = psa_ps_get( PSA_DEVICEID_UID, 0, sizeof(device_id_buffer), device_id_buffer, &read_len );
    if( uxStatus == PSA_SUCCESS )
    {
        device_id_buffer[read_len] = '\0';
        TEST_INFO(( "\r\nRetrieved Device ID : %s\r\n", device_id_buffer ));
        sas_credentials.device_id = (uint8_t*)device_id_buffer;
        sas_credentials.device_id_len = read_len;
    }
    else
    {
        TEST_INFO(( "\r\n psa_ps_get for Device ID failed with %d\n", (int)uxStatus ));
        TEST_INFO(( "\r\n Taken Device ID from MQTT_CLIENT_IDENTIFIER_AZURE_SAS macro. \n" ));
        sas_credentials.device_id = (uint8_t*)MQTT_CLIENT_IDENTIFIER_AZURE_SAS;
        sas_credentials.device_id_len = MQTT_CLIENT_IDENTIFIER_AZURE_SAS_LENGTH;
    }

    read_len = 0;

    /* Read SAS token from Secured memory */
    uxStatus = psa_ps_get( PSA_SAS_TOKEN_UID, 0, sizeof(sas_token_buffer), sas_token_buffer, &read_len );
    if( uxStatus == PSA_SUCCESS )
    {
        sas_token_buffer[read_len] = '\0';
        TEST_INFO(( "\r\nRetrieved SAS token : %s\r\n", sas_token_buffer ));
        sas_credentials.sas_token = (uint8_t*)sas_token_buffer;
        sas_credentials.sas_token_len = read_len;
    }
    else
    {
        TEST_INFO(( "\r\n psa_ps_get for sas_token failed with %d\n", (int)uxStatus ));
        TEST_INFO(( "\r\n Taken SAS Token from IOT_AZURE_PASSWORD macro. \n" ));
        sas_credentials.sas_token = (uint8_t *)IOT_AZURE_PASSWORD;
        sas_credentials.sas_token_len = IOT_AZURE_PASSWORD_LENGTH;
    }
#else
    sas_credentials.device_id = (uint8_t*)MQTT_CLIENT_IDENTIFIER_AZURE_SAS;
    sas_credentials.device_id_len = MQTT_CLIENT_IDENTIFIER_AZURE_SAS_LENGTH;
    sas_credentials.sas_token = (uint8_t*)IOT_AZURE_PASSWORD;
    sas_credentials.sas_token_len = IOT_AZURE_PASSWORD_LENGTH;
#endif

#endif /* SAS_TOKEN_AUTH */

    /*--------------------------------------------------------------------*/
    TestRes = configure_hub_environment_variables();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\n configure_environment_variables ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n configure_environment_variables ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    /*--------------------------------------------------------------------*/
    TestRes = create_and_configure_mqtt_client();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\n create_and_configure_mqtt_client ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n create_and_configure_mqtt_client ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    /*--------------------------------------------------------------------*/
    TestRes = connect_mqtt_client_to_provisioning_service();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\n connect_mqtt_client_to_iot_hub ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n connect_mqtt_client_to_iot_hub ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    /*--------------------------------------------------------------------*/
    TestRes = subscribe_mqtt_client_to_provisioning_service_topics();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\n subscribe_mqtt_client_to_iot_hub_topics ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n subscribe_mqtt_client_to_iot_hub_topics ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    TestRes = register_device_with_provisioning_service();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\n register_device_with_provisioning_service ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n register_device_with_provisioning_service ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    /*--------------------------------------------------------------------*/
    /* 2 Min check to make the app to work with CI/CD */
    time_ms = ( 120 * 1000 );
    while( connect_state && (time_ms > 0) )
    {
        TestRes = cy_rtos_get_queue( &dps_operation_query_event_queue, (void *)&register_response, 500, false );
        if( TestRes != CY_RSLT_SUCCESS )
        {
            TEST_INFO(( "\r\n cy_rtos_get_queue returned with status [0x%X] ", (unsigned int)TestRes ));
        }
        else
        {
            if( register_response != NULL )
            {
                send_operation_query_message( register_response );
                IOT_SAMPLE_LOG_SUCCESS( "Client sent operation query message." );
                free( register_response );
                register_response = NULL;
            }
        }
        time_ms = time_ms - 500;
    }
    /*--------------------------------------------------------------------*/
exit:
    TestRes = disconnect_mqtt_client_from_provisioning_service();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\n disconnect_mqtt_client_from_iot_hub ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n disconnect_mqtt_client_from_iot_hub ----------- Fail \n" ));
        Failcount++;
    }

    /*************************************************************************************************/

    TEST_INFO(( "\r\nCompleted MQTT Client Test Cases --------------------------\n" ));
    TEST_INFO(( "\r\nTotal Test Cases   ---------------------- %d\n", ( Failcount + Passcount ) ));
    TEST_INFO(( "\r\nTest Cases passed  ---------------------- %d\n", Passcount ));
    TEST_INFO(( "\r\nTest Cases failed  ---------------------- %d\n", Failcount ));

    /*****************************************************************************************************/

    while( 1 )
    {
        vTaskDelay( 5000 );
    }
}

/*-----------------------------------------------------------*/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    (void)(result);

#ifdef CY_TFM_PSA_SUPPORTED
    /* Unlock and disable the WDT */
    Cy_WDT_Unlock();
    Cy_WDT_Disable();
#endif

    /* Initialize the board support package */
    result = cybsp_init() ;
    CY_ASSERT( result == CY_RSLT_SUCCESS );
    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    /* This is not required because target_io_init is done as part of BeginTesting */
    cy_retarget_io_init( CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE );

#ifdef CY_TFM_PSA_SUPPORTED
    tfm_ns_multi_core_boot();
    /* Initialize the TFM interface */
    tfm_ns_interface_init();
#endif

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    xTaskCreate( azure_dps_app_task, "MQTT_DPS", 10*4096, NULL, 1, NULL ) ;

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler() ;

    return 0;
}

/*-----------------------------------------------------------*/
