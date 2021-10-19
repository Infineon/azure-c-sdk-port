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
#ifndef CY_MQTT_APP_H_
#define CY_MQTT_APP_H_

/* Macro for Wi-Fi connection */
#define WIFI_SSID                             "Replace this string by WiFi SSID" /* Wi-Fi SSID */
#define WIFI_KEY                              "Replace this string by WiFi KEY"  /* Wi-Fi KEY */
#define MAX_WIFI_RETRY_COUNT                  ( 3 )

/* MQTT client identifier for Azure broker */
#define MQTT_CLIENT_IDENTIFIER_AZURE_SAS             "Replace this string by device ID generated from Azure cloud" /* Unique Device identifier */
#define MQTT_CLIENT_IDENTIFIER_AZURE_CERT            "Replace this string by device ID generated from Azure cloud" /* Unique Device identifier */

#ifndef PSA_DEVICEID_UID
/* UID for provisioned device ID */
#define PSA_DEVICEID_UID                      ( 2U )
#endif

#ifndef PSA_SAS_TOKEN_UID
/* UID for provisioned SAS token */
#define PSA_SAS_TOKEN_UID                     ( 3U )
#endif

/**
 * @brief Length of client identifier
 */
#define MQTT_CLIENT_IDENTIFIER_LENGTH                ( ( uint16_t ) ( sizeof( MQTT_CLIENT_IDENTIFIER ) - 1 ) )
#define MQTT_CLIENT_IDENTIFIER_AWS_LENGTH            ( ( uint16_t ) ( sizeof( MQTT_CLIENT_IDENTIFIER_AWS ) - 1 ) )
#define MQTT_CLIENT_IDENTIFIER_ECLIPSE_LENGTH        ( ( uint16_t ) ( sizeof( MQTT_CLIENT_IDENTIFIER_ECLIPSE ) - 1 ) )
#define MQTT_CLIENT_IDENTIFIER_AZURE_SAS_LENGTH      ( ( uint16_t ) ( sizeof( MQTT_CLIENT_IDENTIFIER_AZURE_SAS ) - 1 ) )
#define MQTT_CLIENT_IDENTIFIER_AZURE_CERT_LENGTH     ( ( uint16_t ) ( sizeof( MQTT_CLIENT_IDENTIFIER_AZURE_CERT ) - 1 ) )
/**
 * @brief The topic to subscribe and publish to in the example.
 *
 * The topic name starts with the client identifier to ensure that each demo
 * interacts with a unique topic name.
 */
#define MQTT_TOPIC                               "Test_Topic"
#define MQTT_TOPIC_AZURE_C2D                     "devices/+/messages/devicebound/#"
#define MQTT_TOPIC_AZURE_TELEMETRY_SAS           "devices/CYPSoC6_02/messages/events/" /* Device name value must be changed according to MQTT_CLIENT_IDENTIFIER_AZURE_SAS.*/
#define MQTT_TOPIC_AZURE_TELEMETRY_CERT          "devices/TestSelfSigned/messages/events/" /* Device name value must be changed according to MQTT_CLIENT_IDENTIFIER_AZURE_CERT.*/

/**
 * @brief Length of client MQTT topic
 */
#define MQTT_TOPIC_LENGTH                        ( ( uint16_t ) ( sizeof( MQTT_TOPIC ) - 1 ) )
#define MQTT_TOPIC_AZURE_C2D_LENGTH              ( ( uint16_t ) ( sizeof( MQTT_TOPIC_AZURE_C2D ) - 1 ) )
#define MQTT_TOPIC_AZURE_TELEMETRY_SAS_LENGTH    ( ( uint16_t ) ( sizeof( MQTT_TOPIC_AZURE_TELEMETRY_SAS ) - 1 ) )
#define MQTT_TOPIC_AZURE_TELEMETRY_CERT_LENGTH   ( ( uint16_t ) ( sizeof( MQTT_TOPIC_AZURE_TELEMETRY_CERT ) - 1 ) )

/**
 * @brief MQTT message published in this example
 */
#define MQTT_TEST_MESSAGE                                "Hello World!"

/**
 * @brief Length of the MQTT message published in this example
 */
#define MQTT_TEST_MESSAGE_LENGTH                         ( ( uint16_t ) ( sizeof( MQTT_TEST_MESSAGE ) - 1 ) )

/**
 * @brief MQTT will message published in this example
 */
#define MQTT_TEST_WILL_MESSAGE                           "Will message - World!"

/**
 * @brief Length of the MQTT will message published in this example
 */
#define MQTT_TEST_WILL_MESSAGE_LENGTH                    ( ( uint16_t ) ( sizeof( MQTT_TEST_WILL_MESSAGE ) - 1 ) )

/**
 * @brief Number of PUBLISH messages sent per iteration
 */
#define MQTT_TEST_PUBLISH_COUNT                          ( 5U )

/**
 * @brief Size of the network buffer for MQTT packets
 */
#define NETWORK_BUFFER_SIZE                              ( 1024U )

/**
 * @brief Delay between MQTT publishes in seconds
 */
#define DELAY_BETWEEN_PUBLISHES_SECONDS                  ( 1U )

/**
 * @brief Maximum time interval in seconds which is allowed to elapse
 *  between two Control packets.
 *
 *  It is the responsibility of the client to ensure that the interval between
 *  Control packets being sent does not exceed this keepalive value. If no other Control packets are sent, the client MUST send a
 *  PINGREQ packet.
 */
#define MQTT_KEEP_ALIVE_INTERVAL_SECONDS                  ( 240U )

/**
 * MQTT-supported QoS levels
 */
typedef enum cy_demo_mqtt_qos
{
    CY_MQTT_QOS_0 = 0, /**< Delivery at most once */
    CY_MQTT_QOS_1 = 1, /**< Delivery at least once */
    CY_MQTT_QOS_2 = 2  /**< Delivery exactly once */
} cy_demo_mqtt_qos_t;

typedef enum {
    SECURED_MQTT,                             /**< @brief For secured TCP connection */
    NON_SECURED_MQTT                          /**< @brief For non-secured TCP connection */
} mqtt_security_flag;

typedef enum {
    AWS_MQTT_BROKER,                          /**< @brief For AWS MQTT Broker connection */
	AZURE_MQTT_BROKER,                        /**< @brief For Azure MQTT Broker connection */
    ECLIPSE_MOSQUITTO_MQTT_BROKER,            /**< @brief For Eclipse Mosquitto MQTT Broker connection */
} mqtt_broker;

typedef enum {
    AUTH_MODE_X509_CERT = 1,                  /**< @brief For cert authentication. */
    AUTH_MODE_SAS_TOKEN = 2,                  /**< @brief For SAS token authentication. */
} mqtt_auth_mode;

/**********************************************************************************************/
/* Azure Server endpoints used for the demos */
#define IOT_DEMO_SERVER_AZURE                    "Replace this string by generated IoT Host name from Azure cloud" /* Host name of the created IoT hub */
#define IOT_DEMO_PORT_AZURE_S                    ( 8883 )

/*
 * The following user name is generated using the Azure C SDK API.
 * The following macro will be used in case of direct connection (without Azure SDK).
 */
#define IOT_AZURE_USERNAME                       "Replace this string by generated username using host name and device identifier" /* Value will change depending on the hub and device name. */
#define IOT_AZURE_USERNAME_LENGTH                ( ( uint16_t ) ( sizeof( IOT_AZURE_USERNAME ) - 1 ) )

#define IOT_AZURE_PASSWORD                       "Replace this string by generated SAS token"
#define IOT_AZURE_PASSWORD_LENGTH                ( ( uint16_t ) ( sizeof( IOT_AZURE_PASSWORD ) - 1 ) )

/* For DPS application */
#define IOT_AZURE_DPS_REGISTRATION_ID            "Replace this string by generated registration ID from Azure portal for DPS"
#define IOT_AZURE_DPS_REGISTRATION_ID_LEN        ( ( uint16_t ) ( sizeof( IOT_AZURE_DPS_REGISTRATION_ID ) - 1 ) )
#define IOT_AZURE_ID_SCOPE                       "Replace this string by generated ID scope from Azure portal for DPS"
#define IOT_AZURE_ID_SCOPE_LEN                   ( ( uint16_t ) ( sizeof( IOT_AZURE_ID_SCOPE ) - 1 ) )

#ifdef CY_SECURE_SOCKETS_PKCS_SUPPORT
/* For PKCS feature support demo on CY8CKIT-064S0S2-4343W hardware, RootCA, device key and cert are provisioned to hardware.
 * No need to send cert and keys from application.
 */
#define IOT_AZURE_ROOT_CA_LENGTH                 ( 0 )
#define IOT_AZURE_CLIENT_CERT_LENGTH             ( 0 )
#define IOT_AZURE_CLIENT_KEY_LENGTH              ( 0 )
#else
#define IOT_AZURE_ROOT_CA_LENGTH                 ( ( uint16_t ) ( sizeof( azure_root_ca_certificate ) ) )
#define IOT_AZURE_CLIENT_CERT_LENGTH             ( ( uint16_t ) ( sizeof( azure_client_cert ) ) )
#define IOT_AZURE_CLIENT_KEY_LENGTH              ( ( uint16_t ) ( sizeof( azure_client_key ) ) )
#endif

/**********************************************************************************************/

/* Azure Broker connection Info */
static const char azure_root_ca_certificate[] =
"Replace this string with generated Root CA";

/* Azure - client certificate */
static const char azure_client_cert[] =
"Replace this string with generated client certificate";

/* Azure - private key */
static const char azure_client_key[] =
"Replace this string with generated private key";
/*****************************************************************************************/
#endif /* CY_MQTT_APP_H_ */
