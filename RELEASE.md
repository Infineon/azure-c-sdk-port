# Azure C SDK port library

## What's Included?

See [README.md](./README.md) for a complete description of the Azure Port Library.

## Known issues
| Problem | Workaround |
| ------- | ---------- |
| SAS token-based authentication does not work with the device provisioning service (DPS) sample application (*mqtt_iot_provisioning_sample.c* file). |
None. There is a possible issue either in the Azure C SDK or the procedure for generating SAS token is unclear from the Azure documentation. This is currently being discussed in the Azure C SDK GitHub on the issues tab. Link : https://github.com/Azure/azure-sdk-for-c/issues/1677 |

## Changelog

### v1.2.0

- Added support for CY8CEVAL-062S2-MUR-43439M2 kit
- Removed Azure sample applications.

### v1.1.0

- Added support for "secure" kit (for example: CY8CKIT-064S0S2-4343W)

- Updated Azure sample applications to support secured kit (for example: CY8CKIT-064S0S2-4343W)

- Minor documentation changes to add support for CY8CEVAL-062S2-LAI-4373M2 kit

### v1.0.0

- Initial release for AnyCloud

* Port layer implementation for [Microsoft's Azure SDK for Embedded C library](https://github.com/Azure/azure-sdk-for-c/releases/tag/1.1.0)


## Supported software and tools

This version of the library was validated for compatibility with the following software and tools:

| Software and tools                                      | Version |
| :---                                                    | :----:  |
| ModusToolbox&trade; software environment                | 2.4     |
| - ModusToolbox&trade; device configurator               | 3.10    |
| - CAPSENSE&trade; configurator / tuner tools            | 4.0     |
| PSoC&trade; 6 peripheral driver library (PDL)           | 2.3.0   |
| GCC compiler                                            | 10.3.1  |
| IAR compiler                                            | 8.32    |
| Arm&reg; compiler 6                                     | 6.14    |
