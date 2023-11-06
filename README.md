| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# Wifi & Bluetooth credentials provisioning with persistence

The following project aims to provide a complete and easy to use Wi-Fi credentials provisioning tool through Wi-Fi and Bluetooth. The Wi-Fi credentials are stored in the NVS partition and are persistent across reboots. The Bluetooth provisioning is done through a simple file transfer and the Wifi version uses an HTTP web portal hosted on the ESP-32 as a softAP to enter the credentials. The project is based on the ESP-IDF framework and uses the ESP32 devboard.
