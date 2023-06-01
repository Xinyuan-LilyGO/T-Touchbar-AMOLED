<h1 align = "center"> 🌟T-Touchbar-AMOLED🌟</h1>

### Arduino

1. In Arduino Preferences, on the Settings tab, enter the `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` URL in the `Additional boards manager URLs` input box. 
2. Click OK and the software will install. 
3. Search for ESP32 in Tools → Board Manager and install ESP32-Arduino SDK (version 2.0.3 or above)
![](image/Arduino_board.png)
4. Copy everything from this repository lib folder to [Arduino library folder](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#manual-installation) (e.g. `C:\Users\YourName\Documents\Arduino\libraries`)
5. Select the correct settings in the Tools menu as shown below.

| Setting                              | Value                                   |
| :----------------------------------- | :-------------------------------------- |
| Board                                | ESP32S3 Dev Module                      |
| Upload Speed                         | 921600                                  |
| USB Mode                             | Hardware CDC and JTAG                   |
| USB CDC On Boot                      | Disabled                                |
| USB DFU On Boot                      | Disabled                                |
| CPU Frequency                        | 240MHz (WiFi)                           |
| Flash Mode                           | QIO 80MHz                               |
| Flash Size                           | 16MB (128Mb)                            |
| Partition Scheme                     | 16MB Flash (3MB APP/9.9MB FATFS)        |
| Core Debug Level                     | 无                                      |
| PSRAM                                | OPI PSRAM                               |
| Events Run On                        | Core 1                                  |
| Arduino Runs On                      | Core 1                                  |
| Erase All Flash Before Sketch Upload | Disabled                                |
| JTAG Adapter                         | Disabled                                |

### PlatformIO

1. PlatformIO plug-in installation: click on the extension on the left column → search platformIO → install the first plug-in
2. Click Platforms → Embedded → search Espressif 32 in the input box → select the corresponding firmware installation
