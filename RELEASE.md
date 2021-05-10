# Azure C SDK Port Library

## What's Included?

See [README.md](./README.md) for a complete description of the Azure Port Library.

## Known Issues
| Problem | Workaround |
| ------- | ---------- |
| SAS token based authentication does not work with DPS (Device Provisioning Service) sample application (file mqtt_iot_provisioning_sample.c). |
None. There is a possible issue either in the Azure C SDK or the procedure for generating SAS token is unclear from the Azure documentation. This is currently being discussed in the Azure C SDK GitHub on the issues tab. Link : https://github.com/Azure/azure-sdk-for-c/issues/1677 |

## Changelog
### v1.0.0

- Initial release for AnyCloud.
- Port layer implementation for [Microsoft's Azure SDK for Embedded C Library](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0)

### Supported Software and Tools

This version of the library was validated for compatibility with the following software and tools:

| Software and Tools                                      | Version |
| :---                                                    | :----:  |
| ModusToolbox&trade; Software Environment                | 2.3     |
| - ModusToolbox Device Configurator                      | 3.0     |
| - ModusToolbox CapSense Configurator / Tuner tools      | 3.15    |
| PSoC&trade; 6 Peripheral Driver Library (PDL)           | 2.2.0   |
| GCC Compiler                                            | 9.3.1   |
| IAR Compiler                                            | 8.32    |
| Arm Compiler 6                                          | 6.14    |
