<div id="top"></div>

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]



<br />
<div align="center">

<h3 align="center">esp32_ble2mqtt</h3>

  <p align="center">
    Bluetooth to MQTT gateway based on ESP32-WROOM-32
    <br />
    <br />
    <a href="https://github.com/blizniukp/esp32_ble2mqtt/issues">Report Bug</a>
    ·
    <a href="https://github.com/blizniukp/esp32_ble2mqtt/issues">Request Feature</a>
  </p>
</div>


*Read this in other language: [English](README.md), [Polski](README.pl.md).*


<details>
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about-the-project">About The Project</a></li>
    <li><a href="#hardware">Hardware</a></li>
    <li><a href="#uploading-firmware-to-esp32">Uploading firmware to ESP32</a></li>
    <li><a href="#device-configuration">Device configuration</a></li>
    <li><a href="#mqtt-communication">Mqtt communication</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#license">License</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project


A simple gateway that allows you to communicate with up to 4 Bluetooth devices at the same time. 

It is used together with the plug-in for HomeAssistant [ha_addon_ble2mqtt](https://github.com/blizniukp/ha_addon_ble2mqtt).

This plug-in also allows you to configure the gateway.


The application is based on the project: [gattc_multi_connect](https://github.com/espressif/esp-idf/tree/22c82a4e28ec331a3f46e0a8f757f6b535f83cc4/examples/bluetooth/bluedroid/ble/gattc_multi_connect)


Currently device supports up to 4 bluetooth connections simultaneously. To increase it's number, edit variable CONFIG_BT_ACL_CONNECTIONS (BT/BLE MAX ACL CONNECTIONS parameter in menuconfig). 

**Values greater than 4 have not been tested yet!**

<p align="right">(<a href="#top">back to top</a>)</p>


## Hardware

The ESP32-WROOM-32 module was used in this project.


In addition, you can use a button and a 10KOHM resistor to reset the configuration.

To do this, connect the button and the resistor between the 3V3 and PIN D4 terminals.

<p align="right">(<a href="#top">powrót do góry</a>)</p>


## Uploading firmware to ESP32

The firmware can be uploaded using the application [Esp Download Tool](https://www.espressif.com/en/support/download/other-tools)


The latest firmware version is available in [Releases](https://github.com/blizniukp/esp32_ble2mqtt/releases)



We should have three files in the firmware package:
- bootloader.bin
- partitions.bin
- firmware.bin

You should extract them to some directory on your disk.

After running the `ESP32 DOWNLOAD TOOL` application, indicate these files and complete the fields as in the screenshot below.

![esp32_download_tool](/img/esp32_download_tool.png)

Press the `Start` button to start uploading the firmware to the device.

<p align="right">(<a href="#top">powrót do góry</a>)</p>


## Device configuration

When the device is first started (or after a configuration reset), WiFi hotspot is created with the following name and password:

SSID: `ble2mqtt`

Password: `12345678`


Once connected to the network, go to the configuration web page.


On the Linux system, just go to the page: `http://ble2mqtt.local`

On Windows/Android use the address: `http://192.168.4.1`


![ble2mqtt_webpage](/img/ble2mqtt_webpage.png)

On the above page, complete the fields for WiFi configuration and Mqtt broker.

When all fields are completed, click on the `Save` button. The entered data will be saved to the non-volatile memory of the ESP32 module.

After writing, restart the device. If no error occurs, the device should connect to the WiFi network and then to the MQTT broker.


Configuration can be restarted. Then WiFi hotspot starts again and will be able to enter the configuration page.

This is done by starting the device with connected D4 pin with 3.3V supply through 10kOhm resistor. After starting the device, open the above-mentioned connection.

<p align="right">(<a href="#top">powrót do góry</a>)</p>


## Mqtt communication

Device subscribes to topic: `/ble2mqtt/dev/#`.

After connecting to the MQTT broker, the device publishes the following message and waits for a response.

topic: `/ble2mqtt/app/getDevList`

payload: `{}`


Below is an example of a response containing a list of two Bluetooth devices:

topic: `/ble2mqtt/dev/devList`

payload:

    [
       {
          "name":"ROUND_PAD_1",
          "address":"DA:D5:10:DC:D0:3A",
          "address_type":"random",
          "service_uuid":"71F1BB00-1C43-5BAC-B24B-FDABEEC53913",
          "char_uuid":"71F1BB01-1C43-5BAC-B24B-FDABEEC53913"
       },
       {
          "name":"ROUND_PAD_2",
          "address":"DF:E2:FB:80:50:3E",
          "address_type":"random",
          "service_uuid":"71F1BB00-1C43-5BAC-B24B-FDABEEC53913",
          "char_uuid":"71F1BB01-1C43-5BAC-B24B-FDABEEC53913"
       }
    ]


When data is received over Bluetooth, it is published to the topic `/ble2mqtt/app/value`

The payload contains, among other things, the MAC address of the device and the value read.


Payload example:

    {
        "address": "DA:D5:10:DC:D0:3A", 
        "is_notify": true, 
        "val_len": 8, 
        "val": "0101000000000000"
    }


## Roadmap

- [ ] Adding a LED to indicate device status
- [ ] Using TLS when communicating with the MQTT broker
- [ ] Transferring data to Bluetooth devices

<p align="right">(<a href="#top">back to top</a>)</p>


## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.

<p align="right">(<a href="#top">back to top</a>)</p>


<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/blizniukp/esp32_ble2mqtt.svg?style=for-the-badge
[contributors-url]: https://github.com/blizniukp/esp32_ble2mqtt/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/blizniukp/esp32_ble2mqtt.svg?style=for-the-badge
[forks-url]: https://github.com/blizniukp/esp32_ble2mqtt/network/members
[stars-shield]: https://img.shields.io/github/stars/blizniukp/esp32_ble2mqtt.svg?style=for-the-badge
[stars-url]: https://github.com/blizniukp/esp32_ble2mqtt/stargazers
[issues-shield]: https://img.shields.io/github/issues/blizniukp/esp32_ble2mqtt.svg?style=for-the-badge
[issues-url]: https://github.com/blizniukp/esp32_ble2mqtt/issues
[license-shield]: https://img.shields.io/github/license/blizniukp/esp32_ble2mqtt.svg?style=for-the-badge
[license-url]: https://github.com/blizniukp/esp32_ble2mqtt/blob/master/LICENSE
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/paweł-bliźniuk-433535183
[product-screenshot]: images/screenshot.png