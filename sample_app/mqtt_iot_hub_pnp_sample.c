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
 * @file mqtt_iot_hub_pnp_sample.c
 * Implementation of Azure plug and play sample application on Cypress platforms
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
#include "cy_nw_helper.h"

#include <FreeRTOS.h>
#include <task.h>

#include "cyabs_rtos.h"

#include "mqtt_main.h"
#include "cy_mqtt_api.h"
#include <az_core.h>
#include <az_iot.h>
#include "mqtt_iot_sample_common.h"

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

#define DEFAULT_START_TEMP_COUNT              ( 1 )
#define DEFAULT_START_TEMP_CELSIUS            ( 22.0 )
#define DOUBLE_DECIMAL_PLACE_DIGITS           ( 2 )

/************************************************************
 *                         Constants                        *
 ************************************************************/
static whd_interface_t                     iface ;
static cy_mqtt_t                           mqtthandle;
static iot_sample_environment_variables    env_vars;
static az_iot_hub_client                   hub_client;
static char                                mqtt_client_username_buffer[IOT_SAMPLE_APP_BUFFER_SIZE_IN_BYTES];
static char                                mqtt_endpoint_buffer[IOT_SAMPLE_APP_BUFFER_SIZE_IN_BYTES];
static volatile bool                       connect_state = false;
static uint32_t                            connection_request_id_int = 0;
static char                                connection_request_id_buffer[16];

#if SAS_TOKEN_AUTH
static iot_sample_credentials              sas_credentials;
static char                                device_id_buffer[IOT_SAMPLE_APP_BUFFER_SIZE_IN_BYTES];
static char                                sas_token_buffer[IOT_SAMPLE_APP_BUFFER_SIZE_IN_BYTES];
#endif

#ifdef CY_TFM_PSA_SUPPORTED
static struct                              ns_mailbox_queue_t ns_mailbox_queue;
#endif

/* IoT Hub Device Twin Values */
static az_span const twin_desired_name = AZ_SPAN_LITERAL_FROM_STR("desired");
static az_span const twin_version_name = AZ_SPAN_LITERAL_FROM_STR("$version");
static az_span const twin_success_name = AZ_SPAN_LITERAL_FROM_STR("success");
static az_span const twin_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static az_span const twin_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static az_span const twin_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static az_span const twin_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static az_span const twin_desired_temperature_property_name = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static az_span const twin_reported_maximum_temperature_property_name = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");

/* IoT Hub Method (Command) Values */
static az_span const command_getMaxMinReport_name = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static az_span const command_max_temp_name = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static az_span const command_min_temp_name = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static az_span const command_avg_temp_name = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static az_span const command_start_time_name = AZ_SPAN_LITERAL_FROM_STR("startTime");
static az_span const command_end_time_name = AZ_SPAN_LITERAL_FROM_STR("endTime");
static az_span const command_empty_response_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char command_start_time_value_buffer[64];
static char command_end_time_value_buffer[64];
static char command_response_payload_buffer[256];

/* IoT Hub Telemetry Values */
static char const iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z"; /* ISO8601 Time Format */

/* PnP Device Values */
static double device_current_temperature = DEFAULT_START_TEMP_CELSIUS;
static double device_maximum_temperature = DEFAULT_START_TEMP_CELSIUS;
static double device_minimum_temperature = DEFAULT_START_TEMP_CELSIUS;
static double device_temperature_summation = DEFAULT_START_TEMP_CELSIUS;
static uint32_t device_temperature_count = DEFAULT_START_TEMP_COUNT;
static double device_average_temperature = DEFAULT_START_TEMP_CELSIUS;

static cy_queue_t              pnp_msg_event_queue = NULL;

typedef enum pnp_msg_type
{
    device_twin_message = 0,
    device_command_request = 1
}pnp_msg_type_t;

typedef struct pnp_msg_event
{
    pnp_msg_type_t  msg_type;
    az_iot_status status;
    bool is_twin_get;
    az_span command_response_payload;
    az_span message_span;
    az_iot_hub_client_method_request *method_req;
}pnp_msg_event_t;

/**
 * @brief The network buffer must remain valid for the lifetime of the MQTT context.
 */
static uint8_t                             *buffer = NULL;

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
                TEST_ERROR(("\n\rExceeded max WiFi connection attempts\n"));
                return TEST_FAIL;
            }
            TEST_INFO(("\n\rConnection to WiFi network failed. Retrying...\n"));
            continue;
        }
        else
        {
            TEST_INFO(("\n\rSuccessfully joined WiFi network %s \n", ssid));
            break;
        }
    }

    nw_interface.role = CY_LWIP_STA_NW_INTERFACE;
    nw_interface.whd_iface = iface;

    /* Add interface to lwIP */
    cy_lwip_add_interface(&nw_interface, NULL) ;

    /* Bring up the network */
    cy_lwip_network_up(&nw_interface);

    struct netif *net = cy_lwip_get_interface(CY_LWIP_STA_NW_INTERFACE);

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
static az_span get_request_id(void)
{
    az_span remainder;
    az_span out_span = az_span_create(
        (uint8_t*)connection_request_id_buffer, sizeof(connection_request_id_buffer));

    az_result rc = az_span_u32toa(out_span, connection_request_id_int++, &remainder);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to get request id: az_result return code 0x%08x.", (unsigned int)rc);
        return (remainder);
    }
    return az_span_slice(out_span, 0, az_span_size(out_span) - az_span_size(remainder));
}

/*--------------------------------------------------------------------------------------------------------*/
static void send_command_response(
    az_iot_hub_client_method_request const* command_request,
    az_iot_status status,
    az_span response)
{
    az_result rc;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_mqtt_publish_info_t pub_msg;
    size_t topic_len = 0;

    /* Get the Methods response topic to publish the command response. */
    char methods_response_topic_buffer[128];
    rc = az_iot_hub_client_methods_response_get_publish_topic(
      &hub_client,
      command_request->request_id,
      (uint16_t)status,
      methods_response_topic_buffer,
      sizeof(methods_response_topic_buffer),
      &topic_len);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR(
        "Failed to get the Methods Response topic: az_result return code 0x%08x.", (unsigned int)rc);
        return;
    }

    /* Publish the command response. */
    memset(&pub_msg, 0x00, sizeof(cy_mqtt_publish_info_t));
    pub_msg.qos = (cy_mqtt_qos_t)CY_MQTT_QOS0;
    pub_msg.topic = (const char *)&methods_response_topic_buffer;
    pub_msg.topic_len = (uint16_t)topic_len;
    pub_msg.payload = (const char *)response._internal.ptr;
    pub_msg.payload_len = (size_t)response._internal.size;

    /* Publish the reported property update.*/
    result = cy_mqtt_publish(mqtthandle, &pub_msg);
    if(result == CY_RSLT_SUCCESS)
    {
        TEST_INFO(("\r\ncy_mqtt_publish completed........\n\r"));
        IOT_SAMPLE_LOG_SUCCESS("Client published the Command response.");
        IOT_SAMPLE_LOG("Status: %d", status);
        IOT_SAMPLE_LOG_AZ_SPAN("Payload:", response);
    }
    else
    {
        TEST_INFO(("\r\ncy_mqtt_publish failed with Error : [0x%X] ", (unsigned int)result));
        return;
    }
}

/*--------------------------------------------------------------------------------------------------------*/
static void build_property_payload(
    uint8_t property_count,
    az_span const names[],
    double const values[],
    az_span const times[],
    az_span property_payload,
    az_span* out_property_payload)
{
    az_json_writer jw;
    az_result rc;

    rc = az_json_writer_init(&jw, property_payload, NULL);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
        return;
    }
    rc = az_json_writer_append_begin_object(&jw);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
        return;
    }
    for (uint8_t i = 0; i < property_count; i++)
    {
        rc = az_json_writer_append_property_name(&jw, names[i]);
        if (az_result_failed(rc))
        {
            IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
            return;
        }
        rc = az_json_writer_append_double(&jw, values[i], DOUBLE_DECIMAL_PLACE_DIGITS);
        if (az_result_failed(rc))
        {
            IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
            return;
        }
    }

    if (times != NULL)
    {
        rc = az_json_writer_append_property_name(&jw, command_start_time_name);
        if (az_result_failed(rc))
        {
            IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
            return;
        }
        rc = az_json_writer_append_string(&jw, times[0]);
        if (az_result_failed(rc))
        {
            IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
            return;
        }
        rc = az_json_writer_append_property_name(&jw, command_end_time_name);
        if (az_result_failed(rc))
        {
            IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
            return;
        }
        rc = az_json_writer_append_string(&jw, times[1]);
        if (az_result_failed(rc))
        {
            IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
            return;
        }
    }

    rc = az_json_writer_append_end_object(&jw);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload");
        return;
    }
    *out_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

/*--------------------------------------------------------------------------------------------------------*/
static bool invoke_getMaxMinReport(az_span payload, az_span response, az_span* out_response)
{
    int32_t incoming_since_value_len = 0;
    az_result rc;

    /* Parse the `since` field in the payload. */
    az_json_reader jr;
    rc = az_json_reader_init(&jr, payload, NULL);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed az_json_reader_init: az_result return code 0x%08x.", (unsigned int)rc);
        return false;
    }
    rc = az_json_reader_next_token(&jr);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed az_json_reader_next_token: az_result return code 0x%08x.", (unsigned int)rc);
        return false;
    }
    rc = az_json_token_get_string(
          &jr.token,
          command_start_time_value_buffer,
          sizeof(command_start_time_value_buffer),
          &incoming_since_value_len);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed az_json_token_get_string: az_result return code 0x%08x.", (unsigned int)rc);
        return false;
    }

    /* Set the response payload to error if the `since` value was empty. */
    if (incoming_since_value_len == 0)
    {
        *out_response = command_empty_response_payload;
        return false;
    }

    az_span start_time_span
      = az_span_create((uint8_t*)command_start_time_value_buffer, incoming_since_value_len);

    IOT_SAMPLE_LOG_AZ_SPAN("Start time:", start_time_span);

    /* Get the current time as a string. */
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    size_t length = strftime(
      command_end_time_value_buffer,
      sizeof(command_end_time_value_buffer),
      iso_spec_time_format,
      timeinfo);
    az_span end_time_span = az_span_create((uint8_t*)command_end_time_value_buffer, (int32_t)length);

    IOT_SAMPLE_LOG_AZ_SPAN("End Time:", end_time_span);

    /* Build command response message. */
    uint8_t count = 3;
    az_span const names[3] = { command_max_temp_name, command_min_temp_name, command_avg_temp_name };
    double const values[3] = { device_maximum_temperature, device_minimum_temperature, device_average_temperature };
    az_span const times[2] = { start_time_span, end_time_span };

    build_property_payload(count, names, values, times, response, out_response);

    return true;
}

/*--------------------------------------------------------------------------------------------------------*/
static void handle_command_request(cy_mqtt_publish_info_t *message,
                                    az_iot_hub_client_method_request * command_request)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    az_span message_span = az_span_create((uint8_t*)message->payload, message->payload_len);
    pnp_msg_event_t *msg_event = NULL;

    msg_event = (pnp_msg_event_t *) malloc(sizeof(pnp_msg_event_t));
    if(msg_event == NULL)
    {
        IOT_SAMPLE_LOG("Memory not available...");
        return;
    }
    msg_event->msg_type = device_command_request;

    if (az_span_is_content_equal(command_getMaxMinReport_name, command_request->name))
    {
        az_iot_status status;
        az_span command_response_payload = AZ_SPAN_FROM_BUFFER(command_response_payload_buffer);

        /* Invoke command. */
        if (invoke_getMaxMinReport(message_span, command_response_payload, &command_response_payload))
        {
            status = AZ_IOT_STATUS_BAD_REQUEST;
        }
        else
        {
            status = AZ_IOT_STATUS_OK;
        }
        IOT_SAMPLE_LOG_SUCCESS("Client invoked command 'getMaxMinReport'.");
        msg_event->method_req = command_request;
        msg_event->status = status;
        memcpy(&(msg_event->command_response_payload), &command_response_payload, sizeof(az_span));
     }
    else
    {
        IOT_SAMPLE_LOG_AZ_SPAN("Command not supported:", command_request->name);
        msg_event->method_req = command_request;
        msg_event->status = AZ_IOT_STATUS_NOT_FOUND;
        memcpy(&(msg_event->command_response_payload), &command_empty_response_payload, sizeof(az_span));
     }

    TEST_INFO(("\n\r Pushing to PNP message(device_command_request) event queue...\n"));
    result = cy_rtos_put_queue(&pnp_msg_event_queue, (void *)&msg_event, 500, false);
    if(result != CY_RSLT_SUCCESS)
    {
        TEST_INFO(("\n\r Pushing to PNP message event queue failed with Error : [0x%X] ", (unsigned int)result));
        free(msg_event);
        return;
    }
    TEST_INFO(("\n\r Message(device_command_request) queued to PNP message event queue...\n"));
}

/*--------------------------------------------------------------------------------------------------------*/
static void build_property_payload_with_status(
    az_span name,
    double value,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value,
    az_span property_payload,
    az_span* out_property_payload)
{
    az_json_writer jw;
    az_result rc;

    rc = az_json_writer_init(&jw, property_payload, NULL);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }

    rc = az_json_writer_append_begin_object(&jw);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_property_name(&jw, name);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_begin_object(&jw);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_property_name(&jw, twin_value_name);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc =  az_json_writer_append_double(&jw, value, DOUBLE_DECIMAL_PLACE_DIGITS);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_property_name(&jw, twin_ack_code_name);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_int32(&jw, ack_code_value);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_property_name(&jw, twin_ack_version_name);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_int32(&jw, ack_version_value);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_property_name(&jw, twin_ack_description_name);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_string(&jw, ack_description_value);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_end_object(&jw);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }
    rc = az_json_writer_append_end_object(&jw);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to build property payload with status");
        return;
    }

    *out_property_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

/*--------------------------------------------------------------------------------------------------------*/
static void send_reported_property(az_span name, double value, int32_t version, bool confirm)
{
    az_result rc;
    size_t topic_len = 0;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_mqtt_publish_info_t pub_msg;

    /* Get the Twin Patch topic to send a reported property update. */
    char twin_patch_topic_buffer[128];
    rc = az_iot_hub_client_twin_patch_get_publish_topic(
         &hub_client,
         get_request_id(),
         twin_patch_topic_buffer,
         sizeof(twin_patch_topic_buffer),
         &topic_len);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to get the Twin Patch topic: az_result return code 0x%08x.", (unsigned int)rc);
        return;
    }

    /* Build the updated reported property message. */
    char reported_property_payload_buffer[128];
    az_span reported_property_payload = AZ_SPAN_FROM_BUFFER(reported_property_payload_buffer);

    if (confirm)
    {
        build_property_payload_with_status(
        name,
        value,
        AZ_IOT_STATUS_OK,
        version,
        twin_success_name,
        reported_property_payload,
        &reported_property_payload);
    }
    else
    {
        uint8_t count = 1;
        az_span const names[1] = { name };
        double const values[1] = { value };

        build_property_payload(count, names, values, NULL, reported_property_payload, &reported_property_payload);
    }

    /* Publish the reported property update. */
    memset(&pub_msg, 0x00, sizeof(cy_mqtt_publish_info_t));

    pub_msg.qos = (cy_mqtt_qos_t)CY_MQTT_QOS0;
    pub_msg.topic = (const char *)&twin_patch_topic_buffer;
    pub_msg.topic_len = (uint16_t)topic_len;
    pub_msg.payload = (const char *)reported_property_payload._internal.ptr;
    pub_msg.payload_len = (size_t)reported_property_payload._internal.size;

    /* Publish the reported property update. */
    result = cy_mqtt_publish(mqtthandle, &pub_msg);
    if(result == TEST_PASS)
    {
        TEST_INFO(("\r\ncy_mqtt_publish completed........\n\r"));
        IOT_SAMPLE_LOG_SUCCESS("Client published the Twin Patch reported property message.");
        IOT_SAMPLE_LOG_AZ_SPAN("Payload:", reported_property_payload);
    }
    else
    {
        TEST_INFO(("\r\ncy_mqtt_publish failed with Error : [0x%X] ", (unsigned int)result));
        return;
    }
}

/*--------------------------------------------------------------------------------------------------------*/
static void update_device_temperature_property(double temperature, bool* out_is_max_temp_changed)
{
    if (device_maximum_temperature < device_minimum_temperature)
    {
        IOT_SAMPLE_LOG("device_maximum_temperature is less then device_minimum_temperature");
        return;
    }

    *out_is_max_temp_changed = false;
    device_current_temperature = temperature;

    /* Update maximum or minimum temperatures. */
    if (device_current_temperature > device_maximum_temperature)
    {
        device_maximum_temperature = device_current_temperature;
        *out_is_max_temp_changed = true;
    }
    else if (device_current_temperature < device_minimum_temperature)
    {
        device_minimum_temperature = device_current_temperature;
    }

    /* Calculate the new average temperature. */
    device_temperature_count++;
    device_temperature_summation += device_current_temperature;
    device_average_temperature = device_temperature_summation / device_temperature_count;

    IOT_SAMPLE_LOG_SUCCESS("Client updated desired temperature variables locally.");
    IOT_SAMPLE_LOG("Current Temperature: %2f", device_current_temperature);
    IOT_SAMPLE_LOG("Maximum Temperature: %2f", device_maximum_temperature);
    IOT_SAMPLE_LOG("Minimum Temperature: %2f", device_minimum_temperature);
    IOT_SAMPLE_LOG("Average Temperature: %2f", device_average_temperature);
}

/*--------------------------------------------------------------------------------------------------------*/
static bool parse_desired_temperature_property(
    az_span message_span,
    bool is_twin_get,
    double* out_parsed_temperature,
    int32_t* out_parsed_version_number)
{
    az_span property = twin_desired_temperature_property_name;

    *out_parsed_temperature = 0.0;
    *out_parsed_version_number = 0;

    /* Parse message_span. */
    az_json_reader jr;
    az_result rc;

    rc = az_json_reader_init(&jr, message_span, NULL);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
        return false;
    }
    rc = az_json_reader_next_token(&jr);
    if (az_result_failed(rc))
    {
        IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
        return false;
    }
    if (jr.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
        IOT_SAMPLE_LOG(
        "`%.*s` property object not found in device twin GET response.",
        (int)az_span_size(twin_desired_name),
        az_span_ptr(twin_desired_name));
        return false;
    }

    /* Device twin GET response: Parse to the "desired" wrapper if it exists. */
    bool desired_found = false;
    if (is_twin_get)
    {
        rc = az_json_reader_next_token(&jr);
        if (az_result_failed(rc))
        {
            IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
            return false;
        }
        while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
        {
            if (az_json_token_is_text_equal(&jr.token, twin_desired_name))
            {
                rc = az_json_reader_next_token(&jr);
                if (az_result_failed(rc))
                {
                    IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
                    return false;
                }
                desired_found = true;
                break;
            }
            else
            {
                rc = az_json_reader_skip_children(&jr);
                if (az_result_failed(rc))
                {
                    IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
                    return false;
                }
            }
            rc = az_json_reader_next_token(&jr);
            if (az_result_failed(rc))
            {
                IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
                return false;
            }
        }

        if (!desired_found)
        {
            IOT_SAMPLE_LOG(
            "`%.*s` property object not found in device twin GET response.",
            (int)az_span_size(twin_desired_name),
            az_span_ptr(twin_desired_name));
            return false;
        }
    }

    /* Device twin get response OR desired property response:
       Parse for the desired temperature property */
    bool temp_found = false;
    bool version_found = false;

    rc = az_json_reader_next_token(&jr);
    while (!(temp_found && version_found) && (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT))
    {
        if (az_json_token_is_text_equal(&jr.token, twin_desired_temperature_property_name))
        {
            rc = az_json_reader_next_token(&jr);
            if (az_result_failed(rc))
            {
                IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
                return false;
            }
            rc = az_json_token_get_double(&jr.token, out_parsed_temperature);
            if (az_result_failed(rc))
            {
                IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
                return false;
            }
            temp_found = true;
        }
        else if (az_json_token_is_text_equal(&jr.token, twin_version_name))
        {
            rc = az_json_reader_next_token(&jr);
            if (az_result_failed(rc))
            {
                IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
                return false;
            }
            rc = az_json_token_get_int32(&jr.token, out_parsed_version_number);
            if (az_result_failed(rc))
            {
                IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
                return false;
            }
            version_found = true;
        }
        else
        {
            rc = az_json_reader_skip_children(&jr);
            if (az_result_failed(rc))
            {
                IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
                return false;
            }
        }
        rc = az_json_reader_next_token(&jr);
        if (az_result_failed(rc))
        {
            IOT_SAMPLE_LOG_ERROR("Failed to parse_desired_temperature_property");
            return false;
        }
    }

    if (temp_found && version_found)
    {
        IOT_SAMPLE_LOG(
        "Parsed desired `%.*s`: %2f",
        (int)az_span_size(property),
        az_span_ptr(property),
        *out_parsed_temperature);
        IOT_SAMPLE_LOG(
        "Parsed `%.*s` number: %d",
        (int)az_span_size(twin_version_name),
        az_span_ptr(twin_version_name),
        (int)*out_parsed_version_number);
    }
    else
    {
        IOT_SAMPLE_LOG(
        "Either `%.*s` or `%.*s` were not found in desired property response.",
        (int)az_span_size(property),
        az_span_ptr(property),
        (int)az_span_size(twin_version_name),
        az_span_ptr(twin_version_name));
        return false;
    }

    return true;
}

/*--------------------------------------------------------------------------------------------------------*/
static void process_device_twin_message(az_span message_span, bool is_twin_get)
{
    double desired_temperature;
    int32_t version_number;

    /* Parse for the desired temperature property. */
    if (parse_desired_temperature_property(
          message_span, is_twin_get, &desired_temperature, &version_number))
    {
        IOT_SAMPLE_LOG(" "); /* Formatting */
        bool confirm = true;
        bool is_max_temp_changed = false;
        /* Update device temperature locally and report update to server. */
        update_device_temperature_property(desired_temperature, &is_max_temp_changed);
        send_reported_property(twin_desired_temperature_property_name, desired_temperature, version_number, confirm);
        if(is_max_temp_changed)
        {
            confirm = false;
            send_reported_property(twin_reported_maximum_temperature_property_name, device_maximum_temperature, -1, confirm);
        }
    }
}

/*--------------------------------------------------------------------------------------------------------*/
static void handle_device_twin_message(cy_mqtt_publish_info_t *message,
                                        az_iot_hub_client_twin_response *twin_response)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    az_span message_span = az_span_create((uint8_t*)message->payload, message->payload_len);
    pnp_msg_event_t *msg_event = NULL;

    msg_event = (pnp_msg_event_t *) malloc(sizeof(pnp_msg_event_t));
    if(msg_event == NULL)
    {
        IOT_SAMPLE_LOG("Memory not available...");
        return;
    }
    msg_event->msg_type = device_twin_message;

    /* Invoke appropriate action per response type (3 types only). */
    switch(twin_response->response_type)
    {
      case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET:
           IOT_SAMPLE_LOG("Message Type: GET");
           memcpy(&(msg_event->message_span), &message_span, sizeof(az_span));
           msg_event->is_twin_get = true;
           break;

      case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
           IOT_SAMPLE_LOG("Message Type: Reported Properties");
           free(msg_event);
           msg_event = NULL;
           break;

      case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
           IOT_SAMPLE_LOG("Message Type: Desired Properties");
           memcpy(&(msg_event->message_span), &message_span, sizeof(az_span));
           msg_event->is_twin_get = false;
           break;
    }

    if(msg_event != NULL)
    {
        TEST_INFO(("\n\r Pushing to PNP message(device_twin_message) event queue...\n"));
        result = cy_rtos_put_queue(&pnp_msg_event_queue, (void *)&msg_event, 500, false);
        if(result != CY_RSLT_SUCCESS)
        {
            TEST_INFO(("\n\r Pushing to PNP message event queue failed with Error : [0x%X] ", (unsigned int)result));
            free(msg_event);
            return;
        }
        TEST_INFO(("\n\r Message(device_twin_message) queued to PNP message event queue...\n"));
    }
}

/*--------------------------------------------------------------------------------------------------------*/
static void on_message_received(char* topic, uint16_t topic_len, cy_mqtt_publish_info_t *message)
{
    az_result rc;

    az_span const topic_span = az_span_create((uint8_t*)topic, topic_len);
    az_span const message_span = az_span_create((uint8_t*)message->payload, message->payload_len);

    az_iot_hub_client_twin_response twin_response;
    az_iot_hub_client_method_request command_request;

    /* Parse the incoming message topic and handle appropriately. */
    rc = az_iot_hub_client_twin_parse_received_topic(&hub_client, topic_span, &twin_response);
    if (az_result_succeeded(rc))
    {
        IOT_SAMPLE_LOG_SUCCESS("Client received a valid topic response.");
        IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
        IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);
        IOT_SAMPLE_LOG("Status: %d", twin_response.status);
        handle_device_twin_message(message, &twin_response);
    }
    else
    {
        rc = az_iot_hub_client_methods_parse_received_topic(&hub_client, topic_span, &command_request);
        if (az_result_succeeded(rc))
        {
            IOT_SAMPLE_LOG_SUCCESS("Client received a valid topic response.");
            IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
            IOT_SAMPLE_LOG_AZ_SPAN("Payload:", message_span);
            handle_command_request(message, &command_request);
        }
        else
        {
            IOT_SAMPLE_LOG_ERROR("Message from unknown topic: az_result return code 0x%08x.", (unsigned int)rc);
            IOT_SAMPLE_LOG_AZ_SPAN("Topic:", topic_span);
        }
    }
}

/*--------------------------------------------------------------------------------------------------------*/

void mqtt_event_cb(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *arg)
{
    cy_mqtt_publish_info_t *received_msg;

    TEST_INFO(("\r\nMQTT App callback with handle : %p \n", mqtt_handle));

    switch(event.type)
    {
        case CY_MQTT_EVENT_TYPE_DISCONNECT :
            if(event.data.reason == CY_MQTT_DISCONN_TYPE_BROKER_DOWN)
            {
                TEST_INFO(("\r\nCY_MQTT_DISCONN_TYPE_BROKER_DOWN .....\n"));
            }
            else
            {
                TEST_INFO(("\r\nCY_MQTT_DISCONN_REASON_NETWORK_DISCONNECTION .....\n"));
            }
            connect_state = false;
            break;

        case CY_MQTT_EVENT_TYPE_PUBLISH_RECEIVE :
            received_msg = &(event.data.pub_msg.received_message);
            TEST_INFO(("\n\rMessage received from broker...\n"));
            on_message_received((char*)received_msg->topic, received_msg->topic_len, received_msg);
            break;

        default :
            TEST_INFO(("\r\nUNKNOWN EVENT .....\n"));
            break;
    }
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t disconnect_and_delete_mqtt_client(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    result = cy_mqtt_disconnect(mqtthandle);
    if(result == CY_RSLT_SUCCESS)
    {
        TEST_INFO(("\n\rcy_mqtt_disconnect ----------------------- Pass \n"));
    }
    else
    {
        TEST_INFO(("\n\rcy_mqtt_disconnect ----------------------- Fail \n"));
    }
    connect_state = false;
    result = cy_mqtt_delete(mqtthandle);
    if(result == TEST_PASS)
    {
        TEST_INFO(("\n\rcy_mqtt_delete --------------------------- Pass \n"));
    }
    else
    {
        TEST_INFO(("\n\rcy_mqtt_delete --------------------------- Fail \n"));
    }

    result = cy_mqtt_deinit();
    if(result == TEST_PASS)
    {
        TEST_INFO(("\n\rcy_mqtt_deinit --------------------------- Pass \n"));
    }
    else
    {
        TEST_INFO(("\n\rcy_mqtt_deinit --------------------------- Fail \n"));
    }

    /* Free the network buffer */
    if(buffer != NULL)
    {
        free(buffer);
        buffer = NULL;
    }

    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t request_device_twin_document(void)
{
    int rc;
    uint16_t topic_len = 0;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_mqtt_publish_info_t pub_msg;
    char twin_document_topic_buffer[128];

    memset(&pub_msg, 0x00, sizeof(cy_mqtt_publish_info_t));
    memset(&twin_document_topic_buffer, 0x00, sizeof(twin_document_topic_buffer));
    IOT_SAMPLE_LOG("Client requesting device twin document from service.");

    rc = az_iot_hub_client_twin_document_get_publish_topic(
        &hub_client,
        get_request_id(),
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

    /* Publish the twin document request. */
    result = cy_mqtt_publish(mqtthandle, &pub_msg);
    if(result == TEST_PASS)
    {
        TEST_INFO(("\r\ncy_mqtt_publish completed........\n\r"));
    }
    else
    {
        TEST_INFO(("\r\ncy_mqtt_publish failed with Error : [0x%X] ", (unsigned int)result));
        return result;
    }
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t subscribe_mqtt_client_to_iot_hub_topics(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_mqtt_subscribe_info_t sub_msg[1];

    /* Messages received on the Methods topic will be commands to be invoked. */
    sub_msg[0].qos = (cy_mqtt_qos_t)CY_MQTT_QOS1;
    sub_msg[0].topic = AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC;
    sub_msg[0].topic_len = ((uint16_t) (sizeof(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC) - 1));
    result = cy_mqtt_subscribe(mqtthandle, &sub_msg[0], 1);
    if(result == TEST_PASS)
    {
        TEST_INFO(("\ncy_mqtt_subscribe ------------------------ Pass \n"));
    }
    else
    {
        TEST_INFO(("\ncy_mqtt_subscribe ------------------------ Fail \n"));
    }

    /* Messages received on the Twin Patch topic will be updates to the desired properties. */
    sub_msg[0].qos = (cy_mqtt_qos_t)CY_MQTT_QOS1;
    sub_msg[0].topic = AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC;
    sub_msg[0].topic_len = ((uint16_t) (sizeof(AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC) - 1));
    result = cy_mqtt_subscribe(mqtthandle, &sub_msg[0], 1);
    if(result == TEST_PASS)
    {
        TEST_INFO(("\ncy_mqtt_subscribe ------------------------ Pass \n"));
    }
    else
    {
        TEST_INFO(("\ncy_mqtt_subscribe ------------------------ Fail \n"));
    }

    /* Messages received on Twin Response topic will be response statuses from the server. */
    sub_msg[0].qos = (cy_mqtt_qos_t)CY_MQTT_QOS1;
    sub_msg[0].topic = AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC;
    sub_msg[0].topic_len = ((uint16_t) (sizeof(AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC) - 1));
    result = cy_mqtt_subscribe(mqtthandle, &sub_msg[0], 1);
    if(result == TEST_PASS)
    {
        TEST_INFO(("\ncy_mqtt_subscribe ------------------------ Pass \n"));
    }
    else
    {
        TEST_INFO(("\ncy_mqtt_subscribe ------------------------ Fail \n"));
    }
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/
static cy_rslt_t connect_mqtt_client_to_iot_hub(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    int rc;
    size_t username_len = 0, client_id_len = 0;
    cy_mqtt_connect_info_t connect_info;

    /* Get the MQTT client ID used for the MQTT connection */
    char mqtt_client_id_buffer[128];

    memset(&connect_info, 0x00, sizeof(cy_mqtt_connect_info_t));
    memset(&mqtt_client_id_buffer, 0x00, sizeof(mqtt_client_id_buffer));

    rc = az_iot_hub_client_get_client_id(&hub_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), &client_id_len);
    if(az_result_failed(rc))
    {
        TEST_INFO(("\r\nFailed to get MQTT client id: az_result return code 0x%08x.", rc));
        return TEST_FAIL;
    }

    /* Get the MQTT client user name */
    rc = az_iot_hub_client_get_user_name(&hub_client, mqtt_client_username_buffer,
                                          sizeof(mqtt_client_username_buffer), &username_len);
    if(az_result_failed(rc))
    {
        TEST_INFO(("\r\nFailed to get MQTT client username: az_result return code 0x%08x.", rc));
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

    result = cy_mqtt_connect(mqtthandle, &connect_info);
    if(result == TEST_PASS)
    {
        TEST_INFO(("\r\ncy_mqtt_connect -------------------------- Pass \n"));
    }
    else
    {
        TEST_INFO(("\r\ncy_mqtt_connect -------------------------- Fail \n"));
        return TEST_FAIL;
    }
    connect_state = true;
  return CY_RSLT_SUCCESS;
}

/*--------------------------------------------------------------------------------------------------------*/

static cy_rslt_t create_and_configure_mqtt_client(void)
{
    int32_t rc;
    uint16_t ep_size = 0;
    cy_rslt_t result = TEST_PASS;
    cy_awsport_ssl_credentials_t credentials;
    cy_awsport_ssl_credentials_t *security = NULL;
    cy_mqtt_broker_info_t broker_info;

    memset(&mqtt_endpoint_buffer, 0x00, sizeof(mqtt_endpoint_buffer));

    ep_size = iot_sample_create_mqtt_endpoint(CY_MQTT_IOT_HUB, &env_vars,
                                                 mqtt_endpoint_buffer,
                                                 sizeof(mqtt_endpoint_buffer));

    /* Initialize the hub client with the default connection options */
    rc = az_iot_hub_client_init(&hub_client, env_vars.hub_hostname, env_vars.hub_device_id, NULL);
    if(az_result_failed(rc))
    {
        TEST_INFO(("\r\nFailed to initialize hub client: az_result return code 0x%08x.", (unsigned int)rc));
        return TEST_FAIL;
    }

    /* Allocate the network buffer */
    buffer = (uint8_t *) malloc(sizeof(uint8_t) * NETWORK_BUFFER_SIZE);
    if(buffer == NULL)
    {
        TEST_INFO(("\r\nNo memory is available for network buffer..! \n"));
        return TEST_FAIL;
    }

    memset(&broker_info, 0x00, sizeof(cy_mqtt_broker_info_t));
    memset(&credentials, 0x00, sizeof(cy_awsport_ssl_credentials_t));

    result = cy_mqtt_init();
    if(result == TEST_PASS)
    {
        TEST_INFO(("\ncy_mqtt_init ----------------------------- Pass \n"));
    }
    else
    {
        TEST_INFO(("\ncy_mqtt_init ----------------------------- Fail \n"));
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
    broker_info.hostname_len = ep_size;
    broker_info.port = IOT_DEMO_PORT_AZURE_S;
    security = &credentials;

    result = cy_mqtt_create(buffer, NETWORK_BUFFER_SIZE,
                             security, &broker_info,
                             (cy_mqtt_callback_t)mqtt_event_cb, NULL,
                             &mqtthandle);
    if(result == TEST_PASS)
    {
        TEST_INFO(("\r\ncy_mqtt_create ----------------------------- Pass \n"));
    }
    else
    {
        TEST_INFO(("\r\ncy_mqtt_create ----------------------------- Fail \n"));
        return TEST_FAIL;
    }
#if 0
    /* Generate the shared access signature (SAS) key here for SAS-based authentication */
#endif
    return CY_RSLT_SUCCESS;
}


static cy_rslt_t configure_hub_environment_variables(void)
{
    cy_rslt_t result = TEST_PASS;
#if SAS_TOKEN_AUTH
    env_vars.hub_device_id._internal.ptr = (uint8_t*)sas_credentials.device_id;
    env_vars.hub_device_id._internal.size = (int32_t)sas_credentials.device_id_len;
    env_vars.hub_sas_key._internal.ptr = (uint8_t*)sas_credentials.sas_token;
    env_vars.hub_sas_key._internal.size = (int32_t)sas_credentials.sas_token_len;
#else
    env_vars.hub_device_id._internal.ptr = (uint8_t*)MQTT_CLIENT_IDENTIFIER_AZURE_CERT;
    env_vars.hub_device_id._internal.size = MQTT_CLIENT_IDENTIFIER_AZURE_CERT_LENGTH;
    env_vars.hub_sas_key._internal.ptr = NULL;
    env_vars.hub_sas_key._internal.size = 0;
#endif
    env_vars.hub_hostname._internal.ptr = (uint8_t*)IOT_DEMO_SERVER_AZURE;
    env_vars.hub_hostname._internal.size = strlen(IOT_DEMO_SERVER_AZURE);
    env_vars.sas_key_duration_minutes = 240;
    return result;
}

/*--------------------------------------------------------------------------------------------------------*/

static void Azure_hub_pnp_app(void *arg)
{
    cy_rslt_t TestRes = TEST_PASS ;
    uint8_t Failcount = 0, Passcount = 0;
    uint32_t time_ms = 0;
    cy_log_init(CY_LOG_ERR, NULL, NULL);
    pnp_msg_event_t *msg_event = NULL;
#ifdef CY_TFM_PSA_SUPPORTED
    psa_status_t uxStatus = PSA_SUCCESS;
    size_t read_len = 0;
    (void)uxStatus;
    (void)read_len;
#endif

    cy_log_init( CY_LOG_ERR, NULL, NULL );

    TestRes = ConnectWifi();
    if(TestRes == TEST_PASS)
    {
        TEST_INFO(("\r\nConnectWifi ----------- Pass \n"));
        Passcount++;
    }
    else
    {
        TEST_INFO(("\r\nConnectWifi ----------- Fail \n"));
        Failcount++;
    }
    /*--------------------------------------------------------------------*/
    /*
     * Initialize the queue for hub methods events.
     */
    TestRes = cy_rtos_init_queue(&pnp_msg_event_queue, 10, sizeof(az_iot_provisioning_client_register_response *));
    if(TestRes == CY_RSLT_SUCCESS)
    {
        TEST_INFO(("\r\n cy_rtos_init_queue ----------- Pass \n"));
        Passcount++;
    }
    else
    {
        TEST_INFO(("\r\n cy_rtos_init_queue ----------- Fail \n"));
        Failcount++;
        goto exit;
    }

    /*--------------------------------------------------------------------*/
#if SAS_TOKEN_AUTH
    (void)device_id_buffer;
    (void)sas_token_buffer;
#if ( (defined CY_TFM_PSA_SUPPORTED) && ( SAS_TOKEN_LOCATION_FLASH == false ) )
    /* Read the Device ID from the secured memory */
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

    /* Read the SAS token from the secured memory */
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
    if(TestRes == TEST_PASS)
    {
        TEST_INFO(("\r\n configure_environment_variables ----------- Pass \n"));
        Passcount++;
    }
    else
    {
        TEST_INFO(("\r\n configure_environment_variables ----------- Fail \n"));
        Failcount++;
    }

    /*--------------------------------------------------------------------*/
    TestRes = create_and_configure_mqtt_client();
    if(TestRes == TEST_PASS)
    {
        TEST_INFO(("\r\n create_and_configure_mqtt_client ----------- Pass \n"));
        Passcount++;
    }
    else
    {
        TEST_INFO(("\r\n create_and_configure_mqtt_client ----------- Fail \n"));
        Failcount++;
    }

    /*--------------------------------------------------------------------*/
    TestRes = connect_mqtt_client_to_iot_hub();
    if(TestRes == TEST_PASS)
    {
        TEST_INFO(("\r\n connect_mqtt_client_to_iot_hub ----------- Pass \n"));
        Passcount++;
    }
    else
    {
        TEST_INFO(("\r\n connect_mqtt_client_to_iot_hub ----------- Fail \n"));
        Failcount++;
    }

    /*--------------------------------------------------------------------*/
    TestRes = subscribe_mqtt_client_to_iot_hub_topics();
    if(TestRes == TEST_PASS)
    {
        TEST_INFO(("\r\n subscribe_mqtt_client_to_iot_hub_topics ----------- Pass \n"));
        Passcount++;
    }
    else
    {
        TEST_INFO(("\r\n subscribe_mqtt_client_to_iot_hub_topics ----------- Fail \n"));
        Failcount++;
    }

    /*--------------------------------------------------------------------*/
    TestRes = request_device_twin_document();
    if(TestRes == TEST_PASS)
    {
        TEST_INFO(("\r\n request_device_twin_document ----------- Pass \n"));
        Passcount++;
    }
    else
    {
        TEST_INFO(("\r\n request_device_twin_document ----------- Fail \n"));
        Failcount++;
    }

    /*--------------------------------------------------------------------*/
    /* 10 Min check to make the app to work with CI/CD */
    time_ms = (600 * 1000);
    while((connect_state) && (time_ms > 0))
    {
        TestRes = cy_rtos_get_queue(&pnp_msg_event_queue, (void *)&msg_event, 500, false);
        if(TestRes != CY_RSLT_SUCCESS)
        {
            /* No need to do anything */
        }
        else
        {
            if(msg_event != NULL)
            {
                if(msg_event->msg_type == device_twin_message)
                {
                    process_device_twin_message(msg_event->message_span, msg_event->is_twin_get);
                }
                else if(msg_event->msg_type == device_command_request)
                {
                    send_command_response(msg_event->method_req, msg_event->status, msg_event->command_response_payload);
                }
                else
                {
                    TEST_INFO(("\r\n Invalid PNP event message type... \n"));
                }

                free(msg_event);
                msg_event = NULL;
            }
        }
        time_ms = time_ms - 500;
    }
    /*--------------------------------------------------------------------*/
    TestRes = disconnect_and_delete_mqtt_client();
    if(TestRes == TEST_PASS)
    {
        TEST_INFO(("\r\n disconnect_and_delete_mqtt_client ----------- Pass \n"));
        Passcount++;
    }
    else
    {
        TEST_INFO(("\r\n disconnect_and_delete_mqtt_client ----------- Fail \n"));
        Failcount++;
    }

exit :
    /*************************************************************************************************/

    TEST_INFO(("\r\nCompleted MQTT Client Test Cases --------------------------\n"));
    TEST_INFO(("\r\nTotal Test Cases   ---------------------- %d\n", (Failcount + Passcount)));
    TEST_INFO(("\r\nTest Cases passed  ---------------------- %d\n", Passcount));
    TEST_INFO(("\r\nTest Cases failed  ---------------------- %d\n", Failcount));

    /*****************************************************************************************************/

    while(1)
    {
        vTaskDelay(5000);
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
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    /* This is not required because target_io_init is done as part of BeginTesting */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

#ifdef CY_TFM_PSA_SUPPORTED
    tfm_ns_multi_core_boot();
    /* Initialize the TFM interface */
    tfm_ns_interface_init();
#endif

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    xTaskCreate(Azure_hub_pnp_app, "Azure_hub_pnp_app", 1024 * 10, NULL, 1, NULL) ;

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler() ;

    return 0;
}

/*-----------------------------------------------------------*/
