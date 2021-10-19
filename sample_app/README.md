# Azure IoT hub applications for ModusToolbox&trade; environment

## Introduction

This document explains how to build an application for Azure-supported IoT hub services on ModusToolbox&trade;.

This Azure example supports PKCS and non-PKCS modes. PKCS11 is a cryptography standard in which device credentials are used from the hardware security module. On the other hand, non-PKCS mode uses device credentials loaded from the application.


## Supported platforms

Azure sample examples are supported on various platforms.

### [CY8CPROTO-062-4343W](https://www.cypress.com/documentation/development-kitsboards/psoc-6-wi-fi-bt-prototyping-kit-cy8cproto-062-4343w)


#### **Non-PKCS mode**

The *CY8CPROTO-062-4343W* platform supports only non-PKCS mode in which the device certificate, device private key, and RootCA certificate should be loaded from the application to make the connection to Azure cloud.

### [CY8CKIT-064S0S2-4343W](https://www.cypress.com/documentation/development-kitsboards/psoc-64-standard-secure-aws-wi-fi-bt-pioneer-kit-cy8ckit)

#### **Non-PKCS mode**

This platform supports non-PKCS mode in which the device certificate, device private key, and RootCA certificate should be loaded from the application to make the connection to Azure cloud.

#### **PKCS mode**

This platform also supports PKCS mode in which the device certificate, device private key, and RootCA certificate should be provisioned to the "secure" hardware/"secure" element. By default, provisioned certificates and keys will be used for connection to the cloud. If the RootCA certificate or device certificate and device keys are loaded from the application, those will be used to make the connection to the cloud.

## Hardware setup for secured platform (*CY8CKIT-064S0S2-4343W*)

### X509 certificate-based authentication mode

Before running the sample application, you need to provision a valid device certificate, device keys, and Azure RootCA certificate to the "secure" element.

Azure sample applications supports PSA for a secured connection to the Azure broker. Do the following to enable the PSA flow:

1. Provision the "secure" kit with the device certificate and RootCA certificate. See [Device provisioning steps](https://community.cypress.com/t5/Resource-Library/Provisioning-Guide-for-the-Cypress-CY8CKIT-064S0S2-4343W-Kit/ta-p/252469).

2. Register the new X509 authentication-based device in the Azure portal using the provisioned device certificate footprint.

   **Note:** Do not pass any certificates/key from the application. While establishing the connection with the Azure broker, certificates and keys stored in the secured memory are used.

### Shared access signature (SAS)-based authentication mode

Before running the Azure IoT hub sample application, you need to store a valid device ID and corresponding SAS token in the secured element.

Use the sample application *mqtt_iot_sas_token_provision.c* from *sample_app* to store the device ID and SAS token.

1. Fill the valid device ID in the `AZURE_CLIENT_DEVICE_ID` macro, and SAS token in the `AZURE_CLIENT_SAS_TOKEN` macro present in the provisioning sample application (*mqtt_iot_sas_token_provision.c*).

2. Run the sample application (*mqtt_iot_sas_token_provision.c*) to store the device ID and SAS token in the secured element.

   **Note:** For SAS-based authentication mode, the application should pass RootCA, private key, and device certificate value as NULL. In addition, the application  must set the RootCA, private key, and device certificate location as RAM to avoid reading it from the secured element during a TLS connection.


## Compile the application

1. Download the *sample_app* folder from the *azure-c-sdk-port* repo. The *sample_app/deps* folder of the application contains the following:

   Folder | Description
   -------|------------
   *non_secured* | Contains the dependent library .lib files for *CY8CPROTO-062-4343W* kit
   *secured* | Contains the dependent library .lib files for *CY8CKIT-064S0S2-4343W*.

2. Depending on the hardware used for the demonstration, delete the *.lib* files of other kits to avoid building of libraries which are not required.

3. Copy the *Makefile* from *platform/non_secured* to the *sample_app* directory for the *CY8CPROTO-062-4343W* kit.

4. For *pkcs* mode, copy the *Makefile* from *platform/secured/pkcs* to the *sample_app* directory. For *non-pkcs* mode copy the *Makefile* from *platform/secured/non_pkcs* to the *sample_app* directory for the *CY8CKIT-064S0S2-4343W* kit.

5. From the *sample_app* directory where the Makefile is, run the following command:
   ```
   make getlibs
   ```

6. Copy *FreeRTOSConfig.h* from *wifi-mw-core/configs* to the current directory.

7. Copy *lwipopts.h* from *wifi-mw-core/configs* to the current directory.

8. Define the macros `CY_TFM_PSA_SUPPORTED` and `TFM_MULTI_CORE_NS_OS` in the application Makefile to add PSA library support for the *CY8CKIT-064S0S2-4343W* hardware platform.
   ```
   DEFINES+= CY_TFM_PSA_SUPPORTED TFM_MULTI_CORE_NS_OS
   ```

9. Define the macro `CY_SECURE_SOCKETS_PKCS_SUPPORT` in the application Makefile to enable PKCS support in secure socket library for the *CY8CKIT-064S0S2-4343W* hardware platform. This macro is required only for X509 certificate-based authentication mode.
    ```
    DEFINES+= CY_SECURE_SOCKETS_PKCS_SUPPORT
    ```

10. Update the device and certificate information in the *mqtt_main.h*` file.

    **Shared access signature (SAS)-based authentication mode:**

     1. Set the `SAS_TOKEN_AUTH` macro  to `1`.

     2. Set the `SAS_TOKEN_LOCATION_FLASH` macro as `true`, to read device ID from macro `MQTT_CLIENT_IDENTIFIER_AZURE_SAS` and SAS token from `IOT_AZURE_PASSWORD`.

     3. Set the `SAS_TOKEN_LOCATION_FLASH` macro as `false`, to read the device ID and SAS token from the secured element(For *CY8CKIT-064S0S2-4343W* hardware platform).

     4. Update host name of the created IoT hub in the `IOT_DEMO_SERVER_AZURE` macro.

     5. Update the device ID in the `MQTT_CLIENT_IDENTIFIER_AZURE_SAS` macro if the `SAS_TOKEN_LOCATION_FLASH` macro is `true`.

     6. For the device provisioning service (DPS) application, update the DPS registration ID in the `IOT_AZURE_DPS_REGISTRATION_ID` macro. Update the ID scope value in the `IOT_AZURE_ID_SCOPE` macro.

     7. For Azure hub applications, generate the SAS tokens for the device by following the instructions at [SAS token generation](https://docs.microsoft.com/en-us/cli/azure/ext/azure-iot/iot/hub?view=azure-cli-latest#ext_azure_iot_az_iot_hub_generate_sas_token).

     8. For device provisioning service (DPS) application, use the *dps_sas_token_generation.py* Python script to generate the SAS token. Update valid `Scope ID`, `Registration ID`, and `Provisioning SAS Key` values in *dps_sas_token_generation.py* before executing the script. Upon successful execution, an SAS token will be printed on the console.

     9. Update the generated SAS token in the `IOT_AZURE_PASSWORD` macro if the `SAS_TOKEN_LOCATION_FLASH` macro is `true`.


     **X509 certificate-based authentication mode:**

     1. Update the host name of the created IoT hub in the `IOT_DEMO_SERVER_AZURE` macro.

     2. Update the device ID in the `MQTT_CLIENT_IDENTIFIER_AZURE_CERT` macro.

        For the *CY8CPROTO-062-4343W* kit, see [X509 Ceritificate generation](https://docs.microsoft.com/en-us/azure/iot-edge/how-to-create-test-certificates?view=iotedge-2018-06) to generate the certificates for a device.

        For the *CY8CKIT-064S0S2-4343W* kit, see [CY8CKIT-064S0S2-4343W Provisioning Guide](https://community.cypress.com/t5/Resource-Library/Provisioning-Guide-for-the-Cypress-CY8CKIT-064S0S2-4343W-Kit/ta-p/252469) to generate the certificate for a device and provisioning it to device.

     3. For the *CY8CPROTO-062-4343W* kit, update the generated certificate info in `azure_root_ca_certificate`, `azure_client_cert`, and `azure_client_key`.

     4. For the *CY8CKIT-064S0S2-4343W* kit, leave the fields `azure_root_ca_certificate`, `azure_client_cert`, and `azure_client_key` unchanged. Certificate and keys will be read from the secured memory during a TLS connection.

         - **Note:** For *CY8CKIT-064S0S2-4343W* kit, if `azure_root_ca_certificate`, `azure_client_cert`, and `azure_client_key` are supplied from the application, these keys and certificates will be used while establishing the connection to the Azure broker.

     5. For the device provisioning service (DPS) application, update the DPS registration ID in the `IOT_AZURE_DPS_REGISTRATION_ID` macro. Update the ID scope value in the `IOT_AZURE_ID_SCOPE` macro.

        This *sample_app* folder includes all the Azure-supported IoT hub applications. To demonstrate any specific application, update the application's Makefile to exclude other applications from the build. The Makefile entry would look like the following:

        ```
        CY_IGNORE+= <Application name>
        ```

        For example:
        ```
        CY_IGNORE+= mqtt_iot_hub_c2d_sample.c
        ```

     6. In *CY8CKIT-064S0S2-4343W* kit example, [FreeRTOS PSA PKCS11](https://github.com/Linaro/freertos-pkcs11-psa/) implementation supports only SHA-256 hashing algorithm. So the application should choose the cipher suite list compatible for SHA-256. To choose the cipher suite list, application need to add required cipher suites to the `MBEDTLS_SSL_CIPHERSUITES` macro in *configs/mbedtls_user_config.h* file. The sample *configs/mbedtls_user_config.h* file includes `MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` and `MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256` cipher suite to communicate with the Azure broker.
        ```
        #define MBEDTLS_SSL_CIPHERSUITES MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,\
                                         MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
        ```

        - **Note:** Refer to the list of supported cipher suites [Azure Hub](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-tls-support#cipher-suites) and [Azure DPS](https://docs.microsoft.com/en-us/azure/iot-dps/tls-support#recommended-ciphers) before adding cipher suites to the `MBEDTLS_SSL_CIPHERSUITES`.

11. Update the Wi-Fi details *WIFI_SSID* and *WIFI_KEY* in the *mqtt_main.h* file.

12. Run the following command to compile and program the application:

    ```
    make program TARGET=<TARGET> TOOLCHAIN=<TOOLCHAIN>
    ````

    For example:

    ```
    make program TARGET=CY8CPROTO-062-4343W TOOLCHAIN=GCC_ARM
    ```



## Sample applications


### mqtt_iot_hub_c2d_sample

This sample receives the incoming cloud-to-device (C2D) messages sent from the Azure IoT hub to the device. It  receives all messages sent from the service. If the network disconnects while waiting for a message, the sample will exit.

To run this application, ensure that all other applications are added to the "ignore" list of the application Makefile to avoid compilation errors.

Authentication based on both X509 certificate and shared access signature (SAS) token are supported.

- For X509 certificate based authentication mode, define the value of `SAS_TOKEN_AUTH` as `0`.

- For SAS token based authentication mode, define the value of `SAS_TOKEN_AUTH` as `1`.

To send a C2D message, select your device's *Message to Device* tab in the Azure portal for your IoT hub. Enter a message in the *Message Body* and click **Send Message**.


### mqtt_iot_hub_methods_sample

This sample receives incoming method commands invoked from the Azure IoT hub to the device. It receives all method commands sent from the service. If the network disconnects while waiting for a message, the sample will exit.

To run this application, ensure that all other applications are added to the "ignore" list of the application Makefile to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate based authentication mode, define the value of `SAS_TOKEN_AUTH` as `0`.

- For SAS token based authentication mode, define the value of `SAS_TOKEN_AUTH` as `1`.

To send a method command, select your device's *Direct Method* tab in the Azure portal for your IoT hub. Enter a method name and click **Invoke Method**. Only one method named `ping` is supported, which if successful will return the following JSON payload:

```
{"response": "pong"}
```

No other method commands are supported. If any other methods are attempted to be invoked, the log will report that the method is not found.


### mqtt_iot_hub_telemetry_sample

This sample sends 100 telemetry messages to the Azure IoT hub. If the network disconnects, the sample will exit.

To run this application, ensure that all other applications are added to the "ignore" list of the application Makefile to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate-based authentication mode, define the value of `SAS_TOKEN_AUTH` as `0`.

- For SAS token based-authentication mode, define the value of `SAS_TOKEN_AUTH` as `1`.


### mqtt_iot_hub_twin_sample

This sample uses the Azure IoT hub to get the device twin document, send a reported property message, and receive up to five desired property messages. When a desired property message is received, the sample application will update the twin property locally and send a reported property message back to the service. If the network disconnects while waiting for a message from the Azure IoT hub, the sample will exit.

To run this application, ensure that all other applications are added to the "ignore" list of the application Makefile to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate-based authentication mode, define the value of `SAS_TOKEN_AUTH` as `0`.

- For SAS token-based authentication mode, define the value of `SAS_TOKEN_AUTH` as `1`.

A property named `device_count` is supported for this sample. To send a device twin desired property message, select your device's *Device Twin* tab in the Azure Portal of your IoT hub. Add the `device_count` property along with the corresponding value to the `desired` section of the JSON. Click **Save** to update the twin document and send the twin message to the device.

```
{
    "properties":
    {
      "desired":
       {
           "device_count": 42,
       }
    }
}
```

No other property names in a desired property message are supported. If any are sent, the log will report that there is nothing to update.


### mqtt_iot_provisioning_sample

This sample registers a device with the Azure IoT Device Provisioning Service. It will wait to receive the registration status and print the provisioning information. If the network disconnects, the sample will exit.

To run this application, ensure that all other applications are added to the "ignore" list of the application Makefile to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate-based authentication mode, define the value of `SAS_TOKEN_AUTH` as `0`.

- For SAS token-based authentication mode, define the value of `SAS_TOKEN_AUTH` as `1`.


### mqtt_iot_pnp_sample

This sample connects an IoT plug-and-play enabled device with the Digital Twin Model ID (DTMI). The sample will exit if the network disconnects while waiting for a message, or after 10 minutes. This 10 minutes time constraint can be removed per application usage.

To run this application, ensure that all other applications are added to the "ignore" list of the application Makefile to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate-based authentication mode, define the value of `SAS_TOKEN_AUTH` as `0`.

- For SAS token-based authentication mode, define the value of `SAS_TOKEN_AUTH` as `1`.

To interact with this sample, use the Azure IoT Explorer or use the Azure portal directly. The capabilities are Device Twin, Direct Method (Command), and Telemetry.

#### **Device Twin**

Two device twin properties are supported in this sample:

1. A desired property named `targetTemperature` with a `double` value for the desired temperature.

2. A reported property named `maxTempSinceLastReboot` with a `double` value for the highest temperature reached since device boot.

To send a device twin desired property message, select your device's *Device Twin* tab in the Azure portal. Add the `targetTemperature` property along with a corresponding value to the desired section of the JSON object. Select **Save** to update the twin document and send the twin message to the device.

```
{
    "properties":
    {
      "desired":
      {
        "targetTemperature": 68.5,
      }
    }
}
```

When a desired property message is received, the sample application will update the twin property locally and send a reported property of the same name back to the service. This message will include a set of "ack" values: `ac` for the HTTP-like ack code, `av` for ack version of the property, and an optional `ad` for an ack description.

```
{
    "properties":
    {
      "reported":
      {
        "targetTemperature":
        {
          "value": 68.5,
          "ac": 200,
          "av": 14,
          "ad": "success"
        },
        "maxTempSinceLastReboot": 74.3,
      }
    }
}
```

#### **Direct method (command)**

One device command is supported in this sample: `getMaxMinReport`.

If any other commands are attempted to be invoked, the log will report that the command is not found. To invoke a command, select your device's *Direct Method* tab in the Azure portal. Enter the command name `getMaxMinReport` along with a payload using an ISO 8061 time format and select **Invoke method**. A sample payload is given below:

```
"2020-08-18T17:09:29-0700"
```

The command will send back to the service a response containing the following JSON payload with updated values in each field:

```
{
    "maxTemp": 74.3,
    "minTemp": 65.2,
    "avgTemp": 68.79,
    "startTime": "2020-08-18T17:09:29-0700",
    "endTime": "2020-08-18T17:24:32-0700"
}
```

**Note:** The system time at the time of sending the response will be reflected in endTime.

#### **Telemetry**

The device sends a JSON message with the field name `temperature` and the `double` value of the current temperature.
