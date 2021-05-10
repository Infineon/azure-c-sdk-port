# Azure C SDK Port Library

## Introduction

This library implements the port layer for the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0) to work on PSoC&trade; 6 MCU connectivity-enabled platforms. This library automatically pulls the Azure SDK for Embedded C library; the port layer functions implemented by this library are used by the [Azure SDK for Embedded C](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0) library. If application needs Azure SDK for Embedded C library with MQTT client functionality, it needs to explicitly import MQTT library. Refer README.md located in *./sample_app/README.md* for additional information.

## Features

- Supports RESTful HTTP methods: `HEAD`, `GET`, `PUT`, `POST`, `DELETE`, and `PATCH`

- See [Azure SDK features](https://github.com/Azure/azure-sdk-for-c/blob/master/sdk/docs/iot/README.md) for the list of IoT features supported

- This library also includes sample applications which demonstarte Azure IoT hub, and Azure DPS usecase using [MQTT Library](https://github.com/cypresssemiconductorco/mqtt/releases/tag/release-v3.1.0) and [Microsoft Azure SDK for Embedded C Library](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0).

## Supported Platforms

- [PSoC 6 WiFi-BT Prototyping Kit (CY8CPROTO-062-4343W)](https://www.cypress.com/documentation/development-kitsboards/psoc-6-wi-fi-bt-prototyping-kit-cy8cproto-062-4343w)

- [PSoC 62S2 Wi-Fi BT Pioneer Kit (CY8CKIT-062S2-43012)](https://www.cypress.com/documentation/development-kitsboards/psoc-62s2-wi-fi-bt-pioneer-kit-cy8ckit-062s2-43012)

## Supported Frameworks

This middleware library is supported on the AnyCloud framework.

AnyCloud is a FreeRTOS-based solution. This framework uses the [abstraction-rtos](https://github.com/cypresssemiconductorco/abstraction-rtos) library for RTOS abstraction APIs, the [http-client](https://github.com/cypresssemiconductorco/http-client/releases/tag/release-v1.0.0) library for sending HTTP client requests and receiving HTTP server responses, and the [mqtt](https://github.com/cypresssemiconductorco/mqtt/releases/tag/release-v3.1.0) library for Azure-supported IoT hub services.

## Dependencies

- [Microsoft Azure SDK for Embedded C Library](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0)

- [Wi-Fi Middleware Core](https://github.com/cypresssemiconductorco/wifi-mw-core)

- [HTTP Client](https://github.com/cypresssemiconductorco/http-client/releases/tag/release-v1.0.0)

- [AWS IoT SDK Port](https://github.com/cypresssemiconductorco/aws-iot-device-sdk-port/releases/tag/release-v1.0.0)

## Quick Start

This library is supported only on the AnyCloud framework.

1. Review pre-defined configuration files that have been bundled with the *wifi-mw-core* library for FreeRTOS, lwIP, and mbed TLS, and make the required changes.

   See the "Quick Start" section in [README.md](https://github.com/cypresssemiconductorco/wifi-mw-core/blob/master/README.md).

2. Define the following COMPONENTS in the application's Makefile for the Azure Port Library.

   For additional information, see the "Quick Start" section in [README.md](https://github.com/cypresssemiconductorco/wifi-mw-core/blob/master/README.md).

    ```
    COMPONENTS=FREERTOS MBEDTLS LWIP SECURE_SOCKETS
    ```
3. By default, the Azure Port Library disables all the debug log messages. To enable log messages, the application must perform the following:

   1. Add the `ENABLE_AZ_PORT_LOGS` macro to the *DEFINES* in the code example's Makefile. The Makefile entry would look like as follows:
       ```
       DEFINES+=ENABLE_AZ_PORT_LOGS
       ```
   2. Call the `cy_log_init()` function provided by the *cy-log* module. cy-log is part of the *connectivity-utilities* library.
      Refer [connectivity-utilities library API documentation](https://cypresssemiconductorco.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for input arguments and usage of this API.

4. Define the following macro in the application's Makefile for HTTP client configuration:

   1. This is an optional configuration to change the response header maximum length to 'N'. By default, this value will be set to 2048:
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
5. Sample applications for Azure IoT hub, and Azure DPS features are located in folder *./sample_app/*. For running the sample applications on PSoC&trade; 6 MCU connectivity-enabled platforms, Refer README.md located in *./sample_app/README.md*.

## Additional Information

- [Azure SDK Port Library RELEASE.md](./RELEASE.md)

- [Azure SDK Port Library Version](./version.txt)

- [Azure SDK Port Library API Documentation](https://cypresssemiconductorco.github.io/azure-sdk-port/api_reference_manual/html/index.html)

- [Microsoft Azure SDK for Embedded C Library](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0)

- [Connectivity Utilities API documentation](https://cypresssemiconductorco.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for details of cy-log

- [ModusToolbox&trade; Software Environment, Quick Start Guide, Documentation, and Videos](https://www.cypress.com/products/modustoolbox-software-environment)

- [ModusToolbox AnyCloud code examples](https://github.com/cypresssemiconductorco?q=mtb-example-anycloud%20NOT%20Deprecated)