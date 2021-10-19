# Azure C SDK port library

## Introduction

This library implements the port layer for the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0) to work on PSoC&trade; 6 MCU connectivity-enabled platforms. This library automatically pulls the Azure SDK for Embedded C library; the port layer functions implemented by this library are used by the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0) library. If your application needs Azure SDK for Embedded C library with MQTT client functionality, it needs to explicitly import the MQTT library. See *README.md* located in *./sample_app/README.md* for additional information.

## Features

- Supports RESTful HTTP methods: `HEAD`, `GET`, `PUT`, `POST`, `DELETE`, and `PATCH`

- See [Azure SDK features](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/README.md) for the list of IoT features supported

- This library also includes sample applications which demonstrate Azure IoT hub, and Azure DPS use case using [MQTT library](https://github.com/Infineon/mqtt/releases/tag/release-v3.1.0) and [Microsoft Azure SDK for Embedded C Library](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0).

## Supported platforms

- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; prototyping kit (CY8CPROTO-062-4343W)](https://www.cypress.com/documentation/development-kitsboards/psoc-6-wi-fi-bt-prototyping-kit-cy8cproto-062-4343w)

- [PSoC&trade; 62S2 Wi-Fi Bluetooth&reg; pioneer kit (CY8CKIT-062S2-43012)](https://www.cypress.com/documentation/development-kitsboards/psoc-62s2-wi-fi-bt-pioneer-kit-cy8ckit-062s2-43012)

- [PSoC&trade; 64 "Standard Secure" AWS Wi-Fi Bluetooth&reg; pioneer kit (CY8CKIT-064S0S2-4343W)](https://www.cypress.com/documentation/development-kitsboards/psoc-64-standard-secure-aws-wi-fi-bt-pioneer-kit-cy8ckit)

- [PSoC&trade; 62S2 evaluation kit (CY8CEVAL-062S2-LAI-4373M2)](https://www.cypress.com/documentation/development-kitsboards/psoc-62s2-evaluation-kit-cy8ceval-062s2)

## Supported frameworks

This middleware library is supported on the ModusToolbox&trade; environment.

This environment uses the [abstraction-rtos](https://github.com/Infineon/abstraction-rtos) library for RTOS abstraction APIs, the [http-client](https://github.com/Infineon/http-client/releases/tag/release-v1.0.0) library for sending HTTP client requests and receiving HTTP server responses, and the [mqtt](https://github.com/Infineon/mqtt/releases/tag/release-v3.1.0) library for Azure-supported IoT hub services.

## Dependencies

- [Microsoft Azure SDK for Embedded C library](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0)

- [Wi-Fi middleware core](https://github.com/Infineon/wifi-mw-core)

- [HTTP client](https://github.com/Infineon/http-client/releases/tag/release-v1.0.0)

- [AWS IoT SDK port](https://github.com/Infineon/aws-iot-device-sdk-port/releases/tag/release-v1.0.0)

- [FreeRTOS PKCS11 PSA](https://github.com/Linaro/freertos-pkcs11-psa)

- Trusted-Firmware-m library

## Quick start

This library is supported only on the ModusToolbox&trade; environment.

1. Review pre-defined configuration files that have been bundled with the *wifi-mw-core* library for FreeRTOS, lwIP, and mbed TLS, and make the required changes.

   See the [README.md](https://github.com/Infineon/wifi-mw-core/blob/master/README.md).

2. Define the following COMPONENTS in the application's Makefile for the Azure port library.

   For additional information, see the [README.md](https://github.com/Infineon/wifi-mw-core/blob/master/README.md).

    ```
    COMPONENTS=FREERTOS MBEDTLS LWIP SECURE_SOCKETS
    ```
3. By default, the Azure port library disables all the debug log messages. Do the following to enable log messages:

   1. Add the `ENABLE_AZ_PORT_LOGS` macro to the *DEFINES* in the code example's Makefile. The Makefile entry would look like as follows:
       ```
       DEFINES+=ENABLE_AZ_PORT_LOGS
       ```
   2. Call the `cy_log_init()` function provided by the *cy-log* module. cy-log is part of the *connectivity-utilities* library.
      See [connectivity-utilities library API documentation](https://Infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for input arguments and usage of this API.

4. Do the following in the application's Makefile for HTTP client configuration:

   1. (Optional) Change the response header maximum length to 'N'. By default, this value is set to `2048`:
       ```
       DEFINES+=HTTP_MAX_RESPONSE_HEADERS_SIZE_BYTES=<N>
       ```
   2. Define the following macro in the application's Makefile to configure the user agent name in all HTTP request headers. By default, this component will be added to the request header. Update this for user-defined agent values:

       ```
       DEFINES += HTTP_USER_AGENT_VALUE="\"anycloud-http-client\""
       ```
   3. Define the following macros in the application's Makefile to mandatorily disable custom configuration header file:
       ```
       DEFINES += HTTP_DO_NOT_USE_CUSTOM_CONFIG
       DEFINES += MQTT_DO_NOT_USE_CUSTOM_CONFIG
       ```
   4. If the user application doesn't use MQTT client features, add the following path in the *.cyignore* file of the application to exclude the coreMQTT source files from the build:
       ```
       $(SEARCH_aws-iot-device-sdk-embedded-C)/libraries/standard/coreMQTT
       libs/aws-iot-device-sdk-embedded-C/libraries/standard/coreMQTT
       ```
Sample applications for Azure IoT hub and Azure DPS features are located in *./sample_app/*. For running the sample applications on PSoC&trade; 6 MCU connectivity-enabled platforms, see README.md located in *./sample_app/README.md*.

## Additional information

- [Azure SDK port library RELEASE.md](./RELEASE.md)

- [Azure SDK port library version](./version.xml)

- [Azure SDK port library API documentation](https://Infineon.github.io/azure-sdk-port/api_reference_manual/html/index.html)

- [Microsoft Azure SDK for Embedded C library](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0)

- [Connectivity utilities API documentation](https://Infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for details of cy-log

- [ModusToolbox&trade; software environment, quick start guide, documentation, and videos](https://www.cypress.com/products/modustoolbox-software-environment)

- [ModusToolbox&trade; AnyCloud code examples](https://github.com/Infineon?q=mtb-example-anycloud%20NOT%20Deprecated)
