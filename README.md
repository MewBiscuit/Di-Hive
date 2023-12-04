| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# ESP-IDF-Interface

## Table of Contents
1. [Overview](#overview)
2. [Current Components](#current-components)
3. [Installation Instructions](#installation-instructions)
4. [Usage Instructions](#usage-instructions)
5. [Features](#features)
6. [Dependencies](#dependencies)
7. [License](#license)
8. [Contribution](#contribution)

## Overview
The ESP-IDF-Interface repository is dedicated to developing components that streamline and offer various functionalities for ESP-IDF projects. This repository aims to reduce the overhead associated with ESP-IDF, making it more accessible and easier to use for developers.

### Current Components
- **Wifi Manager**: Enables ESP to function as a station or access point.
- **MQTT Manager**: Simplifies the use of MQTT protocols.
- **ADC Manager**: Assists in analog-to-digital conversion management.
- **NVS Manager**: Aids in non-volatile storage handling.

## Installation Instructions
1. Locate the component you need within this repository.
2. Copy and paste the folder containing the chosen component into your project's `components` folder.

## Usage Instructions
To use a component:
1. Include the header files of the desired component in your project.
2. Call the functions implemented in the component as per your project's needs.

## Features
- **Simplified ESP-IDF Usage**: This repository aims to significantly reduce the amount of work required by programmers by handling much of the ESP-IDF's complexity.
- **Modular Components**: Each component is designed to be independently integrated into your projects, providing specific functionalities as needed. Some of these components might depend on one another, so make sure you check the REQUIRES field of the component you wish to utilize

## Dependencies
There are no external dependencies for this repository, apart from the ESP-IDF framework itself.

## License
The licensing terms for this repository are currently undetermined. Please check back later for updates regarding the license.

## Contribution
This project is currently closed for contributions. However, any feedback or suggestions are always welcome. Please feel free to reach out for discussions or queries related to this project.


