# 🎵 IoT Metronome (Arduino UNO R4 WiFi + Node-RED)

This project transforms an **Arduino UNO R4 WiFi** into a smart, connected metronome. It can be controlled entirely via physical buttons on a breadboard or remotely through a convenient web interface (Dashboard) built with **Node-RED** over the MQTT protocol.

The system is designed to be **resilient**: it works seamlessly even **offline**. If the WiFi network drops or is unavailable, the metronome automatically enters a standalone mode, allowing you to keep playing your music without any interruptions.

---

## ✨ Main Features

* 📶 **Bidirectional Control:** Real-time synchronization between the physical hardware and the Node-RED Dashboard.
* 🥁 **Tap Tempo (Smart):** Automatically calculates BPM by tapping the tempo (minimum 4 taps required). Can be triggered via a physical button or the web dashboard.
* 📴 **Native Offline Mode:** Fully non-blocking code. If there's no WiFi at startup, the metronome boots offline and tries to reconnect in the background every 5 seconds without freezing the main loop.
* 🔊 **Audio Management (Mute):** The piezoelectric buzzer can be toggled on/off on the fly.
* 💡 **Rich Visual Feedback:**
  * **LCD 16x2:** Displays WiFi status, current mode (PLAY/REC), audio state, and the current BPM.
  * **LED Matrix (UNO R4):** Renders a musical note icon when the sound is active; turns blank when muted.
  * **External LEDs:** A Green LED blinks in sync with the BPM; a Red LED indicates when the REC/Learn mode is active.

---

## 🛠️ Hardware Requirements

* **Board:** Arduino UNO R4 WiFi
* **Display:** LCD 16x2 (standard parallel connection) with a potentiometer for contrast adjustment.
* **Audio:** 1x Piezoelectric Buzzer.
* **Visual Indicators:** 1x Green LED, 1x Red LED (with 220Ω resistors).
* **Controls:** 3x Tactile push buttons (for Mode, Tap, and Mute).

### 🔌 Wiring Schematic (Pinout)

| Component | Arduino Pin | Notes |
| :--- | :--- | :--- |
| **Piezo Buzzer** | `A0` | Connect the other pin to GND |
| **Sound Button (Mute)** | `2` | Connect the other pin to GND (Uses `INPUT_PULLUP`) |
| **Mode Button (Rec/Play)**| `4` | Connect the other pin to GND (Uses `INPUT_PULLUP`) |
| **Tap Tempo Button** | `5` | Connect the other pin to GND (Uses `INPUT_PULLUP`) |
| **Green LED (Beat)** | `8` | Via a 220Ω resistor to GND |
| **Red LED (REC Mode)** | `9` | Via a 220Ω resistor to GND |
| **LCD RS** | `12` | - |
| **LCD EN** | `11` | - |
| **LCD D4, D5, D6, D7** | `10, 7, 6, 3`| - |

*(Note on the LCD: ensure Pins 15 and 16 are powered for the backlight, and Pin 3 is connected to the center pin of a potentiometer for contrast control).*

---

## 💻 Software & Libraries

Make sure to install the following libraries via the Arduino IDE Library Manager:

1. `WiFiS3` (Included in the UNO R4 board core)
2. `PubSubClient` (For MQTT communication)
3. `LiquidCrystal` (For the 16x2 LCD display)
4. `Arduino_LED_Matrix` (Included in the UNO R4 board core, for the onboard LED matrix)

---

## 🌐 Node-RED / MQTT Configuration

The system connects to an MQTT broker (e.g., Mosquitto) and uses the following Topic structure to communicate with Node-RED:

### 📥 Topics Subscribed by Arduino (RX Commands)
* `metronome/bpm/set` : Receives a number (e.g., `120`) to set the playback BPM.
* `metronome/sound/set` : Receives `ON`, `OFF`, `TRUE`, `FALSE`, `1`, or `0` to toggle the buzzer.
* `metronome/state/set` : Receives any message (e.g., `TOGGLE`) to enter/exit the REC mode.
* `metronome/tap/set` : Receives any message to register a Tap Tempo beat (only processed if REC mode is active).

### 📤 Topics Published by Arduino (TX Telemetry)
* `metronome/bpm` : Sends the current BPM (useful for dashboard gauges).
* `metronome/bpm/min` & `metronome/bpm/max` : Sends BPM statistics.
* `metronome/sound/state` : Sends `ON` or `OFF` when the sound state changes physically (syncs the web switch).
* `metronome/state/rec` : Sends `ON` or `OFF` to indicate the Learn/REC mode status.

*Note: The Arduino code currently uses Italian topic strings (e.g., `metronomo/suono/set`). You can easily translate them in the `.ino` file and Node-RED nodes if you prefer an all-English setup.*

### 📦 Importing the Dashboard
If you exported the Node-RED flow as a `.json` file, you can easily restore it: go to **Menu (hamburger icon) > Import**, select your file, and deploy.

---

## 🚀 Usage Guide

1. **Power On:** Power the Arduino. The display will show the boot screen and try to connect to WiFi for 5 seconds. If successful, it connects to MQTT; otherwise, it boots into **Offline Mode** and is instantly ready to use.
2. **Setting the BPM:** Send a numeric value from the Node-RED Dashboard form or slider.
3. **Tap Tempo:** 
   * Press the physical **MODE** button (or the *REC/PLAY Mode* button on the Dashboard). The Red LED will turn on.
   * Press the physical **TAP** button (or the *Tap Tempo* button on the Dashboard) to the beat of the music for **at least 4 times**.
   * Press **MODE** again. The system will calculate the average interval, apply the new BPM, and start blinking/beeping.
4. **Mute:** Press the physical button connected to Pin 2, or toggle the Dashboard switch to silence the metronome (the visual green LED beat will continue uninterrupted).

![Schema dei collegamenti del Progetto 7](schema.jpg)
