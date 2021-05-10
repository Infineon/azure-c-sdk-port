# Azure IoT Hub Applications for AnyCloud

## Introduction

This document explains how to build an application for Azure-supported IoT Hub services on the AnyCloud platform.

## Compile the application

1. Download the *sample_app* folder from the *azure-c-sdk-port* repo.

2. *deps* folder inside *sample_app* folder have all the dependent library .lib files.

3. From the *sample_app* directory where the Makefile is, run the following command:
   ```
   make getlibs
   ```

4. Copy *FreeRTOSConfig.h* from *wifi-mw-core/configs* to the current directory.

5. Copy *lwipopts.h* from *wifi-mw-core/configs* to the current directory.

6. Update the device info and certificate information in the *MQTT_main.h*` file.

   **Shared access signature (SAS)-based authentication mode:**
    1. Set the `SAS_TOKEN_AUTH` macro  to `1`.

    2. Update the device ID in the `MQTT_CLIENT_IDENTIFIER_AZURE_SAS` macro.

    3. Generate SAS tokens for a device by following instructions at https://docs.microsoft.com/en-us/cli/azure/ext/azure-iot/iot/hub?view=azure-cli-latest#ext_azure_iot_az_iot_hub_generate_sas_token.

    4. Update the generated SAS token in the `IOT_AZURE_PASSWORD` macro.

    **X509 cert-based authentication mode:**


    1.  Update the device ID in the `MQTT_CLIENT_IDENTIFIER_AZURE_CERT` macro.

    2. See https://docs.microsoft.com/en-us/azure/iot-edge/how-to-create-test-certificates?view=iotedge-2018-06 to generate the certificates for a device.

    3. Update the generated cert info in `azure_root_ca_certificate`, `azure_client_cert`, and `azure_client_key`.

    4. For Device Provisioning Service (DPS) application, update the DPS registration ID in the `IOT_AZURE_DPS_REGISTRATION_ID` macro. Update the ID scope value in the `IOT_AZURE_ID_SCOPE` macro.

    This *sample_app* folder includes all the Azure-supported IoT hub applications. To demonstrate any specific application, update the application's Makefile to exclude other applications from the build. The Makefile entry would look like the following:

    ```
    CY_IGNORE+= <Application name>
    ```

    For example:
    ```
    CY_IGNORE+= mqtt_iot_hub_c2d_sample.c
    ```

7. Update Wi-Fi details *WIFI_SSID* and *WIFI_KEY* in the *mqtt_main.h* file.

8. Run the following command to compile and program the application:

    ```
    make program TARGET=<TARGET> TOOLCHAIN=<TOOLCHAIN>
    ````

    For example:

    ```
    make program TARGET=CY8CPROTO-062-4343W TOOLCHAIN=GCC_ARM
    ```

## Sample applications

### mqtt_iot_hub_c2d_sample

This sample receives the incoming cloud-to-device (C2D) messages sent from the Azure IoT Hub to the device. It  receives all messages sent from the service. If the network disconnects while waiting for a message, the sample will exit.

To run the *mqtt_iot_hub_c2d_sample* application, Make sure that all other applications are added in ignore list of application make file to avoid compilation errors.

Authentication based on both X509 certificate and shared access signature (SAS) token are supported.

- For X509 certificate based authentication mode, define the value of `SAS_TOKEN_AUTH` as '0'.

- For SAS token based authentication mode, define the value of `SAS_TOKEN_AUTH` as '1'.

To send a C2D message, select your device's *Message to Device* tab in the Azure Portal for your IoT Hub. Enter a message in the *Message Body* and click **Send Message**.

### mqtt_iot_hub_methods_sample

This sample receives incoming method commands invoked from the the Azure IoT Hub to the device. It receives all method commands sent from the service. If the network disconnects while waiting for a message, the sample will exit.

To run the *mqtt_iot_hub_methods_sample* application, Make sure that all other applications are added in ignore list of application make file to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate based authentication mode, define the value of `SAS_TOKEN_AUTH` as '0'.

- For SAS token based authentication mode, define the value of `SAS_TOKEN_AUTH` as '1'.

To send a method command, select your device's *Direct Method* tab in the Azure Portal for your IoT Hub. Enter a method name and click **Invoke Method**. Only one method named `ping` is supported, which if successful will return the following JSON payload:

```
{"response": "pong"}
```

No other method commands are supported. If any other methods are attempted to be invoked, the log will report the method is not found.

### mqtt_iot_hub_telemetry_sample

This sample sends 100 telemetry messages to the Azure IoT Hub. If the network disconnects, the sample will exit.

To run the *mqtt_iot_hub_telemetry_sample* application, Make sure that all other applications are added in ignore list of application make file to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate based authentication mode, define the value of `SAS_TOKEN_AUTH` as '0'.

- For SAS token based authentication mode, define the value of `SAS_TOKEN_AUTH` as '1'.

### mqtt_iot_hub_twin_sample

This sample uses the Azure IoT Hub to get the device twin document, send a reported property message, and receive up to 5 desired property messages. Upon receiving a desired property message, the sample will update the twin property locally and send a reported property message back to the service. If the network disconnects while waiting for a message from the Azure IoT Hub, the sample will exit.

To run the *mqtt_iot_hub_twin_sample* application, Make sure that all other applications are added in ignore list of application make file to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate based authentication mode, define the value of `SAS_TOKEN_AUTH` as '0'.

- For SAS token based authentication mode, define the value of `SAS_TOKEN_AUTH` as '1'.

A property named `device_count` is supported for this sample. To send a device twin desired property message, select your device's *Device Twin* tab in the Azure Portal of your IoT Hub. Add the `device_count` property along with a corresponding value to the `desired` section of the JSON. Click **Save** to update the twin document and send the twin message to the device.

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

No other property names sent in a desired property message are supported. If any are sent, the log will report there is nothing to update.

### mqtt_iot_provisioning_sample

This sample registers a device with the Azure IoT Device Provisioning Service. It will wait to receive the registration status and print the provisioning information. If the network disconnects, then the sample will exit.

To run the *mqtt_iot_provisioning_sample* application, Make sure that all other applications are added in ignore list of application make file to avoid compilation errors.

Currently supports X509 certificate based authentication. SAS token based authentication will be supported in the future.

- For X509 certificate based authentication mode, define the value of `SAS_TOKEN_AUTH` as '0'.

- For SAS token based authentication mode, define the value of `SAS_TOKEN_AUTH` as '1' --- for the future support.


### mqtt_iot_pnp_sample

This sample connects an IoT Plug-and-Play enabled device with the Digital Twin Model ID (DTMI). The sample will exit If network disconnects while waiting for a message, or after 10 minutes. This 10 minutes time constraint can be removed as per application usage.

To run the *mqtt_iot_pnp_sample* application, Make sure that all other applications are added in ignore list of application make file to avoid compilation errors.

Authentication based on both X509 certificate and SAS token are supported.

- For X509 certificate based authentication mode, define the value of `SAS_TOKEN_AUTH` as '0'.

- For SAS token based authentication mode, define the value of `SAS_TOKEN_AUTH` as '1'.

To interact with this sample, use the Azure IoT Explorer or directly Azure Portal. The capabilities are Device Twin, Direct Method (Command), and Telemetry.

#### **Device Twin**

Two device twin properties are supported in this sample.

1. A desired property named `targetTemperature` with a `double` value for the desired temperature.

2. A reported property named `maxTempSinceLastReboot` with a `double` value for the highest temperature reached since device boot.

To send a device twin desired property message, select your device's *Device Twin* tab in the Azure Portal. Add the property `targetTemperature` along with a corresponding value to the desired section of the JSON. Select **Save** to update the twin document and send the twin message to the device.

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

Upon receiving a desired property message, the sample will update the twin property locally and send a reported property of the same name back to the service. This message will include a set of "ack" values: `ac` for the HTTP-like ack code, `av` for ack version of the property, and an optional `ad` for an ack description.

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

#### **Direct Method (Command)**

One device command is supported in this sample: `getMaxMinReport`.

If any other commands are attempted to be invoked, the log will report the command is not found. To invoke a command, select your device's *Direct Method* tab in the Azure Portal. Enter the command name `getMaxMinReport` along with a payload using an ISO8061 time format and select **Invoke method**. A sample payload is given below:

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
