## An ESP32 based Light bar for the P1P

It uses MQTT to comunicate with the P1P and query the current state of the printer. It then animates an LED strip based on print progress and printer state.

### Why?

Because I wanted the lightbar on the H2D, without having to pay for a H2D :P

### Setup:

All secrets are stored in a user defined library ```Secrets.h```
To use this code, you will have to add the following:

```c++
#define WIFI_SSID "Your Wi-Fi Netowrk name - a string"
#define WIFI_PASS "Your Wi-Fi Password - a string"
#define P1P_IP "Your printer's IP (I suggest assigning it a static IP via your router's admin page) - a string"
#define P1P_MQTT_USERNAME "bblp" // Has to be "bblp"
#define P1P_MQTT_PASS "Your access code/number. You can get this from the pritner's display under the Wi-Fi section - a string"
#define P1P_MQTT_PORT 8883
#define P1P_SERIAL "Your Printer's serial number - I got this from BambuStudio Device tab - a string"
```

### Requirements:
The following libraries are required

MQTT:

![image](https://github.com/user-attachments/assets/0e1ac0aa-984d-4796-be48-661068b50eb4)

Arduino JSON:

![image](https://github.com/user-attachments/assets/fcf32635-d4e3-4d75-9684-3f519734e3af)

Adafruit NeoPixel:

![image](https://github.com/user-attachments/assets/b61870b0-32be-4521-8811-cf35b78bc4d7)

