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
    Bramka Bluetooth do MQTT oparta na urządzeniu ESP32-WROOM.
    <br />
    <br />
    <a href="https://github.com/blizniukp/esp32_ble2mqtt/issues">Zgłoś błąd</a>
    ·
    <a href="https://github.com/blizniukp/esp32_ble2mqtt/issues">Zaproponuj nową funkcjonalność</a>
  </p>
</div>


*Read this in other language: [English](README.md), [Polski](README.pl.md).*


<details>
  <summary>Spis treści</summary>
  <ol>
    <li><a href="#o-projekcie">O projekcie</a></li>
    <li><a href="#sprzęt">Sprzęt</a></li>
    <li><a href="#wgrywanie-firmware-do-esp32">Wgrywanie firmware do ESP32</a></li>
    <li><a href="#konfiguracja-urządzenia">Konfiguracja urządzenia</a></li>
    <li><a href="#komunikacja-po-mqtt">Komunikacja po Mqtt</a></li>
    <li><a href="#plan-rozwoju">Plan rozwoju</a></li>
    <li><a href="#licencja">Licencja</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## O projekcie


Prosta bramka pozwalająca na komunikację z maksymalnie 4 urządzeniami Bluetooth jednocześnie.

Jest ona wykorzystywana razem z wtyczką do HomeAssistant [ha_addon_ble2mqtt](https://github.com/blizniukp/ha_addon_ble2mqtt). Wtyczka ta umożliwia ponadto konfigurację bramki.


Aplikacja bazuje na projekcie: [gattc_multi_connect](https://github.com/espressif/esp-idf/tree/22c82a4e28ec331a3f46e0a8f757f6b535f83cc4/examples/bluetooth/bluedroid/ble/gattc_multi_connect)

Aktualnie urządzenie obsługuje do 4 połączeń bluetooth jednocześnie. W celu zwięszenia ilości należy edytować zmienną CONFIG_BT_ACL_CONNECTIONS (parametr BT/BLE MAX ACL CONNECTIONS w menuconfig). 

**Wartości większe niż 4 nie były jeszcze testowane!**

<p align="right">(<a href="#top">powrót do góry</a>)</p>


## Sprzęt

W projekcie został wykorzystany moduł ESP32-WROOM-32.


Dodatkowo można zastosować przycisk oraz rezystor 10kOhm w celu resetowania konfiguracji.

W tym celu należy podłączyć szeregowo przycisk i rezystor pomiędzy zaciski 3v3 oraz pin D4.

Wbudowana dioda LED zapalana zostaje w momencie odebrania danych po Bluetooth, a gaszona po wysłaniu komunikatu MQTT.

<p align="right">(<a href="#top">powrót do góry</a>)</p>


## Wgrywanie firmware do ESP32

Firmware do urządzenia można wgrać za pomocą aplikacji [Esp Download Tool](https://www.espressif.com/en/support/download/other-tools)


Najnowsza wersja firmware jest dostępna w [Releases](https://github.com/blizniukp/esp32_ble2mqtt/releases)


W paczce z firmware powinniśmy mieć trzy pliki:
- bootloader.bin
- partitions.bin
- firmware.bin

Należy je wypakować do jakiegoś katalogu na dysku.

Po uruchomieniu aplikacji `ESP32 DOWNLOAD TOOL` należy wskazać te pliki oraz uzupełnić pola tak jak na zrzucie ekranu ekranu poniżej.

![esp32_download_tool](/img/esp32_download_tool.png)

Naciśnij przycisk `START` aby rozpocząć wgrywanie firmware do urządzenia.

<p align="right">(<a href="#top">powrót do góry</a>)</p>


## Konfiguracja urządzenia

W momencie pierwszego uruchomienia urządzenia (lub po resecie konfiguracji) tworzony jest hotspot WiFi o następującej nazwie i haśle:

Nazwa sieci: `ble2mqtt`

Hasło to: `12345678`

Po połączeniu z siecią należy przejść na stronę www umożliwiającą konfigurację.


Na systemie Linux wystarczy przejść na stronę: `http://ble2mqtt.local`

Na systemie Windows/Android należy użyć adresu: `http://192.168.4.1`


![ble2mqtt_webpage](/img/ble2mqtt_webpage.png)

Na powyższej stronie należy uzupełnić pola dotyczące konfiguracji WiFi oraz brokera Mqtt.

Gdy wszystkie pola zostaną uzupełnione kliknij na przycisk `Save`. Wprowadzone dane zostaną zapisane do pamięci nieulotnej modułu ESP32.


Po zapisie zrestartuj urządzenie. Jeżeli nie wystąpił żaden błąd, to urządzenie powinno połączyć się do sieci WiFi, a następnie do brokera Mqtt.


Konfigurację można zrestartować - tak aby ponownie uruchomiony był hotspot WiFi oraz strona z konfiguracją. 
Wykonuje się to poprzez uruchomienie urządzenia ze zwartym pinem D4 z zasilaniem 3,3V poprzez rezystor 10kOhm. Po uruchomieniu urządzenia należy rozewrzeć w/w połączenie.

<p align="right">(<a href="#top">powrót do góry</a>)</p>


## Komunikacja po Mqtt

Urządzenie subskrybuje topic: `/ble2mqtt/dev/#`.


Po podłaczeniu do brokera Mqtt urządzenie publikuje poniższy komunikat i oczekuje na odpowiedź.

topic: `/ble2mqtt/app/getDevList`

payload: `{}`

Poniżej przykład odpowiedzi zawierającej listę dwóch urządzeń Bluetooth:


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


W momencie odebrania danych po Bluetooth są one publikowane w topic `/ble2mqtt/app/value`

W payload znajduje się między innymi adres MAC urządzenia oraz odczytana wartość.


Przykład payload:

    {
        "address": "DA:D5:10:DC:D0:3A", 
        "is_notify": true, 
        "val_len": 8, 
        "val": "0101000000000000"
    }


## Plan rozwoju

- [x] Dodanie diody LED do sygnalizowania stanu
- [ ] Użycie TLS przy komunikacji z brokerem Mqtt
- [ ] Przesyłanie danych do urządzeń po Bluetooth

<p align="right">(<a href="#top">powrót do góry</a>)</p>


## Licencja

Projekt jest udostępniony na licencji MIT. 

Zerknij do pliku [LICENSE](LICENSE) aby poznać szczegóły.

<p align="right">(<a href="#top">powrót do góry</a>)</p>


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