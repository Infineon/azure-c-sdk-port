# Azure C SDK Port Library - Azure IoT Connectivity Port Layer for Infineon MCUs

# Overview

The Azure C SDK Port Library implements the port layer that enables the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0) to run on PSOC&trade; 6 MCU connectivity-enabled platforms. It automatically pulls in the Azure SDK for Embedded C, and the port-layer functions it implements are consumed by that SDK to connect Infineon devices to Azure IoT services. If your application needs MQTT client functionality with the Azure SDK, it must explicitly import the Infineon [MQTT library](https://github.com/Infineon/mqtt/releases/tag/release-v3.1.0).

## Supported Platforms

This library is supported on the following Infineon device families:

- PSOC&trade; 6 with CYW4343W / CYW43012 / CYW43439 / CYW43022 / CYW955513 connectivity
- PSOC&trade; 64 "Standard Secure" (for example, CY8CKIT-064S0S2-4343W)
- XMC7200
- PSOC&trade; Edge E84

For the specific kits validated with this release, see [RELEASE.md](./RELEASE.md).

# Features

- Supports RESTful HTTP methods: `HEAD`, `GET`, `PUT`, `POST`, `DELETE`, and `PATCH`
- Supports the Azure IoT features provided by the Azure SDK for Embedded C — see [Azure SDK IoT features](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/README.md)
- Bridges the Azure SDK transport interface to Infineon's HTTP Client and secure sockets libraries for secure and non-secure HTTP requests to Azure IoT Hub and the Azure Device Provisioning Service (DPS)

## Supported software and tools

This version of the library was validated for compatibility with the following software and tools:

| Software and tools                                              | Version |
| :---                                                            | :---    |
| ModusToolbox&trade; software environment                        | 3.9     |
| ModusToolbox&trade; Device Configurator                         | 5.90    |
| GCC Compiler                                                    | 14.2.1  |
| IAR Compiler                                                    | 9.50.2  |
| Arm&reg; Compiler 6                                             | 6.22    |
| LLVM compiler                                                   | 22.1.0  |

# When to Use

Use this library when building IoT applications on Infineon connectivity-enabled MCU platforms (such as PSOC&trade; 6, XMC7200, or PSOC&trade; Edge E84) that connect to Azure IoT Hub or the Azure Device Provisioning Service (DPS) using the Azure SDK for Embedded C. It is the required port layer that lets the Azure SDK use Infineon's HTTP Client and MQTT libraries for cloud connectivity.

# How to Use

This library is supported only on the ModusToolbox&trade; environment and is consumed together with the Infineon connectivity stack. Its port-layer APIs are called by the Azure SDK for Embedded C rather than directly by the application.

## Add Required Libraries

Add this library and its dependencies using the ModusToolbox&trade; **Library Manager** (available in the ModusToolbox&trade; for VS Code extension and the Eclipse IDE), or by creating the corresponding *.mtb* files in the application's `deps` folder and running `make getlibs`. Dependencies that are pinned to a specific commit or tag (for example, `freertos-pkcs11-psa`) can only be added through a *.mtb* file.

This library depends on:

- [Microsoft Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0)
- [Wi-Fi middleware core](https://github.com/Infineon/wifi-mw-core)
- [HTTP Client](https://github.com/Infineon/http-client)
- [MQTT](https://github.com/Infineon/mqtt/releases/tag/release-v3.1.0)
- [AWS IoT Device SDK Port](https://github.com/Infineon/aws-iot-device-sdk-port)
- [FreeRTOS PKCS11 PSA](https://github.com/Linaro/freertos-pkcs11-psa)
- Trusted Firmware-M

## Configure the ModusToolbox&trade; Environment

Review and make the required changes to the pre-defined configuration files:

- The configuration files are bundled with the Wi-Fi core library for FreeRTOS, lwIP, and Mbed TLS. See the "Quick Start" section in the [Wi-Fi Core FreeRTOS lwIP Mbed TLS README](https://github.com/Infineon/wifi-core-freertos-lwip-mbedtls/blob/master/README.md).
- If the application uses a bundle library, the configuration files are in that bundle library. For example, if the application uses the Wi-Fi Core FreeRTOS lwIP Mbed TLS bundle library, the configuration files are in the `wifi-core-freertos-lwip-mbedtls/configs` folder. Similarly, if the application uses the Ethernet Core FreeRTOS lwIP Mbed TLS library, the configuration files are in the `ethernet-core-freertos-lwip-mbedtls/configs` folder.

## Enable the Required COMPONENTS

Define the following COMPONENTS in the application's Makefile:

```makefile
COMPONENTS=FREERTOS MBEDTLS LWIP SECURE_SOCKETS
```

## Enable Debug Logs

By default, the Azure port library disables all debug log messages. To enable them:

1. Add the `ENABLE_AZ_PORT_LOGS` macro to the *DEFINES* in the application's Makefile:

   ```makefile
   DEFINES+=ENABLE_AZ_PORT_LOGS
   ```

2. Call the `cy_log_init()` function provided by the *cy-log* module (part of the *connectivity-utilities* library). See the [Connectivity Utilities Reference Manual](https://Infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for input arguments and usage.

## Configure the HTTP Client

Set the following macros in the application's Makefile for HTTP client configuration:

1. (Optional) Change the response header maximum length to `N`. By default, this value is `2048`:

   ```makefile
   DEFINES+=HTTP_MAX_RESPONSE_HEADERS_SIZE_BYTES=<N>
   ```

2. Configure the user agent name used in all HTTP request headers. By default, this component is added to the request header. Update this for user-defined agent values:

   ```makefile
   DEFINES += HTTP_USER_AGENT_VALUE="\"anycloud-http-client\""
   ```

3. Disable the custom configuration header files that are not needed for the build:

   ```makefile
   DEFINES += HTTP_DO_NOT_USE_CUSTOM_CONFIG
   DEFINES += MQTT_DO_NOT_USE_CUSTOM_CONFIG
   ```

4. If the application does not use MQTT client features, add the following paths to the application's *.cyignore* file to exclude the coreMQTT source files from the build:

   ```
   $(SEARCH_aws-iot-device-sdk-embedded-C)/libraries/standard/coreMQTT
   libs/aws-iot-device-sdk-embedded-C/libraries/standard/coreMQTT
   ```

## API Flow for an HTTP Request

The Azure C SDK provides C APIs to connect with the Azure cloud and to form and send HTTP requests to the server. The following diagram shows the API call sequence to issue a secure or non-secure HTTP request:

```mermaid
flowchart TD
    A[cy_http_client_init] --> B[cy_http_client_create]
    B --> C[cy_http_client_connect]
    C --> D[az_http_response_init]
    D --> E[az_http_client_send_request]
    E --> F[cy_http_client_disconnect]
    F --> G[cy_http_client_delete]
    G --> H[cy_http_client_deinit]
```

## Code Snippets

The following snippets show how the Azure APIs are used to issue secure and non-secure HTTP requests to the server.

### Send a Non-Secure HTTP GET Request

Send an HTTP GET request (header + body) to the server and receive the response:

```c
/* HTTP server configuration macros */
#define SERVER_HOST                       "httpbin.org"
#define SERVER_PORT                       80

/* HTTP socket send and receive timeout */
#define TRANSPORT_SEND_RECV_TIMEOUT_MS    ( 5000 )
/* URL to read the data */
#define GET_PATH                          "/get"
/* User buffer for HTTP request and response */
#define USER_BUFFER_LENGTH                ( 2048 )
/* Request body in the HTTP header */
#define REQUEST_BODY                      "Hello, world!"
#define REQUEST_BODY_LENGTH               ( sizeof( REQUEST_BODY ) - 1 )

/* Application buffer to hold the HTTP request and response */
static uint8_t userBuffer[ USER_BUFFER_LENGTH ];
/* Server information for connection */
static cy_awsport_server_info_t server_info;
/* HTTP client object used to send and receive the requests */
static cy_http_client_t handle;
/* Azure SDK structure containing HTTP client request */
static az_http_request az_request;
/* Azure SDK structure containing HTTP client header */
static _az_http_request_header az_header;
/* Azure SDK structure containing HTTP server response */
static az_http_response az_response;

static void disconnect_callback( cy_http_client_t handle, cy_http_client_disconn_type_t type, void *args )
{
    /* Application disconnect callback triggered */
}

void snippet_non_secure_http_client_send(void)
{
    /* Status variables for various operations */
    cy_rslt_t result = CY_RSLT_SUCCESS;
    az_result returnStatus;
    /* User callback on disconnect */
    cy_http_disconnect_callback_t http_cb;

    ( void ) memset( &server_info, 0, sizeof( server_info ) );

    /* HTTP server information for connection */
    server_info.host_name = SERVER_HOST;
    server_info.port = SERVER_PORT;

    /* Initialize the HTTP Client Library */
    result = cy_http_client_init();
    if( result != CY_RSLT_SUCCESS )
    {
        /* Failure path */
    }

    /* HTTP client disconnect callback */
    http_cb = disconnect_callback;

    /* Create an instance of the HTTP client */
    result = cy_http_client_create( NULL, &server_info, http_cb, NULL, &handle );
    if( result != CY_RSLT_SUCCESS )
    {
        /* Failure path */
    }

    /* Connect the HTTP client to the server */
    result = cy_http_client_connect( handle, TRANSPORT_SEND_RECV_TIMEOUT_MS, TRANSPORT_SEND_RECV_TIMEOUT_MS );
    if( result != CY_RSLT_SUCCESS )
    {
        /* Failure path */
    }

    /* Copy user buffer to Azure buffer structure for HTTP request handling */
    az_span az_buffer = az_span_create( ( uint8_t* )userBuffer, ( int32_t )USER_BUFFER_LENGTH );

    /* Initialize the response structure to receive the HTTP server response */
    returnStatus = az_http_response_init( &az_response, az_buffer );
    if( returnStatus != AZ_OK )
    {
        /* Failure path */
    }

    /* HTTP client handle for HTTP request send */
    az_request._internal.context = handle;
    az_request._internal.method = az_http_method_get();
    az_request._internal.url._internal.ptr = ( uint8_t * )GET_PATH;
    az_request._internal.url_length = strlen( GET_PATH );

    /* Update the header */
    az_request._internal.headers_length = 1;
    az_header.name._internal.ptr = ( uint8_t * )"Connection";
    az_header.name._internal.size = sizeof( "Connection" ) - 1;
    az_header.value._internal.ptr = ( uint8_t * )"keep-alive";
    az_header.value._internal.size = sizeof( "keep-alive" ) - 1;

    az_request._internal.headers._internal.ptr = ( uint8_t * )&az_header;
    az_request._internal.headers._internal.size = sizeof ( az_header ) * az_request._internal.headers_length;

    az_request._internal.body._internal.ptr = ( uint8_t * )REQUEST_BODY;
    az_request._internal.body._internal.size = REQUEST_BODY_LENGTH;

    /* Azure SDK API to send the HTTP client request and receive the HTTP response */
    returnStatus = az_http_client_send_request( &az_request, &az_response );
    if( returnStatus != AZ_OK )
    {
        /* Failure path */
    }

    /* Disconnect the HTTP client from the server */
    result = cy_http_client_disconnect(handle);
    if( result != CY_RSLT_SUCCESS )
    {
        /* Failure path */
    }

    /* Delete the instance of the HTTP client */
    result = cy_http_client_delete(handle);
    if( result != CY_RSLT_SUCCESS )
    {
        /* Failure path */
    }

    /* Deinit the HTTP library */
    result = cy_http_client_deinit();
    if( result != CY_RSLT_SUCCESS )
    {
        /* Failure path */
    }
}
```

### Send a Secure HTTP GET Request

A secure request follows the same call sequence as the non-secure request, with the addition of TLS credentials passed as the first argument to `cy_http_client_create()`. Configure the credentials and create the client as follows; the remaining connect, send, disconnect, and cleanup steps are identical to the non-secure snippet:

```c
/* SSL server certificate */
#define SSL_SERVERCERT_PEM      \
"-----BEGIN CERTIFICATE-----\n" \
"........base64 data........\n" \
"-----END CERTIFICATE-----"

/* SSL server private key */
#define SSL_SERVERKEY_PEM          \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"..........base64 data..........\n" \
"-----END RSA PRIVATE KEY-----"

/* SSL root CA certificate */
#define SSL_ROOTCA_PEM            \
"-----BEGIN CERTIFICATE-----\n"   \
"........base64 data........\n"   \
"-----END CERTIFICATE-----"

/* SSL credentials information */
static cy_awsport_ssl_credentials_t credentials;

( void ) memset( &credentials, 0, sizeof( credentials ) );

/* Set the credential information */
credentials.client_cert = ( const char * ) &SSL_SERVERCERT_PEM;
credentials.client_cert_size = sizeof( SSL_SERVERCERT_PEM );
credentials.private_key = ( const char * ) &SSL_SERVERKEY_PEM;
credentials.private_key_size = sizeof( SSL_SERVERKEY_PEM );
credentials.root_ca = ( const char * ) &SSL_ROOTCA_PEM;
credentials.root_ca_size = sizeof( SSL_ROOTCA_PEM );

/* Create a secure instance of the HTTP client by passing the credentials */
result = cy_http_client_create( &credentials, &server_info, http_cb, NULL, &handle );
```

## Additional Information

- [Azure C SDK Port Library Reference Manual](https://Infineon.github.io/azure-sdk-port/api_reference_manual/html/index.html)
- [Microsoft Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0)
- [MQTT Reference Manual](https://Infineon.github.io/mqtt/api_reference_manual/html/index.html)
- [HTTP Client Reference Manual](https://Infineon.github.io/http-client/api_reference_manual/html/index.html)
- [Connectivity Utilities Reference Manual (cy-log)](https://Infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html)
- [ModusToolbox&trade; software environment, quick start guide, documentation, and videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software)
- [Azure code examples for ModusToolbox&trade;](https://github.com/search?q=org%3AInfineon+azure+mtb-example&type=repositories)

# Release Notes and Changelog

- [RELEASE.md](./RELEASE.md) - Detailed release notes for all versions

# License

This library is provided under Infineon software license terms. It automatically pulls the [Microsoft Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0), which is governed by its own license terms, and depends on additional third-party components including `freertos-pkcs11-psa` and Trusted Firmware-M. You are responsible for reviewing and accepting all applicable third-party license terms.

- [LICENSE](./LICENSE.txt)

# Copyright

(c) 2026, Infineon Technologies AG, or an affiliate of Infineon
Technologies AG.  All rights reserved.
