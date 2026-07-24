# 👁️ Naya-Dristi (EchoPath) — Real-Time Spatial Assistive Pipeline

[![Project Status: Active](https://img.shields.io/badge/Project--Status-Active-brightgreen.svg)](#)
[![Python Version](https://img.shields.io/badge/Python-3.10%2B-blue.svg)](https://www.python.org/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32%20%7C%20HC--SR04%20%7C%20Piezo-orange.svg)](#)
[![CV Model](https://img.shields.io/badge/AI-YOLOv8n-red.svg)](https://github.com/ultralytics/ultralytics)

**Naya-Dristi (EchoPath)** is an integrated AI vision and spatial ultrasonic navigation system designed to aid visually impaired individuals. By fusing real-time YOLOv8 computer vision object tracking with high-frequency ultrasonic rangefinding, Naya-Dristi detects physical obstacles and delivers directional audio cues (LEFT, CENTER, RIGHT) via an ESP32-driven piezo sounder while maintaining a real-time HUD interface.

---

## 🚀 Key Features

* **Multi-Modal Sensor Fusion**: Combines 2D visual boundary analysis (YOLOv8) with 1D precise distance metering (HC-SR04 ultrasonic) to minimize false positives.
* **Non-Blocking Firmware Engine**: ESP32 state-machine firmware delivers real-time acoustic pattern pulses without interrupting sensor polling or serial message parsing.
* **Directional Auditory Cues**: Distinct auditory frequencies and cadence patterns indicate object direction (Left: dual-short tone; Right: single long tone; Center: rapid triple tone).
* **Distance Hysteresis**: Dual-threshold logic (Enter: 50 cm, Exit: 60 cm) prevents rapid alert oscillation at obstacle boundaries.
* **Auto-Healing Serial Bridge**: Threaded background reader in Python automatically detects ESP32 USB-Serial chips (CH340/CP210x/FTDI) and reconnects seamlessly on link drop.
* **Real-Time HUD Overlay**: High-visibility heads-up display built with OpenCV and Supervision showing object class, distance telemetry, alert state, and connection status.

---

## 🛠️ Hardware & Component Specifications

| Component | Specification | Interface / Protocol | Function |
|---|---|---|---|
| **Microcontroller** | ESP32 NodeMCU / DevKit v1 | USB-UART / PWM (LEDC) | Ultrasonic polling, buzzer pattern execution |
| **Distance Sensor** | HC-SR04 Ultrasonic Sensor | Digital I/O (TRIG/ECHO) | 2 cm – 400 cm range measurement |
| **Actuator** | 3.3V/5V Passive Piezo Buzzer | PWM (GPIO 4) | Directional audio feedback generation |
| **Camera Input** | External USB Webcam or Laptop Cam | USB DirectShow / V4L2 | 720p @ 30 FPS video feed for object detection |
| **Host System** | Windows / Linux / macOS PC | USB Serial @ 115200 Baud | YOLOv8 inference, sensor fusion, HUD rendering |

---

## 📌 Wiring & Pinout Assignments

### ESP32 Pin Connections

| ESP32 Pin | Component Pin | Notes |
|---|---|---|
| **GPIO 5** | HC-SR04 `TRIG` | Digital Output (10 µs trigger pulse) |
| **GPIO 18** | HC-SR04 `ECHO` | Digital Input (Echo pulse width measurement) |
| **GPIO 4** | Piezo Buzzer `(+)` | LEDC PWM output (driven via transistor or direct) |
| **GND** | Sensor & Buzzer `GND` | Common Ground |
| **5V / 3.3V** | Sensor & Buzzer `VCC` | Power supply rail |

---

## 💻 Software & Firmware Dependencies

### Firmware (Arduino / PlatformIO)
* **Target Core**: ESP32 Arduino Core v2.x or v3.x (shim included for dual compatibility)
* **Libraries**: Standard ESP32 hardware drivers (`HardwareSerial`, `esp32-hal-ledc`)

### Software (Python Environment)
* Python 3.10 or higher
* Core dependencies listed in `requirements.txt`:
  * `ultralytics` (YOLOv8 engine)
  * `opencv-python` (Image processing & HUD)
  * `supervision` (Annotation pipeline)
  * `pyserial` (USB serial bridge)
  * `numpy` (Tensor manipulation)

---

## ⚙️ Setup & Installation Guide

### 1. Hardware Assembly
1. Connect HC-SR04 `TRIG` to ESP32 **GPIO 5** and `ECHO` to **GPIO 18**.
2. Connect the passive piezo buzzer positive leg to ESP32 **GPIO 4** and negative leg to **GND**.
3. Plug the ESP32 and USB webcam into the host computer.

### 2. Flashing ESP32 Firmware
1. Open `echopath_firmware.ino` in the Arduino IDE (or VS Code with Arduino extension).
2. Select target board: `ESP32 Dev Module`.
3. Choose the corresponding serial port and flash the firmware at **115200 Baud**.

### 3. Environment Setup & Execution
1. Clone the repository and navigate to the directory:
   ```bash
   cd herne/Naya-Dristi-main
   ```
2. Install Python dependencies:
   ```bash
   pip install -r requirements.txt
   ```
3. Run the pipeline launcher:
   * **PowerShell**:
     ```powershell
     .\run_main.ps1
     ```
   * **Direct Python**:
     ```bash
     python main.py
     ```

---

## 🔊 Directional Audio Protocol

| Command | Frequencies & Cadence | Meaning |
|---|---|---|
| `LEFT` | 2 short beeps @ 1500 Hz | Obstacle detected on the left side of frame |
| `RIGHT` | 1 long beep @ 2500 Hz | Obstacle detected on the right side of frame |
| `CENTER` | 3 rapid beeps @ 2000 Hz | Obstacle directly ahead (highest priority) |
| `NONE` | Silent | Path clear / object outside threshold |

---

## 👥 Team & Hackathon Acknowledgment

This project was built during a 1-day **Team Hackathon** on **July 24** (from 6:00 AM to 5:00 PM).

### Team Members
* **Arun Bhatt**
* **Swastika Shrestha**
* **Niroj Maharjan**

---

## 📄 License & Acknowledgments

* **License**: MIT Open Source License.
* **Model**: Powered by [Ultralytics YOLOv8](https://github.com/ultralytics/ultralytics).
* **Annotations**: Built using [Supervision by Roboflow](https://github.com/roboflow/supervision).

