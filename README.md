| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# Wifi & Bluetooth credentials provisioning with persistence

The following project aims to provide a complete and easy to use Wi-Fi credentials provisioning tool through Wi-Fi and Bluetooth. The Wi-Fi credentials are stored in the NVS partition and are persistent across reboots. The Bluetooth provisioning is done through a simple file transfer and the Wifi version uses an HTTP web portal hosted on the ESP-32 as a softAP to enter the credentials. The project is based on the ESP-IDF framework and uses the ESP32 devboard.

## How to use

### Bluetooth provisioning

The Bluetooth provisioning is done through a simple file transfer. The file is a text file containing the Wi-Fi credentials. The file is sent over classic Bluetooth protocol for simplicity, but it shouldn't be too hard to implement a BLE credentials transport for the ESP-32.

### Wi-Fi provisioning

The Wi-Fi provisioning is done through a simple web portal hosted on the ESP-32 as a softAP. The ESP32 will start a TCP server on port 80 and will serve a simple web page. The web page will allow the user to enter the Wi-Fi credentials. The ESP32 will then parse the POST request and store the credentials in the NVS partition. The ESP32 will then connect to the Wi-Fi network using the credentials. The ESP32 will then start a TCP server on port 80 and will serve a simple web page. The web page will display the Wi-Fi credentials and will allow the user to change them, but this can be easily customizable. The ESP32 will then connect to the new Wi-Fi network.
