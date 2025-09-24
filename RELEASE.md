# Azure C SDK port library

## What's Included?

See [README.md](./README.md) for a complete description of the Azure Port Library.

## Known issues
| Problem | Workaround |
| ------- | ---------- |
| SAS token-based authentication does not work with the device provisioning service (DPS) sample application (*mqtt_iot_provisioning_sample.c* file). |
None. There is a possible issue either in the Azure C SDK or the procedure for generating SAS token is unclear from the Azure documentation. This is currently being discussed in the Azure C SDK GitHub on the issues tab. Link : https://github.com/Azure/azure-sdk-for-c/issues/1677 |
| IAR 9.30 toolchain throws build errors on Debug mode, if application explicitly includes iar_dlmalloc.h file | Add '--advance-heap' to LDFLAGS in application Makefile. |

## Changelog

### v1.6.0

- Added support for PSOC&trade; Edge E84 (PSE84) platform
- Enabled LLVM_ARM toolchain support

### v1.5.0

- Added support for CY8CEVAL-062S2-CYW955513SDM2WLIPA kit

### v1.4.0

- Added support for CY8CEVAL-062S2-CYW43022CUB kit

### v1.3.0

- Removed deps folder
- Added support for KIT_XMC72_EVK_MUR_43439M2 kit

### v1.2.1

- Added support for CM0P core.
- Minor Documentation Updates.

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

| Software and tools                                              | Version |
| :---                                                            | :----:  |
| ModusToolbox&trade; software environment                        | 3.6     |
| ModusToolbox&trade; Device Configurator                         | 5.50    |
| GCC Compiler                                                    | 14.2.1  |
| IAR Compiler                                                    | 9.50.2  |
| Arm&reg; Compiler 6                                             | 6.22    |
| LLVM compiler                                                   | 19.1.5  |
