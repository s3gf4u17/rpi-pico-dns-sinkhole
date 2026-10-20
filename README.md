# Raspberry Pi Pico W DNS Sinkhole

A DNS sinkhole built for the **Raspberry Pi Pico W** using C, the Raspberry Pi Pico SDK, and lwIP.

The project aims to provide a standalone DNS-based filtering solution on an inexpensive microcontroller.

## Overview

A DNS sinkhole intercepts DNS requests and prevents selected domain names from resolving to their normal destinations. It can be used to block unwanted network traffic before a device establishes a connection to the destination.

Typical use cases include:

- **Ad and tracker blocking** - block domains associated with advertising and tracking.
- **Malicious domain blocking** - prevent DNS resolution for known malware and phishing domains.
- **Network-wide filtering** - provide DNS filtering for devices configured to use the sinkhole.
- **Embedded networking** - experiment with DNS protocols and network services on resource-constrained hardware.

Unlike browser extensions or applications installed on individual devices, DNS-based filtering can work at the network level. Devices must use the sinkhole for their DNS requests to be filtered. DNS filtering alone does not replace a firewall or other security measures.

## Features

Current functionality includes:

- Wi-Fi connectivity using the Pico W's built-in wireless interface.
- DNS request handling over UDP using lwIP.
- Domain-blocking functionality.
- Persistent Wi-Fi configuration stored in Flash memory.

See the TODO list for planned improvements and the features in development.

## Building

Make sure the Raspberry Pi Pico SDK and its build dependencies are installed and configured. Build the project:

```bash
make compile-project
```

This generates firmware artifacts, including a UF2 file that can be loaded onto the Pico W in USB mass storage bootloader mode.

## Network configuration

The firmware reads Wi-Fi credentials from a dedicated region of Flash memory. Create a file named `.env` in the root folder:

```dotenv
ssid=YOUR_WIFI_SSID
password=YOUR_WIFI_PASSWORD
```

Replace the placeholder values with the credentials of the Wi-Fi network the Pico W should connect to.

The configuration must contain both values:

- `ssid` - Wi-Fi network name, up to 32 bytes.
- `password` - Wi-Fi password, up to 64 bytes.

Write the configuration to the Pico W using the provisioning script.

```bash
./config/loader.sh
```

On startup, the firmware reads the saved configuration from Flash and attempts to connect to the configured Wi-Fi network.

## Flash layout

The configuration image occupies one 4096-byte sector beginning at offset `0x101FF000`.

The binary layout is:

| Field | Size |
|---|---:|
| Magic number (`0x5049434F`) | 4 bytes |
| SSID | 32 bytes |
| Password | 64 bytes |
| Padding (`0xFF`) | 3996 bytes |
| **Total** | **4096 bytes** |

The magic number is stored in little-endian byte order. The SSID and password are stored in fixed-size, null-padded fields.

## Testing

Test results and development notes are maintained in the repository.

Relevant areas for testing include:

- Wi-Fi initialization and connection handling.
- DNS request parsing and response generation.
- Correct handling of allowed and blocked domains.
- Malformed or unsupported DNS requests.
- Flash configuration loading and validation.
- Repeated requests and resource usage on the Pico W.

Refer to the available test results for the behavior that has been verified so far.

## TODO

Planned improvements include:

- Support regex for domain blocking.