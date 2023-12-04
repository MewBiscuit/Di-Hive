| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# ESP-IDF-Interface

## Table of Contents
1. [Overview](#overview)
2. [Components](#components)
   - [ADC Manager](#adc-manager)
   - [MQTT Manager](#mqtt-manager)
   - [NVS Manager](#nvs-manager)
   - [Wifi Manager](#wifi-manager)
     - [Access Point Mode](#access-point-mode)
     - [Station Mode](#station-mode)
3. [Installation Instructions](#installation-instructions)
4. [Usage Instructions](#usage-instructions)
5. [Dependencies](#dependencies)
6. [License](#license)
7. [Contribution](#contribution)

## Overview
The ESP-IDF-Interface repository is dedicated to developing components that simplify and provide various functionalities for ESP-IDF projects, aiming to reduce the ESP-IDF's overhead.

## Components

### ADC Manager
Handles analog-to-digital conversion. Functions include initialization (`adc_manager_init`) and configuration of ADC channels.

### MQTT Manager
Facilitates MQTT protocol usage. It includes essential MQTT functionalities encapsulated for easier integration.

### NVS Manager
Manages non-volatile storage (NVS). Provides functions for initializing NVS (`init_nvs`) and reading strings from NVS (`read_string_from_nvs`).

### Wifi Manager
Manages WiFi functionalities, including modes and configurations. It is divided into two sub-components:
- #### Access Point Mode
  Configures and manages the ESP as a WiFi access point.
- #### Station Mode
  Manages the ESP in Station mode, enabling it to connect to existing WiFi networks.

## Installation Instructions
Copy and paste the component files from this repository into your project's `components` folder.

## Usage Instructions
Include the header files of the desired component and call the provided functions as per your project's requirements.

## Dependencies
No external dependencies, apart from the ESP-IDF framework.

## License
The licensing terms for this repository are currently undetermined.

## Contribution
This project is currently closed for contributions. Feedback and suggestions are welcome.

