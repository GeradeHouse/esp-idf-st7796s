# ESP32-S3-WROOM-1 ESP32-S3-WROOM-1U Datasheet

# Module Overview

## Features

### CPU and On-Chip Memory

- ESP32-S3 series of SoCs embedded, Xtensa® dual-core 32-bit LX7 microprocessor (with single precision FPU), up to 240 MHz
- 384 KB ROM
- 512 KB SRAM
- 16 KB SRAM in RTC
- Up to 16 MB PSRAM

### Wi-Fi

- 802.11 b/g/n
- Bit rate: 802.11n up to 150 Mbps
- A-MPDU and A-MSDU aggregation
- 0.4 µs guard interval support
- Center frequency range of operating channel: 2412 ~ 2484 MHz

### Bluetooth

- Bluetooth LE: Bluetooth 5, Bluetooth mesh
- Speed: 125 Kbps, 500 Kbps, 1 Mbps, 2 Mbps
- Advertising extensions
- Multiple advertisement sets
- Channel selection algorithm #2
- Internal co-existence mechanism between Wi-Fi and Bluetooth to share the same antenna

### Peripherals

•	GPIO, SPI, LCD interface, Camera interface, UART, I2C, I2S, remote control, pulse counter,

LED PWM, full-speed USB 2.0 OTG, USB Serial/JTAG controller, MCPWM, SDIO host, GDMA, TWAI® controller (compatible with ISO 11898-1), ADC, touch sensor, temperature sensor, timers and watchdogs

### Integrated Components on Module

•	40 MHz crystal oscillator
•	Up to 16 MB Quad SPI flash

### Antenna Options
•	On-board PCB antenna (ESP32-S3-WROOM-1)
•	External antenna via a connector (ESP32-S3-WROOM-1U)

### Operating Conditions

•	Operating voltage/Power supply: 3.0 ~ 3.6 V
•	Operating ambient temperature:
– 65 °C version: –40 ~ 65 °C
– 85 °C version: –40 ~ 85 °C
– 105 °C version: –40 ~ 105 °C

### Certification

•	RF certification: See certificates
•	Green certification: RoHS/REACH

### Test

•	HTOL/HTSL/uHAST/TCT/ESD 


## Description
ESP32-S3-WROOM-1 and ESP32-S3-WROOM-1U are two powerful, generic Wi-Fi + Bluetooth LE MCU modules that are built around the ESP32-S3 series of SoCs. On top of a rich set of peripherals, the acceleration for neural network computing and signal processing workloads provided by the SoC make the modules an ideal choice for a wide variety of application scenarios related to AI and Artificial Intelligence of Things (AIoT), such as wake word detection, speech commands recognition, face detection and recognition, smart home, smart appliances, smart control panel, smart speaker, etc.
ESP32-S3-WROOM-1 comes with a PCB antenna. ESP32-S3-WROOM-1U comes with an external antenna connector. A wide selection of module variants are available for customers as shown in Table 1 and 2. Among them, H4 series modules operate at –40 ~ 105 °C ambient temperature, R8 and R16V series modules operate at –40 ~ 65 °C ambient temperature, and other module variants operate at –40 ~ 85 °C ambient temperature. For R8 and R16V series modules with Octal SPI PSRAM, if the PSRAM ECC function is enabled, the maximum ambient temperature can be improved to 85 °C, while the usable size of PSRAM will be reduced by 1/16.



### Table 1: ESP32-S3-WROOM-1 Series Comparison

<table>
  <tr>
    <th>Ordering Code</th>
    <th>Flash<sup>3, 4</sup></th>
    <th>PSRAM<sup>5</sup></th>
    <th>Ambient Temp.<sup>6</sup> (°C)</th>
    <th>Size<sup>7</sup> (mm)</th>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N4</td>
    <td>4 MB (Quad SPI)</td>
    <td>-</td>
    <td>–40 ~ 85</td>
    <td rowspan="11">18.0 × 25.5 × 3.1</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N8</td>
    <td>8 MB (Quad SPI)</td>
    <td>-</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N16</td>
    <td>16 MB (Quad SPI)</td>
    <td>-</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-H4</td>
    <td>4 MB (Quad SPI)</td>
    <td>-</td>
    <td>–40 ~ 105</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N4R2</td>
    <td>4 MB (Quad SPI)</td>
    <td>2 MB (Quad SPI)</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N8R2</td>
    <td>8 MB (Quad SPI)</td>
    <td>2 MB (Quad SPI)</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N16R2</td>
    <td>16 MB (Quad SPI)</td>
    <td>2 MB (Quad SPI)</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N4R8</td>
    <td>4 MB (Quad SPI)</td>
    <td>8 MB (Octal SPI)</td>
    <td>–40 ~ 65</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N8R8</td>
    <td>8 MB (Quad SPI)</td>
    <td>8 MB (Octal SPI)</td>
    <td>–40 ~ 65</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N16R8</td>
    <td>16 MB (Quad SPI)</td>
    <td>8 MB (Octal SPI)</td>
    <td>–40 ~ 65</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1-N16R16V<sup>8</sup></td>
    <td>16 MB (Quad SPI)</td>
    <td>16 MB (Octal SPI)</td>
    <td>–40 ~ 65</td>
  </tr>
</table>

<sup>1</sup> This table shares the same notes presented in Table 2 below.


---

### Table 2: ESP32-S3-WROOM-1U Series Comparison

<table>
  <tr>
    <th>Ordering Code</th>
    <th>Flash<sup>3, 4</sup></th>
    <th>PSRAM<sup>5</sup></th>
    <th>Ambient Temp.<sup>6</sup> (°C)</th>
    <th>Size<sup>7</sup> (mm)</th>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N4</td>
    <td>4 MB (Quad SPI)</td>
    <td>-</td>
    <td>–40 ~ 85</td>
    <td rowspan="11">18.0 × 19.2 × 3.2</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N8</td>
    <td>8 MB (Quad SPI)</td>
    <td>-</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N16</td>
    <td>16 MB (Quad SPI)</td>
    <td>-</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-H4</td>
    <td>4 MB (Quad SPI)</td>
    <td>-</td>
    <td>–40 ~ 105</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N4R2</td>
    <td>4 MB (Quad SPI)</td>
    <td>2 MB (Quad SPI)</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N8R2</td>
    <td>8 MB (Quad SPI)</td>
    <td>2 MB (Quad SPI)</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N16R2</td>
    <td>16 MB (Quad SPI)</td>
    <td>2 MB (Quad SPI)</td>
    <td>–40 ~ 85</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N4R8</td>
    <td>4 MB (Quad SPI)</td>
    <td>8 MB (Octal SPI)</td>
    <td>–40 ~ 65</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N8R8</td>
    <td>8 MB (Quad SPI)</td>
    <td>8 MB (Octal SPI)</td>
    <td>–40 ~ 65</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N16R8</td>
    <td>16 MB (Quad SPI)</td>
    <td>8 MB (Octal SPI)</td>
    <td>–40 ~ 65</td>
  </tr>
  <tr>
    <td>ESP32-S3-WROOM-1U-N16R16V<sup>8</sup></td>
    <td>16 MB (Quad SPI)</td>
    <td>16 MB (Octal SPI)</td>
    <td>–40 ~ 65</td>
  </tr>
</table>


### Notes for table 1 & 2:

<sup>2</sup> For customization of ESP32-S3-WROOM-1-H4, ESP32-S3-WROOM-1U-H4, and ESP32-S3-WROOM-1U-N16R16V, please [contact us](mailto:contact@example.com).  
<sup>3</sup> By default, the SPI flash on the module operates at a maximum clock frequency of 80 MHz and does not support the auto suspend feature. If you require a higher flash clock frequency of 120 MHz or need the flash auto suspend feature, please [contact us](mailto:contact@example.com).  
<sup>4</sup> The integrated flash supports:  
- More than 100,000 program/erase cycles  
- More than 20 years of data retention time  
<sup>5</sup> The modules use PSRAM integrated in the chip’s package.  
<sup>6</sup> Ambient temperature specifies the recommended temperature range of the environment immediately outside the Espressif module.  
<sup>7</sup> For details, refer to Section 7.1 "Physical Dimensions".  
<sup>8</sup> Please note that the VDD_SPI voltage is 1.8 V for ESP32-S3-WROOM-1-N16R16V and ESP32-S3-WROOM-1U-N16R16V only.

---

At the core of the modules is an ESP32-S3 series of SoC *, an Xtensa® 32-bit LX7 CPU that operates at up to 240 MHz. You can power off the CPU and make use of the low-power co-processor to constantly monitor the peripherals for changes or crossing of thresholds.
ESP32-S3 integrates a rich set of peripherals including SPI, LCD, Camera interface, UART, I2C, I2S, remote control, pulse counter, LED PWM, USB Serial/JTAG controller, MCPWM, SDIO host, GDMA, TWAI® controller (compatible with ISO 11898-1), ADC, touch sensor, temperature sensor, timers and watchdogs, as well as up to 45 GPIOs. It also includes a full-speed USB 2.0 On-The-Go (OTG) interface to enable USB
communication.

## Applications

- Generic Low-power IoT Sensor Hub
- Generic Low-power IoT Data Loggers
- Cameras for Video Streaming
- Over-the-top (OTT) Devices
- USB Devices
- Speech Recognition
- Image Recognition
- Mesh Network
- Home Automation
 
# Pin Definitions

## Pin Layout

The pin diagram below shows the approximate location of pins on the module. For the actual diagram drawn to scale, please refer to Figure 7.1 Physical Dimensions.

The pin diagram is applicable for ESP32-S3-WROOM-1 and ESP32-S3-WROOM-1U, but the latter has no keepout zone.

<div style="display: flex; justify-content: center; align-items: center;">
  <table style="border-collapse: collapse; border: 1px solid black;">
    <tr>
      <td colspan="12" style="text-align: center; border: 1px solid black; padding: 10px;">Keepout Zone</td>
    </tr>
    <tr>
      <td style="text-align: left;">GND</td>
      <td style="text-align: center;"><a href="#pin1"><sup>1</sup></a></td>
      <td colspan="5" rowspan="14" style="text-align: center; padding: 80px;"></td>
      <td style="text-align: right;"><a href="#pin40"><sup>40</sup></a></td>
      <td style="text-align: right;">GND</td>
    </tr>
    <tr>
      <td style="text-align: left;">3V3</td>
      <td style="text-align: center;"><a href="#pin2"><sup>2</sup></a></td>
      <td style="text-align: right;"><a href="#pin39"><sup>39</sup></a></td>
      <td style="text-align: right;">IO1</td>
    </tr>
    <tr>
      <td style="text-align: left;">EN</td>
      <td style="text-align: center;"><a href="#pin3"><sup>3</sup></a></td>
      <td style="text-align: right;"><a href="#pin38"><sup>38</sup></a></td>
      <td style="text-align: right;">IO2</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO4</td>
      <td style="text-align: center;"><a href="#pin4"><sup>4</sup></a></td>
      <td style="text-align: right;"><a href="#pin37"><sup>37</sup></a></td>
      <td style="text-align: right;">TXD0</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO5</td>
      <td style="text-align: center;"><a href="#pin5"><sup>5</sup></a></td>
      <td style="text-align: right;"><a href="#pin36"><sup>36</sup></a></td>
      <td style="text-align: right;">RXD0</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO6</td>
      <td style="text-align: center;"><a href="#pin6"><sup>6</sup></a></td>
      <td style="text-align: right;"><a href="#pin35"><sup>35</sup></a></td>
      <td style="text-align: right;">IO42</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO7</td>
      <td style="text-align: center;"><a href="#pin7"><sup>7</sup></a></td>
      <td style="text-align: right;"><a href="#pin34"><sup>34</sup></a></td>
      <td style="text-align: right;">IO41</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO15</td>
      <td style="text-align: center;"><a href="#pin8"><sup>8</sup></a></td>
      <td style="text-align: right;"><a href="#pin33"><sup>33</sup></a></td>
      <td style="text-align: right;">IO40</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO16</td>
      <td style="text-align: center;"><a href="#pin9"><sup>9</sup></a></td>
      <td style="text-align: right;"><a href="#pin32"><sup>32</sup></a></td>
      <td style="text-align: right;">IO39</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO17</td>
      <td style="text-align: center;"><a href="#pin10"><sup>10</sup></a></td>
      <td style="text-align: right;"><a href="#pin31"><sup>31</sup></a></td>
      <td style="text-align: right;">IO38</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO18</td>
      <td style="text-align: center;"><a href="#pin11"><sup>11</sup></a></td>
      <td style="text-align: right;"><a href="#pin30"><sup>30</sup></a></td>
      <td style="text-align: right;">IO37</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO8</td>
      <td style="text-align: center;"><a href="#pin12"><sup>12</sup></a></td>
      <td style="text-align: right;"><a href="#pin29"><sup>29</sup></a></td>
      <td style="text-align: right;">IO36</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO19</td>
      <td style="text-align: center;"><a href="#pin13"><sup>13</sup></a></td>
      <td style="text-align: right;"><a href="#pin28"><sup>28</sup></a></td>
      <td style="text-align: right;">IO35</td>
    </tr>
    <tr>
      <td style="text-align: left;">IO20</td>
      <td style="text-align: center;"><a href="#pin14"><sup>14</sup></a></td>
      <td style="text-align: right;"><a href="#pin27"><sup>27</sup></a></td>
      <td style="text-align: right;">IO0</td>
    </tr>
    <tr>
      <td style="text-align: center;" colspan="12">
        <table style="border: none;">
          <tr>
            <td><a href="#pin15"><sup>15</sup></a></td>
            <td><a href="#pin16"><sup>16</sup></a></td>
            <td><a href="#pin17"><sup>17</sup></a></td>
            <td><a href="#pin18"><sup>18</sup></a></td>
            <td><a href="#pin19"><sup>19</sup></a></td>
            <td><a href="#pin20"><sup>20</sup></a></td>
            <td><a href="#pin21"><sup>21</sup></a></td>
            <td><a href="#pin22"><sup>22</sup></a></td>
            <td><a href="#pin23"><sup>23</sup></a></td>
            <td><a href="#pin24"><sup>24</sup></a></td>
            <td><a href="#pin25"><sup>25</sup></a></td>
            <td><a href="#pin26"><sup>26</sup></a></td>
          </tr>
          <tr>
            <td>IO3</td>
            <td>IO46</td>
            <td>IO9</td>
            <td>IO10</td>
            <td>IO11</td>
            <td>IO12</td>
            <td>IO13</td>
            <td>IO14</td>
            <td>IO21</td>
            <td>IO47</td>
            <td>IO45</td>
            <td>IO48</td>
          </tr>
        </table>
      </td>
    </tr>
  </table>
</div>

Figure 3: Pin Layout (Top View)



 
## Pin Description
The module has 41 pins. See pin definitions in Table 3 Pin Definitions.
For explanations of pin names and function names, as well as configurations of peripheral pins, please refer to the ESP32-S3 Series Datasheet.

### Table 3: Pin Definitions

| Name   | No. | Type a | Function |
|--------|-----|--------|----------------------------------------------------------|
| GND    | <a href="#pin1"><sup>1</sup></a>   | P      | GND |
| 3V3    | <a href="#pin2"><sup>2</sup></a>   | P      | Power supply |
| EN     | <a href="#pin3"><sup>3</sup></a>   | I      | High: on, enables the chip. Low: off, the chip powers off. **Note**: Do not leave the EN pin floating. |
| IO4    | <a href="#pin4"><sup>4</sup></a>   | I/O/T  | RTC_GPIO4, GPIO4, TOUCH4, ADC1_CH3 |
| IO5    | <a href="#pin5"><sup>5</sup></a>   | I/O/T  | RTC_GPIO5, GPIO5, TOUCH5, ADC1_CH4 |
| IO6    | <a href="#pin6"><sup>6</sup></a>   | I/O/T  | RTC_GPIO6, GPIO6, TOUCH6, ADC1_CH5 |
| IO7    | <a href="#pin7"><sup>7</sup></a>   | I/O/T  | RTC_GPIO7, GPIO7, TOUCH7, ADC1_CH6 |
| IO15   | <a href="#pin8"><sup>8</sup></a>   | I/O/T  | RTC_GPIO15, GPIO15, U0RTS, ADC2_CH4, XTAL_32K_P |
| IO16   | <a href="#pin9"><sup>9</sup></a>   | I/O/T  | RTC_GPIO16, GPIO16, U0CTS, ADC2_CH5, XTAL_32K_N |
| IO17   | <a href="#pin10"><sup>10</sup></a> | I/O/T  | RTC_GPIO17, GPIO17, U1TXD, ADC2_CH6 |
| IO18   | <a href="#pin11"><sup>11</sup></a> | I/O/T  | RTC_GPIO18, GPIO18, U1RXD, ADC2_CH7, CLK_OUT3 |
| IO8    | <a href="#pin12"><sup>12</sup></a> | I/O/T  | RTC_GPIO8, GPIO8, TOUCH8, ADC1_CH7, SUBSPICS1 |
| IO19   | <a href="#pin13"><sup>13</sup></a> | I/O/T  | RTC_GPIO19, GPIO19, U1RTS, ADC2_CH8, CLK_OUT2, USB_D- |
| IO20   | <a href="#pin14"><sup>14</sup></a> | I/O/T  | RTC_GPIO20, GPIO20, U1CTS, ADC2_CH9, CLK_OUT1, USB_D+ |
| IO3    | <a href="#pin15"><sup>15</sup></a> | I/O/T  | RTC_GPIO3, GPIO3, TOUCH3, ADC1_CH2 |
| IO46   | <a href="#pin16"><sup>16</sup></a> | I/O/T  | GPIO46 |
| IO9    | <a href="#pin17"><sup>17</sup></a> | I/O/T  | RTC_GPIO9, GPIO9, TOUCH9, ADC1_CH8, FSPIHD, SUBSPIHD |
| IO10   | <a href="#pin18"><sup>18</sup></a> | I/O/T  | RTC_GPIO10, GPIO10, TOUCH10, ADC1_CH9, FSPICS0, FSPIIO4, SUBSPICS0 |
| IO11   | <a href="#pin19"><sup>19</sup></a> | I/O/T  | RTC_GPIO11, GPIO11, TOUCH11, ADC2_CH0, FSPID, FSPIIO5, SUBSPID |
| IO12   | <a href="#pin20"><sup>20</sup></a> | I/O/T  | RTC_GPIO12, GPIO12, TOUCH12, ADC2_CH1, FSPICLK, FSPIIO6, SUBSPICLK |
| IO13   | <a href="#pin21"><sup>21</sup></a> | I/O/T  | RTC_GPIO13, GPIO13, TOUCH13, ADC2_CH2, FSPIQ, FSPIIO7, SUBSPIQ |
| IO14   | <a href="#pin22"><sup>22</sup></a> | I/O/T  | RTC_GPIO14, GPIO14, TOUCH14, ADC2_CH3, FSPIWP, FSPIDQS, SUBSPIWP |
| IO21   | <a href="#pin23"><sup>23</sup></a> | I/O/T  | RTC_GPIO21, GPIO21 |
| IO47   | <a href="#pin24"><sup>24</sup></a> | I/O/T  | SPICLK_P_DIFF, GPIO47, SUBSPICLK_P_DIFF |
| IO48   | <a href="#pin25"><sup>25</sup></a> | I/O/T  | SPICLK_N_DIFF, GPIO48, SUBSPICLK_N_DIFF |
| IO45   | <a href="#pin26"><sup>26</sup></a> | I/O/T  | GPIO45 |
| IO0    | <a href="#pin27"><sup>27</sup></a> | I/O/T  | RTC_GPIO0, GPIO0 |
| IO35   | <a href="#pin28"><sup>28</sup></a> | I/O/T  | SPIIO6, GPIO35, FSPID, SUBSPID |
| IO36   | <a href="#pin29"><sup>29</sup></a> | I/O/T  | SPIIO7, GPIO36, FSPICLK, SUBSPICLK |
| IO37   | <a href="#pin30"><sup>30</sup></a> | I/O/T  | SPIDQS, GPIO37, FSPIQ, SUBSPIQ |
| IO38   | <a href="#pin31"><sup>31</sup></a> | I/O/T  | GPIO38, FSPIWP, SUBSPIWP |
| IO39   | <a href="#pin32"><sup>32</sup></a> | I/O/T  | MTCK, GPIO39, CLK_OUT3, SUBSPICS1 |
| IO40   | <a href="#pin33"><sup>33</sup></a> | I/O/T  | MTDO, GPIO40, CLK_OUT2 |
| IO41   | <a href="#pin34"><sup>34</sup></a> | I/O/T  | MTDI, GPIO41, CLK_OUT1 |
| IO42   | <a href="#pin35"><sup>35</sup></a> | I/O/T  | MTMS, GPIO42 |
| RXD0   | <a href="#pin36"><sup>36</sup></a> | I/O/T  | U0RXD, GPIO44, CLK_OUT2 |
| TXD0   | <a href="#pin37"><sup>37</sup></a> | I/O/T  | U0TXD, GPIO43, CLK_OUT1 |
| IO2    | <a href="#pin38"><sup>38</sup></a> | I/O/T  | RTC_GPIO2, GPIO2, TOUCH2, ADC1_CH1 |
| IO1    | <a href="#pin39"><sup>39</sup></a> | I/O/T  | RTC_GPIO1, GPIO1, TOUCH1, ADC1_CH0 |
| GND    | <a href="#pin40"><sup>40</sup></a> | P      | GND |
| EPAD   | 41  | P      | GND |

**Notes:**
- **a** P: power supply; I: input; O: output; T: high impedance. Pin functions in bold font are the default pin functions. For pin 28–30, the default function is decided by eFuse bit.
- **b** For modules with Octal SPI PSRAM, i.e., modules embedded with ESP32-S3R8 or ESP32-S3R16V, pins IO35, IO36, and IO37 are connected to the Octal SPI PSRAM and are not available for other uses.
- **c** For modules embedded with ESP32-S3R16V, as the VDD_SPI voltage of the ESP32-S3R16V chip is set to 1.8 V, the working voltage for GPIO47 and GPIO48 is also 1.8 V, which is different from other GPIOs.

## LOLIN S3 PRO Pin Mapping

The following table lists the available pins on the LOLIN S3 PRO board and maps them to the corresponding ESP32-S3-WROOM-1 / ESP32-S3-WROOM-1U pins. The ESP32-S3-WROOM-1 / ESP32-S3-WROOM-1U modules are used in this board.

| LOLIN S3 PRO Pin | ESP32-S3 Pin | Description/Functionality |
|------------------|--------------|---------------------------|
| `LED_BUILTIN`    | <a href="#pin31"><sup>31</sup></a> (GPIO38) | Built-in LED |
| `TX`             | <a href="#pin37"><sup>37</sup></a> (GPIO43) | UART TX |
| `RX`             | <a href="#pin36"><sup>36</sup></a> (GPIO44) | UART RX |
| `SDA`            | <a href="#pin17"><sup>17</sup></a> (GPIO9)  | I2C Data |
| `SCL`            | <a href="#pin18"><sup>18</sup></a> (GPIO10) | I2C Clock |
| `SS`             | <a href="#pin27"><sup>27</sup></a> (GPIO0)  | SPI Slave Select (SS) |
| `MOSI`           | <a href="#pin19"><sup>19</sup></a> (GPIO11) | SPI MOSI |
| `MISO`           | <a href="#pin21"><sup>21</sup></a> (GPIO13) | SPI MISO |
| `SCK`            | <a href="#pin20"><sup>20</sup></a> (GPIO12) | SPI Clock |
| `TF_CS`          | <a href="#pin24"><sup>24</sup></a> (GPIO46) | TF Card Chip Select |
| `TS_CS`          | <a href="#pin26"><sup>26</sup></a> (GPIO45) | Touch Screen Chip Select |
| `TFT_CS`         | <a href="#pin25"><sup>25</sup></a> (GPIO48) | TFT Display Chip Select |
| `TFT_DC`         | <a href="#pin24"><sup>24</sup></a> (GPIO47) | TFT Display Data/Command Select |
| `TFT_RST`        | <a href="#pin23"><sup>23</sup></a> (GPIO21) | TFT Display Reset |
| `TFT_LED`        | <a href="#pin22"><sup>22</sup></a> (GPIO14) | TFT Display LED Control |
| `A0`             | <a href="#pin39"><sup>39</sup></a> (GPIO1)  | ADC Channel 0 |
| `A1`             | <a href="#pin32"><sup>32</sup></a> (GPIO2)  | ADC Channel 1 |
| `A2`             | <a href="#pin33"><sup>33</sup></a> (GPIO3)  | ADC Channel 2 |
| `A3`             | <a href="#pin4"><sup>4</sup></a> (GPIO4)    | ADC Channel 3 |
| `A4`             | <a href="#pin5"><sup>5</sup></a> (GPIO5)    | ADC Channel 4 |
| `A5`             | <a href="#pin6"><sup>6</sup></a> (GPIO6)    | ADC Channel 5 |
| `A6`             | <a href="#pin7"><sup>7</sup></a> (GPIO7)    | ADC Channel 6 |
| `A7`             | <a href="#pin8"><sup>8</sup></a> (GPIO15)   | ADC Channel 7 |
| `A8`             | <a href="#pin17"><sup>17</sup></a> (GPIO9)  | ADC Channel 8 |
| `A9`             | <a href="#pin18"><sup>18</sup></a> (GPIO10) | ADC Channel 9 |
| `A10`            | <a href="#pin19"><sup>19</sup></a> (GPIO11) | ADC Channel 10 |
| `A11`            | <a href="#pin20"><sup>20</sup></a> (GPIO12) | ADC Channel 11 |
| `A12`            | <a href="#pin21"><sup>21</sup></a> (GPIO13) | ADC Channel 12 |
| `A13`            | <a href="#pin22"><sup>22</sup></a> (GPIO14) | ADC Channel 13 |
| `A14`            | <a href="#pin15"><sup>15</sup></a> (GPIO3)  | ADC Channel 14 |
| `A15`            | <a href="#pin16"><sup>16</sup></a> (GPIO46) | ADC Channel 15 |
| `A16`            | <a href="#pin9"><sup>9</sup></a> (GPIO16)   | ADC Channel 16 |
| `A17`            | <a href="#pin10"><sup>10</sup></a> (GPIO17) | ADC Channel 17 |

### Touch Pins (T-Function)
The LOLIN S3 PRO also supports touch-sensitive GPIO pins:

| LOLIN S3 PRO Pin | ESP32-S3 Pin | Touch Sensor Functionality |
|------------------|--------------|----------------------------|
| `T1`             | <a href="#pin39"><sup>39</sup></a> (GPIO1)  | Touch Pad 1 |
| `T2`             | <a href="#pin32"><sup>32</sup></a> (GPIO2)  | Touch Pad 2 |
| `T3`             | <a href="#pin33"><sup>33</sup></a> (GPIO3)  | Touch Pad 3 |
| `T4`             | <a href="#pin4"><sup>4</sup></a> (GPIO4)    | Touch Pad 4 |
| `T5`             | <a href="#pin5"><sup>5</sup></a> (GPIO5)    | Touch Pad 5 |
| `T6`             | <a href="#pin6"><sup>6</sup></a> (GPIO6)    | Touch Pad 6 |
| `T7`             | <a href="#pin7"><sup>7</sup></a> (GPIO7)    | Touch Pad 7 |
| `T8`             | <a href="#pin8"><sup>8</sup></a> (GPIO15)   | Touch Pad 8 |
| `T9`             | <a href="#pin17"><sup>17</sup></a> (GPIO9)  | Touch Pad 9 |
| `T10`            | <a href="#pin18"><sup>18</sup></a> (GPIO10) | Touch Pad 10 |
| `T11`            | <a href="#pin19"><sup>19</sup></a> (GPIO11) | Touch Pad 11 |
| `T12`            | <a href="#pin20"><sup>20</sup></a> (GPIO12) | Touch Pad 12 |
| `T13`            | <a href="#pin21"><sup>21</sup></a> (GPIO13) | Touch Pad 13 |
| `T14`            | <a href="#pin22"><sup>22</sup></a> (GPIO14) | Touch Pad 14 |

### Notes:
- `LED_BUILTIN` (GPIO38) is connected to the onboard LED.
- `TFT_CS`, `TFT_DC`, `TFT_RST`, and `TFT_LED` are used for controlling the TFT display.
- `A0` to `A17` represent the analog pins that map to the respective GPIOs with ADC capabilities.
- `GPIO19` and `GPIO20` are not available as external pins as they are internally connected to the USB-C port:
  - `GPIO19` `(IO19)` is connected to `USB_D-` (USB Data Negative)
  - `GPIO20` `(IO20)` is connected to `USB_D+` (USB Data Positive)
  - 
This mapping provides easy access to connect and control the GPIOs on the LOLIN S3 PRO board based on the underlying ESP32-S3-WROOM-1/1U module.

 
## Strapping Pins

At each startup or reset, a module requires some initial configuration parameters, such as in which boot mode to load the module, voltage of flash memory, etc. These parameters are passed over via the strapping pins. After reset, the strapping pins operate as regular IO pins.
The parameters controlled by the given strapping pins at module reset are as follows:
•	Chip boot mode – GPIO0 and GPIO46
•	VDD_SPI voltage – GPIO45
•	ROM messages printing – GPIO46
•	JTAG signal source – GPIO3
GPIO0, GPIO45, and GPIO46 are connected to the chip’s internal weak pull-up/pull-down resistors at chip reset. These resistors determine the default bit values of the strapping pins. Also, these resistors determine the bit values if the strapping pins are connected to an external high-impedance circuit.
Table 4: Default Configuration of Strapping Pins

Strapping Pin	Default Configuration	Bit Value
GPIO0	Pull-up	1
GPIO3	Floating	–
GPIO45	Pull-down	0
GPIO46	Pull-down	0

To change the bit values, the strapping pins should be connected to external pull-down/pull-up resistances. If the ESP32-S3 is used as a device by a host MCU, the strapping pin voltage levels can also be controlled by the host MCU.
All strapping pins have latches. At system reset, the latches sample the bit values of their respective strapping pins and store them until the chip is powered down or shut down. The states of latches cannot be changed in any other way. It makes the strapping pin values available during the entire chip operation, and the pins are freed up to be used as regular IO pins after reset.
Regarding the timing requirements for the strapping pins, there are such parameters as setup time and hold time. For more information, see Table 5 and Figure 4.
Table 5: Description of Timing Parameters for the Strapping Pins

Parameter	Description	Min (ms)

tSU	Setup time is the time reserved for the power rails to stabilize before
the CHIP_PU pin is pulled high to activate the chip.	0

tH	Hold time is the time reserved for the chip to read the strapping pin values after CHIP_PU is already high and before these pins start
operating as regular IO pins.	
3

Figure 4: Visualization of Timing Parameters for the Strapping Pins

### Chip Boot Mode Control
GPIO0 and GPIO46 control the boot mode after the reset is released. See Table 6 Chip Boot Mode Control.
Table 6: Chip Boot Mode Control

Boot Mode	GPIO0	GPIO46
Default Configuration	1 (Pull-up)	0 (Pull-down)
SPI Boot (default)	1	Any value
Joint Download Boot1	0	0
1 Joint Download Boot mode supports the following download methods:
•	USB Download Boot:
–	USB-Serial-JTAG Download Boot
–	USB-OTG Download Boot
•	UART Download Boot

In SPI Boot mode, the ROM bootloader loads and executes the program from SPI flash to boot the system.
In Joint Download Boot mode, users can download binary files into flash using UART0 or USB interface. It is also possible to download binary files into SRAM and execute it from SRAM.
In addition to SPI Boot and Joint Download Boot modes, ESP32-S3 also supports SPI Download Boot mode. For details, please see ESP32-S3 Technical Reference Manual > Chapter Chip Boot Control.

### VDD_SPI Voltage Control
Depending on the value of EFUSE_VDD_SPI_FORCE, the voltage can be controlled in two ways.

Table 7: VDD_SPI Voltage Control

EFUSE_VDD_SPI_FORCE	GPIO45	eFuse 1	Voltage	VDD_SPI power source 2
0	0	Ignored	3.3 V	VDD3P3_RTC via RSP I
	1		1.8 V	Flash Voltage Regulator
1	Ignored	0	1.8 V	Flash Voltage Regulator
		1	3.3 V	VDD3P3_RTC via RSP I
1 eFuse: EFUSE_VDD_SPI_TIEH
2 See ESP32-S3 Series Datasheet > Section Power Scheme

### ROM Messages Printing Control
During boot process the messages by the ROM code can be printed to:
•	(Default) UART and USB Serial/JTAG controller.
•	USB Serial/JTAG controller.
•	UART.
The ROM messages printing to UART or USB Serial/JTAG controller can be respectively disabled by configuring registers and eFuse. For detailed information, please refer to ESP32-S3 Technical Reference Manual > Chapter Chip Boot Control.

### JTAG Signal Source Control
The strapping pin GPIO3 can be used to control the source of JTAG signals during the early boot process. This pin does not have any internal pull resistors and the strapping value must be controlled by the external circuit that cannot be in a high impedance state.
As Table 8 shows, GPIO3 is used in combination with EFUSE_DIS_PAD_JTAG, EFUSE_DIS_USB_JTAG, and EFUSE_STRAP_JTAG_SEL.
Table 8: JTAG Signal Source Control

eFuse 1a	eFuse 2b	eFuse 3c	GPIO3	JTAG Signal Source

0	
0	0	Ignored	USB Serial/JTAG Controller
		1	0	JTAG pins MTDI, MTCK, MTMS, and MTDO
			1	USB Serial/JTAG Controller
0	1	Ignored	Ignored	JTAG pins MTDI, MTCK, MTMS, and MTDO
1	0	Ignored	Ignored	USB Serial/JTAG Controller
1	1	Ignored	Ignored	JTAG is disabled
a eFuse 1: EFUSE_DIS_PAD_JTAG
b eFuse 2: EFUSE_DIS_USB_JTAG
c eFuse 3: EFUSE_STRAP_JTAG_SEL
# Electrical Characteristics
## Absolute Maximum Ratings
Stresses above those listed in Table 9 Absolute Maximum Ratings may cause permanent damage to the device. These are stress ratings only and functional operation of the device at these or any other conditions beyond those indicated under Table 10 Recommended Operating Conditions is not implied. Exposure to
absolute-maximum-rated conditions for extended periods may affect device reliability.

Table 9: Absolute Maximum Ratings

Symbol	Parameter	Min	Max	Unit
VDD33	Power supply voltage	–0.3	3.6	V
TSTORE	Storage temperature	–40	105	°C

## Recommended Operating Conditions

Table 10: Recommended Operating Conditions

Symbol	Parameter	Min	Typ	Max	Unit
VDD33	Power supply voltage	3.0	3.3	3.6	V
IV DD	Current delivered by external power supply	0.5	—	—	A

TA	
Operating ambient temperature	65 °C version	
–40	
—	65	
°C
		85 °C version			85	
		105 °C version			105	

## DC Characteristics (3.3 V, 25 °C)

Table 11: DC Characteristics (3.3 V, 25 °C)

Symbol	Parameter	Min	Typ	Max	Unit
CIN	Pin capacitance	—	2	—	pF
VIH	High-level input voltage	0.75 × VDD1	—	VDD1+ 0.3	V
VIL	Low-level input voltage	–0.3	—	0.25 × VDD1	V
IIH	High-level input current	—	—	50	nA
IIL	Low-level input current	—	—	50	nA
VOH 2	High-level output voltage	0.8 × VDD1	—	—	V
VOL2	Low-level output voltage	—	—	0.1 × VDD1	V

IOH	High-level source current (VDD1= 3.3 V, VOH >=
2.64 V, PAD_DRIVER = 3)	—	40	—	mA

IOL	Low-level sink current (VDD1= 3.3 V, VOL =
0.495 V, PAD_DRIVER = 3)	—	28	—	mA
RPU	Internal weak pull-up resistor	—	45	—	kΩ
RPD	Internal weak pull-down resistor	—	45	—	kΩ

Table 11 – cont’d from previous page
Symbol	Parameter	Min	Typ	Max	Unit

VIH_nRST	Chip reset release voltage (EN voltage is within
the specified range)	0.75 × VDD1	—	VDD1+ 0.3	V

VIL_nRST	Chip reset voltage (EN voltage is within the
specified range)	–0.3	—	0.25 × VDD1	V
1 VDD is the I/O voltage for pins of a particular power domain.
2 VOH and VOL are measured using high-impedance load.

## Current Consumption Characteristics
### RF Current Consumption in Active Mode
With the use of advanced power-management technologies, the module can switch between different power modes. For details on different power modes, please refer to Section Low Power Management
in ESP32-S3 Series Datasheet.

Table 12: Current Consumption Depending on RF Modes

Work mode	Description	Peak (mA)

Active (RF working)	

TX	802.11b, 1 Mbps, @20.5 dBm	355
		802.11g, 54 Mbps, @18 dBm	297
		802.11n, HT20, MCS 7, @17.5 dBm	286
		802.11n, HT40, MCS 7, @17 dBm	285
	RX	802.11b/g/n, HT20	95
		802.11n, HT40	97
1 The current consumption measurements are taken with a 3.3 V supply at 25 °C of ambient temperature at the RF port. All transmitters’ measurements are based on a 100% duty cycle. 2 The current consumption figures for in RX mode are for cases when the peripherals are dis-
abled and the CPU idle.
Note:
The content below is excerpted from Section Power Consumption in Other Modes in ESP32-S3 Series Datasheet.

### Current Consumption in Other Modes
Please note that if the chip embedded has in-package PSRAM, the current consumption of the module might be higher compared to the measurements below.
 
Table 13: Current Consumption in Modem-sleep Mode

Work mode	Frequency
(MHz)	
Description	Typ1
(mA)	Typ2
(mA)

Modem-sleep3	

40	WAITI (Dual core in idle state)	13.2	18.8
		Single core running 32-bit data access instructions, the
other core in idle state	16.2	21.8
		Dual core running 32-bit data access instructions	18.7	24.4
		Single core running 128-bit data access instructions, the
other core in idle state	19.9	25.4
		Dual core running 128-bit data access instructions	23.0	28.8

80	WAITI	22.0	36.1
		Single core running 32-bit data access instructions, the
other core in idle state	28.4	42.6
		Dual core running 32-bit data access instructions	33.1	47.3
		Single core running 128-bit data access instructions, the
other core in idle state	35.1	49.6
		Dual core running 128-bit data access instructions	41.8	56.3

160	WAITI	27.6	42.3
		Single core running 32-bit data access instructions, the
other core in idle state	39.9	54.6
		Dual core running 32-bit data access instructions	49.6	64.1
		Single core running 128-bit data access instructions, the
other core in idle state	54.4	69.2
		Dual core running 128-bit data access instructions	66.7	81.1

240	WAITI	32.9	47.6
		Single core running 32-bit data access instructions, the
other core in idle state	51.2	65.9
		Dual core running 32-bit data access instructions	66.2	81.3
		Single core running 128-bit data access instructions, the
other core in idle state	72.4	87.9
		Dual core running 128-bit data access instructions	91.7	107.9
1 Current consumption when all peripheral clocks are disabled.
2 Current consumption when all peripheral clocks are enabled. In practice, the current consumption might be different depending on which peripherals are enabled.
3 In Modem-sleep mode, Wi-Fi is clock gated, and the current consumption might be higher when accessing flash. For a flash rated at 80 Mbit/s, in SPI 2-line mode the consumption is 10 mA.
Table 14: Current Consumption in Low-Power Modes

Work mode	Description	Typ (µA)
Light-sleep1	VDD_SPI and Wi-Fi are powered down, and all GPIOs
are high-impedance.	240
Deep-sleep	RTC memory and RTC peripherals are powered up.	8
	RTC memory is powered up. RTC peripherals are
powered down.	7

Power off	CHIP_PU is set to low level. The chip is shut down.	1
1 In Light-sleep mode, all related SPI pins are pulled up. For chips embedded with PSRAM, please add corresponding PSRAM consumption values, e.g., 140 µA for 8 MB Octal PSRAM (3.3 V), 200 µA for 8 MB Octal PSRAM (1.8 V) and 40 µA for 2 MB Quad PSRAM (3.3 V).

## Wi-Fi RF Characteristics
### Wi-Fi RF Standards

Table 15: Wi-Fi RF Standards

Name	Description
Center frequency range of operating channel 1	2412 ~ 2484 MHz
Wi-Fi wireless standard	IEEE 802.11b/g/n

Data rate	
20 MHz	11b: 1, 2, 5.5 and 11 Mbps
11g: 6, 9, 12, 18, 24, 36, 48, 54 Mbps
11n: MCS0-7, 72.2 Mbps (Max)
	40 MHz	11n: MCS0-7, 150 Mbps (Max)
Antenna type	PCB antenna, external antenna connector 2
1 Device should operate in the center frequency range allocated by regional regulatory authorities. Target center frequency range is configurable by software.
2 For the modules that use external antenna connectors, the output impedance is 50 Ω. For other modules
without external antenna connectors, the output impedance is irrelevant.

### Wi-Fi RF Transmitter (TX) Specifications
Target TX power is configurable based on device or certification requirements. The default characteristics are provided in Table 16 TX Power with Spectral Mask and EVM Meeting 802.11 Standards.
Table 16: TX Power with Spectral Mask and EVM Meeting 802.11 Standards

Rate	Min
(dBm)	Typ
(dBm)	Max
(dBm)
802.11b, 1 Mbps	—	20.5	—
802.11b, 11 Mbps	—	20.5	—
802.11g, 6 Mbps	—	20.0	—
802.11g, 54 Mbps	—	18.0	—
802.11n, HT20, MCS 0	—	19.0	—
802.11n, HT20, MCS 7	—	17.5	—
802.11n, HT40, MCS 0	—	18.5	—
802.11n, HT40, MCS 7	—	17.0	—
 
Table 17: TX EVM Test

Rate	Min
(dB)	Typ
(dB)	SL1
(dB)
802.11b, 1 Mbps, @20.5 dBm	—	–24.5	–10
802.11b, 11 Mbps, @20.5 dBm	—	–24.5	–10
802.11g, 6 Mbps, @20 dBm	—	–23.0	–5
802.11g, 54 Mbps, @18 dBm	—	–29.5	–25
802.11n, HT20, MCS 0, @19 dBm	—	–24.0	–5
802.11n, HT20, MCS 7, @17.5 dBm	—	–30.5	–27
802.11n, HT40, MCS 0, @18.5 dBm	—	–25.0	–5
802.11n, HT40, MCS 7, @17 dBm	—	–30.0	–27
1 SL stands for standard limit value.

### Wi-Fi RF Receiver (RX) Specifications

Table 18: RX Sensitivity

Rate	Min
(dBm)	Typ
(dBm)	Max
(dBm)
802.11b, 1 Mbps	—	–98.2	—
802.11b, 2 Mbps	—	–95.6	—
802.11b, 5.5 Mbps	—	–92.8	—
802.11b, 11 Mbps	—	–88.5	—
802.11g, 6 Mbps	—	–93.0	—
802.11g, 9 Mbps	—	–92.0	—
802.11g, 12 Mbps	—	–90.8	—
802.11g, 18 Mbps	—	–88.5	—
802.11g, 24 Mbps	—	–85.5	—
802.11g, 36 Mbps	—	–82.2	—
802.11g, 48 Mbps	—	–78.0	—
802.11g, 54 Mbps	—	–76.2	—
802.11n, HT20, MCS 0	—	–93.0	—
802.11n, HT20, MCS 1	—	–90.6	—
802.11n, HT20, MCS 2	—	–88.4	—
802.11n, HT20, MCS 3	—	–84.8	—
802.11n, HT20, MCS 4	—	–81.6	—
802.11n, HT20, MCS 5	—	–77.4	—
802.11n, HT20, MCS 6	—	–75.6	—
802.11n, HT20, MCS 7	—	–74.2	—
802.11n, HT40, MCS 0	—	–90.0	—
802.11n, HT40, MCS 1	—	–87.5	—
802.11n, HT40, MCS 2	—	–85.0	—
802.11n, HT40, MCS 3	—	–82.0	—

Table 18 – cont’d from previous page
Rate	Min
(dBm)	Typ
(dBm)	Max
(dBm)
802.11n, HT40, MCS 4	—	–78.5	—
802.11n, HT40, MCS 5	—	–74.4	—
802.11n, HT40, MCS 6	—	–72.5	—
802.11n, HT40, MCS 7	—	–71.2	—

Table 19: Maximum RX Level

Rate	Min
(dBm)	Typ
(dBm)	Max
(dBm)
802.11b, 1 Mbps	—	5	—
802.11b, 11 Mbps	—	5	—
802.11g, 6 Mbps	—	5	—
802.11g, 54 Mbps	—	0	—
802.11n, HT20, MCS 0	—	5	—
802.11n, HT20, MCS 7	—	0	—
802.11n, HT40, MCS 0	—	5	—
802.11n, HT40, MCS 7	—	0	—

Table 20: RX Adjacent Channel Rejection

Rate	Min
(dB)	Typ
(dB)	Max
(dB)
802.11b, 1 Mbps	—	35	—
802.11b, 11 Mbps	—	35	—
802.11g, 6 Mbps	—	31	—
802.11g, 54 Mbps	—	14	—
802.11n, HT20, MCS 0	—	31	—
802.11n, HT20, MCS 7	—	13	—
802.11n, HT40, MCS 0	—	19	—
802.11n, HT40, MCS 7	—	8	—

## Bluetooth LE Radio

Table 21: Bluetooth LE Frequency

Parameter	Min
(MHz)	Typ
(MHz)	Max
(MHz)
Center frequency of operating channel	2402	—	2480
 
### Bluetooth LE RF Transmitter (TX) Specifications

Table 22: Transmitter Characteristics - Bluetooth LE 1 Mbps

Parameter	Description	Min	Typ	Max	Unit
RF transmit power	RF power control range	–24.00	0	20.00	dBm
	Gain control step	—	3.00	—	dB

Carrier frequency offset and drift	Max |fn|n=0, 1, 2, ..k	—	2.50	—	kHz
	Max |f0 − fn|	—	2.00	—	kHz
	Max |fn − fn−5|	—	1.40	—	kHz
	|f1 − f0|	—	1.00	—	kHz

Modulation characteristics	∆ f 1avg	—	249.00	—	kHz
	Min ∆ f 2max (for at least
99.9% of all ∆ f 2max)	—	198.00	—	kHz
	∆ f 2avg/∆ f 1avg	—	0.86	—	—

In-band spurious emissions	±2 MHz offset	—	–37.00	—	dBm
	±3 MHz offset	—	–42.00	—	dBm
	>±3 MHz offset	—	–44.00	—	dBm

Table 23: Transmitter Characteristics - Bluetooth LE 2 Mbps

Parameter	Description	Min	Typ	Max	Unit
RF transmit power	RF power control range	–24.00	0	20.00	dBm
	Gain control step	—	3.00	—	dB

Carrier frequency offset and drift	Max |fn|n=0, 1, 2, ..k	—	2.50	—	kHz
	Max |f0 − fn|	—	2.00	—	kHz
	Max |fn − fn−5|	—	1.40	—	kHz
	|f1 − f0|	—	1.00	—	kHz

Modulation characteristics	∆ f 1avg	—	499.00	—	kHz
	Min ∆ f 2max (for at least
99.9% of all ∆ f 2max)	—	416.00	—	kHz
	∆ f 2avg/∆ f 1avg	—	0.89	—	—

In-band spurious emissions	±4 MHz offset	—	–42.00	—	dBm
	±5 MHz offset	—	–44.00	—	dBm
	>±5 MHz offset	—	–47.00	—	dBm

Table 24: Transmitter Characteristics - Bluetooth LE 125 Kbps

Parameter	Description	Min	Typ	Max	Unit
RF transmit power	RF power control range	–24.00	0	20.00	dBm
	Gain control step	—	3.00	—	dB

Carrier frequency offset and drift	Max |fn|n=0, 1, 2, ..k	—	0.80	—	kHz
	Max |f0 − fn|	—	1.00	—	kHz
	|fn − fn−3|	—	0.30	—	kHz

Table 24 – cont’d from previous page
Parameter	Description	Min	Typ	Max	Unit
	|f0 − f3|	—	1.00	—	kHz

Modulation characteristics	∆ f 1avg	—	248.00	—	kHz
	Min ∆ f 1max (for at least
99.9% of all∆ f 1max)	—	222.00	—	kHz

In-band spurious emissions	±2 MHz offset	—	–37.00	—	dBm
	±3 MHz offset	—	–42.00	—	dBm
	>±3 MHz offset	—	–44.00	—	dBm

Table 25: Transmitter Characteristics - Bluetooth LE 500 Kbps

Parameter	Description	Min	Typ	Max	Unit
RF transmit power	RF power control range	–24.00	0	20.00	dBm
	Gain control step	—	3.00	—	dB

Carrier frequency offset and drift	Max |fn|n=0, 1, 2, ..k	—	0.80	—	kHz
	Max |f0 − fn|	—	1.00	—	kHz
	|fn − fn−3|	—	0.85	—	kHz
	|f0 − f3|	—	0.34	—	kHz

Modulation characteristics	∆ f 2avg	—	213.00	—	kHz
	Min ∆ f 2max (for at least
99.9% of all ∆ f 2max)	—	196.00	—	kHz

In-band spurious emissions	±2 MHz offset	—	–37.00	—	dBm
	±3 MHz offset	—	–42.00	—	dBm
	>±3 MHz offset	—	–44.00	—	dBm

### Bluetooth LE RF Receiver (RX) Specifications

Table 26: Receiver Characteristics - Bluetooth LE 1 Mbps

Parameter	Description	Min	Typ	Max	Unit
Sensitivity @30.8% PER	—	—	–96.5	—	dBm
Maximum received signal @30.8% PER	—	—	8	—	dBm
Co-channel C/I	F = F0 MHz	—	8	—	dB

Adjacent channel selectivity C/I	F = F0 + 1 MHz	—	4	—	dB
	F = F0 – 1 MHz	—	4	—	dB
	F = F0 + 2 MHz	—	–23	—	dB
	F = F0 – 2 MHz	—	–23	—	dB
	F = F0 + 3 MHz	—	–34	—	dB
	F = F0 – 3 MHz	—	–34	—	dB
	F > F0 + 3 MHz	—	–36	—	dB
	F > F0 – 3 MHz	—	–37	—	dB
Image frequency	—	—	–36	—	dB
Adjacent channel to image frequency	F = Fimage + 1 MHz	—	–39	—	dB
	F = Fimage – 1 MHz	—	–34	—	dB

Out-of-band blocking performance	30 MHz ~ 2000 MHz	—	–12	—	dBm
	2003 MHz ~ 2399 MHz	—	–18	—	dBm
	2484 MHz ~ 2997 MHz	—	–16	—	dBm
	3000 MHz ~ 12.75 GHz	—	–10	—	dBm
Intermodulation	—	—	–29	—	dBm

Table 27: Receiver Characteristics - Bluetooth LE 2 Mbps

Parameter	Description	Min	Typ	Max	Unit
Sensitivity @30.8% PER	—	—	–92	—	dBm
Maximum received signal @30.8% PER	—	—	3	—	dBm
Co-channel C/I	F = F0 MHz	—	8	—	dB

Adjacent channel selectivity C/I	F = F0 + 2 MHz	—	4	—	dB
	F = F0 – 2 MHz	—	4	—	dB
	F = F0 + 4 MHz	—	–27	—	dB
	F = F0 – 4 MHz	—	–27	—	dB
	F = F0 + 6 MHz	—	–38	—	dB
	F = F0 – 6 MHz	—	–38	—	dB
	F > F0 + 6 MHz	—	–41	—	dB
	F > F0 – 6 MHz	—	–41	—	dB
Image frequency	—	—	–27	—	dB
Adjacent channel to image frequency	F = Fimage + 2 MHz	—	–38	—	dB
	F = Fimage – 2 MHz	—	4	—	dB

Out-of-band blocking performance	30 MHz ~ 2000 MHz	—	–15	—	dBm
	2003 MHz ~ 2399 MHz	—	–21	—	dBm
	2484 MHz ~ 2997 MHz	—	–21	—	dBm
	3000 MHz ~ 12.75 GHz	—	–9	—	dBm
Intermodulation	—	—	–29	—	dBm

Table 28: Receiver Characteristics - Bluetooth LE 125 Kbps

Parameter	Description	Min	Typ	Max	Unit
Sensitivity @30.8% PER	—	—	–103.5	—	dBm
Maximum received signal @30.8% PER	—	—	8	—	dBm
Co-channel C/I	F = F0 MHz	—	4	—	dB

Adjacent channel selectivity C/I	F = F0 + 1 MHz	—	1	—	dB
	F = F0 – 1 MHz	—	2	—	dB
	F = F0 + 2 MHz	—	–26	—	dB
	F = F0 – 2 MHz	—	–26	—	dB
	F = F0 + 3 MHz	—	–36	—	dB
	F = F0 – 3 MHz	—	–39	—	dB
	F > F0 + 3 MHz	—	–42	—	dB
	F > F0 – 3 MHz	—	–43	—	dB
Image frequency	—	—	–42	—	dB
Adjacent channel to image frequency	F = Fimage + 1 MHz	—	–43	—	dB
	F = Fimage – 1 MHz	—	–36	—	dB
 
Table 29: Receiver Characteristics - Bluetooth LE 500 Kbps

Parameter	Description	Min	Typ	Max	Unit
Sensitivity @30.8% PER	—	—	–100	—	dBm
Maximum received signal @30.8% PER	—	—	8	—	dBm
Co-channel C/I	F = F0 MHz	—	4	—	dB

Adjacent channel selectivity C/I	F = F0 + 1 MHz	—	1	—	dB
	F = F0 – 1 MHz	—	0	—	dB
	F = F0 + 2 MHz	—	–24	—	dB
	F = F0 – 2 MHz	—	–24	—	dB
	F = F0 + 3 MHz	—	–37	—	dB
	F = F0 – 3 MHz	—	–39	—	dB
	F > F0 + 3 MHz	—	–38	—	dB
	F > F0 – 3 MHz	—	–42	—	dB
Image frequency	—	—	–38	—	dB
Adjacent channel to image frequency	F = Fimage + 1 MHz	—	–42	—	dB
	F = Fimage – 1 MHz	—	–37	—	dB
 
# Peripheral Schematics
This is the typical application circuit of the module connected with peripheral components (for example, power supply, antenna, reset button, JTAG interface, and UART interface).

•	Soldering the EPAD to the ground of the base board is not a must, however, it can optimize thermal performance. If you choose to solder it, please apply the correct amount of soldering paste. Too much soldering paste may increase the gap between the module and the baseboard. As result, the adhesion between other pins and the baseboard may be poor.
•	To ensure that the power supply to the ESP32-S3 chip is stable during power-up, it is advised to add an RC delay circuit at the EN pin. The recommended setting for the RC delay circuit is usually R = 10 kΩ and C = 1 µF. However, specific parameters should be adjusted based on the power-up timing of the module and the power-up and reset sequence timing of the chip. For ESP32-S3’s power-up and reset sequence timing diagram, please refer to ESP32-S3 Series Datasheet > Section Power Supply.