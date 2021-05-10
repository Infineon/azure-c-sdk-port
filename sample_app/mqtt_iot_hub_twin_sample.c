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
 * @file mqtt_iot_hub_twin_sample.c
 * Implementation of Azure hub twin state sample application on Cypress platforms
 */

#define SAS_TOKEN_AUTH                       1

#include <stdio.h>
#include "cyhal.h"
#include "cybsp.h"
#include "cy_log.h"
#include <lwip/tcpip.h>
#include <lwip/api.h>
#include <cy_retarget_io.h>
#include "cy_lwip.h"
#include <cybsp_wifi.h>
#include "cy_nw_helper.h"

#include <FreeRTOS.h>
#include <task.h>

#include "cyabs_rtos.h"

#include "mqtt_main.h"
#include "cy_mqtt_api.h"
#include <az_core.h>
#include <az_iot.h>
#include "mqtt_iot_sample_common.h"

#define test_result_t cy_rslt_t
#define TEST_PASS  CY_RSLT_SUCCESS
#define TEST_FAIL  ( -1 )

#define TEST_DEBUG( x )                       printf x
#define TEST_INFO( x )                        printf x
#define TEST_ERROR( x )                       printf x

#define MAX_TWIN_MESSAGE_COUNT 5

static az_span const twin_document_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("get_twin");
static az_span const twin_patch_topic_request_id = AZ_SPAN_LITERAL_FROM_STR("reported_prop");
static az_span const version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static az_span const desired_device_count_property_name = AZ_SPAN_LITERAL_FROM_STR("Test_count");
static int32_t device_count_value = 0;

/************************************************************
 *                         Constants                        *
 ************************************************************/
static whd_interface_t                     iface ;
static cy_mqtt_t                           mqtthandle;
static iot_sample_environment_variables    env_vars;
static az_iot_hub_client                   hub_client;
static char                                mqtt_client_username_buffer[128];
static char                                mqtt_endpoint_buffer[128];
static cy_semaphore_t                      twin_app_sem = NULL;                         ;
volatile bool                              connect_state = false;

/**
 * @brief The network buffer must remain valid for the lifetime of the MQTT context
 */
static uint8_t                             *buffer = NULL;

static void donothing(void *arg)
{

}

/*-----------------------------------------------------------*/
test_result_t ConnectWifi()
{
    cy_rslt_t res = CY_RSLT_SUCCESS;

    const char *ssid = WIFI_SSID ;
    const char *key = WIFI_KEY ;
    int retry_count = 0;

    whd_ssid_t ssiddata ;
    tcpip_init(donothing, NULL) ;

    cy_lwip_nw_interface_t nw_interface;

    printf("lwIP TCP/IP stack initialized\r\n") ;
    /*
     *   Initialize the Wi-Fi driver
     */
    cybsp_wifi_init_primary(&iface) ;
    printf("WIFI driver initialized \r\n") ;

    while (1)
    {
        /*
        * Join the Wi-Fi AP
        */
        ssiddata.length = strlen(ssid) ;
        memcpy(ssiddata.value, ssid, ssiddata.length) ;
        res = whd_wifi_join(iface, &ssiddata, WHD_SECURITY_WPA2_AES_PSK, (const uint8_t *)key, strlen(key)) ;
        vTaskDelay(500);

        if (res != CY_RSLT_SUCCESS)
        {
            retry_count++;
            if (retry_count >= MAX_WIFI_RETRY_COUNT)
            {
                TEST_ERROR(( "\n\rExceeded max WiFi connection attempts\n" ));
                return TEST_FAIL;
            }
            TEST_INFO(( "\n\rConnection to WiFi network failed. Retrying...\n" ));
            continue;
        }
        else
        {
            TEST_INFO(( "\n\rSuccessfully joined WiFi network %s \n", ssid ));
            break;
        }
    }

    nw_interface.role = CY_LWIP_STA_NW_INTERFACE;
    nw_interface.whd_iface = iface;

    /* Add interface to lwIP*/
    cy_lwip_add_interface( &nw_interface, NULL ) ;

    /* Bring up the network*/
    cy_lwip_network_up( &nw_interface );

    struct netif *net = cy_lwip_get_interface( CY_LWIP_STA_NW_INTERFACE );

    while (true)
    {
        if (net->ip_addr.u_addr.ip4.addr != 0)
        {
            printf("\n\rIP Address %s assigned\r\n", ip4addr_ntoa(&net->ip_addr.u_addr.ip4)) ;
            break ;
        }
        vTaskDelay(100) ;
    }
    return TEST_PASS;
}

/*--------------------------------------------------------------------------------------------------------*/
static void build_reported_property(
    az_span reported_property_payload,
    az_span* out_reported_property_payload)
{
    char const* const log = "Failed to build reported property payload";

    az_json_writer jw;
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, reported_property_payload, NULL), log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(
        az_json_writer_append_property_name(&jw, desired_device_count_property_name), log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_int32(&jw, device_count_value), log);
    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);

    *out_reported_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

/*--------------------------------------------------------------------------------------------------------*/
static void send_reported_property( void )
{
    int rc;
    uint16_t topic_len = 0;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_mqtt_publish_info_t pub_msg;
    char reported_property_payload_buffer[128];
    char twin_patch_topic_buffer[128];

    memset( &pub_msg, 0x00, sizeof( cy_mqtt_publish_info_t ) );
    memset( &reported_property_payload_buffer, 0x00, sizeof( reported_property_payload_buffer ) );
    memset( &twin_patch_topic_buffer, 0x00, sizeof( twin_patch_topic_buffer ) );

    IOT_SAMPLE_LOG("\n\rClient sending reported property to service.");

    // Get the Twin Patch topic to publish a reported property update
    rc = az_iot_hub_client_twin_patch_get_publish_topic(
        &hub_client,
        twin_patch_topic_request_id,
        twin_patch_topic_buffer,
        sizeof(twin_patch_topic_buffer),
        (size_t*)&topic_len);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("\n\rFailed to get the Twin Patch topic: az_result return code 0x%08x.", rc);
        return;
    }

    az_span reported_property_payload = AZ_SPAN_FROM_BUFFER(reported_property_payload_buffer);
    build_reported_property( reported_property_payload, &reported_property_payload );

    pub_msg.qos = (cy_mqtt_qos_t)CY_MQTT_QOS0;
    pub_msg.topic = (const char *)&twin_patch_topic_buffer;
    pub_msg.topic_len = topic_len;
    pub_msg.payload = (const char *)reported_property_payload._internal.ptr;
    pub_msg.payload_len = (size_t)reported_property_payload._internal.size;

    // Publish the reported property update
    result = cy_mqtt_publish( mqtthandle, &pub_msg );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\r\ncy_mqtt_publish completed........\n\r" ));
    }
    else
    {
        TEST_INFO(( "\r\ncy_mqtt_publish failed with Error : [0x%X] ", (unsigned int)result ));
        return;
    }

    TEST_INFO(( "\r\nClient published the Twin Patch reported property message." ));
    TEST_INFO(( "\r\nPayload: %.*s\r\n", (int)reported_property_payload._internal.size, reported_property_payload._internal.ptr ));
}

/*--------------------------------------------------------------------------------------------------------*/
static void update_device_count_property(int32_t device_count)
{
    device_count_value = device_count;
    TEST_INFO(( "\r\n Client updated `%.*s` locally to %d.",
        (int)az_span_size(desired_device_count_property_name),
        az_span_ptr(desired_device_count_property_name),
        (int)device_count_value ));
}

/*--------------------------------------------------------------------------------------------------------*/
static bool parse_desired_device_count_property( az_span message_span,
                                                 int32_t* out_parsed_device_count )
{
    char const* const log = "Failed to parse for desired `%.*s` property";
    az_span property = desired_device_count_property_name;

    bool property_found = false;
    *out_parsed_device_count = 0;

    // Parse message_span
    az_json_reader jr;
    IOT_SAMPLE_EXIT_IF_AZ_FAILED( az_json_reader_init(&jr, message_span, NULL), log, property );
    IOT_SAMPLE_EXIT_IF_AZ_FAILED( az_json_reader_next_token(&jr), log, property );
    if( jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT )
    {
        TEST_INFO(( "\r\n`%.*s` property was not found in desired property response.",
        (int)az_span_size(property),
        az_span_ptr(property) ));
        return false;
    }

    IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);
    while (!property_found && (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT))
    {
        if( az_json_token_is_text_equal(&jr.token, desired_device_count_property_name) )
        {
            // Move to the value token
            IOT_SAMPLE_EXIT_IF_AZ_FAILED( az_json_reader_next_token(&jr), log, property );
            IOT_SAMPLE_EXIT_IF_AZ_FAILED( az_json_token_get_int32(&jr.token, out_parsed_device_count), log, property);
            property_found = true;
        }
        else if( az_json_token_is_text_equal(&jr.token, version_name) )
        {
            break;
        }
        else
        {
            IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_skip_children(&jr), log, property);
        }
        IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_reader_next_token(&jr), log, property);
    }

    if( property_found )
    {
        IOT_SAMPLE_LOG( "Parsed desired `%.*s`: %d",
                        (int)az_span_size(property),
                        az_span_ptr(property),
                        (int)*out_parsed_device_count );
    }
    else
    {
        IOT_SAMPLE_LOG( "`%.*s` property was not found in desired property response.",
                        (int)az_span_size(property),
                        az_span_ptr(property) );
        return false;
    }

    return true;
}

/*--------------------------------------------------------------------------------------------------------*/
static void handle_device_twin_message( cy_mqtt_publish_info_t *message,
                                        az_iot_hub_client_twin_response const* twin_response )
{
    az_span const message_span = az_span_create((uint8_t*)message->payload, message->payload_len);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    // Invoke the appropriate action per response type (3 types only)
    switch( twin_response->response_type )
    {
      case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET:
           IOT_SAMPLE_LOG("Message Type: GET");
           break;

      case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
           IOT_SAMPLE_LOG("Message Type: Reported Properties");
           break;

      case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
           IOT_SAMPLE_LOG("Message Type: Desired Properties");

           // Parse for the device count property
           int32_t desired_device_count = 0;
           if( parse_desired_device_count_property( message_span, &desired_device_count ) )
           {
               IOT_SAMPLE_LOG(" "); // Formatting
               // Update the device count locally and report the update to the server
               update_device_count_property( desired_device_count );

               TEST_INFO(( "\n\rReleasing twin_app_sem...." ));
               result = cy_rtos_set_semaphore( &twin_app_sem, false );
               if( result != CY_RSLT_SUCCESS )
               {
                   TEST_INFO(( "\n\r Releasing twin_app_sem failed with Error : [0x%X] ", (unsigned int)result ));
               }
           }
           break;
      }
}

/*--------------------------------------------------------------------------------------------------------*/
static void parse_device_twin_message( char* topic, int topic_len,
                                       cy_mqtt_publish_info_t *message,
                                       az_iot_hub_client_twin_response* out_twin_response )
{
    az_span const topic_span = az_span_create( (uint8_t*)topic, topic_len);
    az_span const message_span = az_span_create( (uint8_t*)message->payload, message->payload_len );

    // Parse the message and retrieve twin_response info
    az_result rc = az_iot_hub_client_twin_parse_received_topic( &hub_client, topic_span, out_twin_response );
    if( az_result_failed(rc) )
    {
        IOT_SAMPLE_LOG_ERROR( "Message from unknown topic: az_result return code 0x%08x.", (unsigned int)rc );
        IOT_SAMPLE_LOG_AZ_SPAN( "Topic:", topic_span );
        return;
    }
    IOT_SAMPLE_LOG_SUCCESS( "Client received a valid topic response." );
    IOT_SAMPLE_LOG_AZ_SPAN( "Topic:", topic_span );
    IOT_SAMPLE_LOG_AZ_SPAN( "Payload:", message_span );
    IOT_SAMPLE_LOG( "Status: %d", out_twin_response->status );
}

/*--------------------------------------------------------------------------------------------------------*/
void mqtt_event_cb( cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *arg )
{
    cy_mqtt_publish_info_t *received_msg;

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
            // Parse the device twin message
            az_iot_hub_client_twin_response twin_response;
            parse_device_twin_message( (char*)received_msg->topic, received_msg->topic_len, received_msg, &twin_response);
            TEST_INFO(( "\n\rClient parsed device twin message." ));
            handle_device_twin_message( received_msg, &twin_response);
            break;

        default :
            TEST_INFO(( "\r\nUNKNOWN EVENT .....\n" ));
            break;
    }
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t disconnect_and_delete_mqtt_client( void )
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
static void get_device_twin_document(void)
{
    int rc;
    uint16_t topic_len = 0;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_mqtt_publish_info_t pub_msg;
    char twin_document_topic_buffer[128];

    memset( &pub_msg, 0x00, sizeof( cy_mqtt_publish_info_t ) );
    memset( &twin_document_topic_buffer, 0x00, sizeof( twin_document_topic_buffer ) );
    IOT_SAMPLE_LOG("Client requesting device twin document from service.");

    rc = az_iot_hub_client_twin_document_get_publish_topic(
        &hub_client,
        twin_document_topic_request_id,
        twin_document_topic_buffer,
        sizeof(twin_document_topic_buffer),
        (size_t *)&topic_len);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR(
        "Failed to get the Twin Document topic: az_result return code 0x%08x.", rc);
        exit(rc);
    }

    pub_msg.qos = (cy_mqtt_qos_t)CY_MQTT_QOS0;
    pub_msg.topic = (const char *)&twin_document_topic_buffer;
    pub_msg.topic_len = topic_len;
    pub_msg.payload = (const char *)NULL;
    pub_msg.payload_len = (size_t)0;

    // Publish the twin document request
    result = cy_mqtt_publish( mqtthandle, &pub_msg );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\r\ncy_mqtt_publish completed........\n\r" ));
    }
    else
    {
        TEST_INFO(( "\r\ncy_mqtt_publish failed with Error : [0x%X] ", (unsigned int)result ));
        return;
    }
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t send_and_receive_device_twin_messages( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    get_device_twin_document();

    vTaskDelay( 10 * 1000 );

    //(void)receive_device_twin_message();
    IOT_SAMPLE_LOG(" "); // Formatting

    send_reported_property();
    //(void)receive_device_twin_message();
#if 0
    // Continue until max # messages are received or a timeout occurs.
    for (uint8_t message_count = 0; message_count < MAX_TWIN_MESSAGE_COUNT; message_count++)
    {
        if (!receive_device_twin_message())
        {
            return;
        }
    }
#endif
    IOT_SAMPLE_LOG_SUCCESS("Client received messages.");
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t subscribe_mqtt_client_to_iot_hub_topics( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_mqtt_subscribe_info_t sub_msg[1];

    sub_msg[0].qos = (cy_mqtt_qos_t)CY_MQTT_QOS1;
    sub_msg[0].topic = AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC;
    sub_msg[0].topic_len = sizeof( AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC ) -1 ;

    result = cy_mqtt_subscribe( mqtthandle, &sub_msg[0], 1 );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\n\rcy_mqtt_subscribe ------------------------ Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\n\rcy_mqtt_subscribe ------------------------ Fail \n" ));
    }
    TEST_INFO(( "\r\nSubscribed to Topic : %.*s\r\n", (int)sub_msg[0].topic_len, sub_msg[0].topic ));

    sub_msg[0].qos = (cy_mqtt_qos_t)CY_MQTT_QOS1;
    sub_msg[0].topic = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC;
    sub_msg[0].topic_len = sizeof( AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC ) -1 ;

    result = cy_mqtt_subscribe( mqtthandle, &sub_msg[0], 1 );
    if( result == TEST_PASS )
    {
        TEST_INFO(( "\n\rcy_mqtt_subscribe ------------------------ Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\n\rcy_mqtt_subscribe ------------------------ Fail \n" ));
    }
    TEST_INFO(( "\r\nSubscribed to Topic : %.*s\r\n", (int)sub_msg[0].topic_len, sub_msg[0].topic ));

    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t connect_mqtt_client_to_iot_hub(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    int rc;
    size_t username_len = 0, client_id_len = 0;
    cy_mqtt_connect_info_t connect_info;

    // Get the MQTT client ID used for the MQTT connection
    char mqtt_client_id_buffer[128];

    memset( &connect_info, 0x00, sizeof( cy_mqtt_connect_info_t ) );
    memset( &mqtt_client_id_buffer, 0x00, sizeof( mqtt_client_id_buffer ) );

    rc = az_iot_hub_client_get_client_id( &hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), &client_id_len );
    if( az_result_failed(rc) )
    {
        TEST_INFO(( "\r\nFailed to get MQTT client id: az_result return code 0x%08x.", rc ));
        return TEST_FAIL;
    }

    // Get the MQTT client user name
    rc = az_iot_hub_client_get_user_name( &hub_client, mqtt_client_username_buffer,
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
    connect_info.password = IOT_AZURE_PASSWORD;
    connect_info.username_len = username_len;
    connect_info.password_len = IOT_AZURE_PASSWORD_LENGTH;
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
    int32_t rc;
    uint16_t ep_size = 0;
    cy_rslt_t result = TEST_PASS;
    cy_awsport_ssl_credentials_t credentials;
    cy_awsport_ssl_credentials_t *security = NULL;
    cy_mqtt_broker_info_t broker_info;

    memset( &mqtt_endpoint_buffer, 0x00, sizeof( mqtt_endpoint_buffer ) );

    ep_size = iot_sample_create_mqtt_endpoint( CY_MQTT_IOT_HUB, &env_vars,
                                                 mqtt_endpoint_buffer,
                                                 sizeof(mqtt_endpoint_buffer) );

    // Initialize the hub client with default connection options
    rc = az_iot_hub_client_init( &hub_client, env_vars.hub_hostname, env_vars.hub_device_id, NULL );
    if( az_result_failed( rc ) )
    {
        TEST_INFO(( "\r\nFailed to initialize hub client: az_result return code 0x%08x.", (unsigned int)rc ));
        return TEST_FAIL;
    }

    /*Allocate the network buffer */
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
        TEST_INFO(( "\n\rcy_mqtt_init ----------------------------- Pass \n" ));
    }
    else
    {
        TEST_INFO(( "\n\rcy_mqtt_init ----------------------------- Fail \n" ));
        return TEST_FAIL;
    }

#if SAS_TOKEN_AUTH
            credentials.client_cert = (const char *)NULL;
            credentials.client_cert_size = 0;
            credentials.private_key = (const char *)NULL;
            credentials.private_key_size = 0;
#else
            credentials.client_cert = (const char *) &azure_client_cert;
            credentials.client_cert_size = sizeof( azure_client_cert );
            credentials.private_key = (const char *) &azure_client_key;
            credentials.private_key_size = sizeof( azure_client_key );
#endif
            credentials.root_ca = (const char *) &azure_root_ca_certificate;
            credentials.root_ca_size = sizeof( azure_root_ca_certificate );
            broker_info.hostname = (const char *)&mqtt_endpoint_buffer;
            broker_info.hostname_len = ep_size;
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

static cy_rslt_t configure_hub_environment_variables( void )
{
    cy_rslt_t result = TEST_PASS;
#if SAS_TOKEN_AUTH
    env_vars.hub_device_id._internal.ptr = (uint8_t*)MQTT_CLIENT_IDENTIFIER_AZURE_SAS;
    env_vars.hub_device_id._internal.size = MQTT_CLIENT_IDENTIFIER_AZURE_SAS_LENGTH;
#else
    env_vars.hub_device_id._internal.ptr = (uint8_t*)MQTT_CLIENT_IDENTIFIER_AZURE_CERT;
    env_vars.hub_device_id._internal.size = MQTT_CLIENT_IDENTIFIER_AZURE_CERT_LENGTH;
#endif
    env_vars.hub_hostname._internal.ptr = (uint8_t*)IOT_DEMO_SERVER_AZURE;
    env_vars.hub_hostname._internal.size = strlen(IOT_DEMO_SERVER_AZURE);
    env_vars.hub_sas_key._internal.ptr = (uint8_t*)IOT_AZURE_PASSWORD;
    env_vars.hub_sas_key._internal.size = IOT_AZURE_PASSWORD_LENGTH;

    env_vars.sas_key_duration_minutes = 240;
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/

static void Azure_hub_twin_app(void *arg)
{
    cy_rslt_t TestRes = TEST_PASS ;
    uint8_t Failcount = 0, Passcount = 0;
    uint32_t time_ms = 0;

    cy_log_init( CY_LOG_ERR, NULL, NULL );

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

    /*
     * Initialize the queue for hub methods events
     */
    TestRes = cy_rtos_init_semaphore( &twin_app_sem, 1, 0 );
    if( TestRes == CY_RSLT_SUCCESS )
    {
        TEST_INFO(( "\r\n cy_rtos_init_semaphore ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n cy_rtos_init_semaphore ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

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
    TestRes = connect_mqtt_client_to_iot_hub();
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
    TestRes = subscribe_mqtt_client_to_iot_hub_topics();
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

    /*--------------------------------------------------------------------*/
    TestRes = send_and_receive_device_twin_messages();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\n send_and_receive_device_twin_messages ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n send_and_receive_device_twin_messages ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    /*--------------------------------------------------------------------*/
    /* 2 Min check to make the app to work with CI/CD */
    time_ms = ( 120 * 1000 );
    while( connect_state )
    {
        TEST_INFO(( "\r\n Waiting for hub to client messages...... \n" ));

        TestRes = cy_rtos_get_semaphore( &twin_app_sem, 500, false );
        if( TestRes != CY_RSLT_SUCCESS )
        {
            TEST_INFO(( "\r\n cy_rtos_get_semaphore returned with status [0x%X] ", (unsigned int)TestRes ));
        }
        else
        {
            send_reported_property();
            TEST_INFO(( " " ));
            TEST_INFO(( "\n\rClient received messages." ));
        }
        time_ms = time_ms - 500;
    }
    /*--------------------------------------------------------------------*/
    TestRes = disconnect_and_delete_mqtt_client();
    if( TestRes == TEST_PASS )
    {
        TEST_INFO(( "\r\n disconnect_and_delete_mqtt_client ----------- Pass \n" ));
        Passcount++;
    }
    else
    {
        TEST_INFO(( "\r\n disconnect_and_delete_mqtt_client ----------- Fail \n" ));
        Failcount++;
        goto exit;
    }

    /*************************************************************************************************/
exit:
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

    /* Initialize the board support package */
    result = cybsp_init() ;
    CY_ASSERT( result == CY_RSLT_SUCCESS );
    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port. */
    // This is not required because target_io_init is done as part of BeginTesting.
    cy_retarget_io_init( CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE );

    xTaskCreate( Azure_hub_twin_app, "Azure_hub_twin_app", 1024 * 10, NULL, 1, NULL ) ;

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler() ;

    return 0;
}

/*-----------------------------------------------------------*/