# Easy Car v1

> ⚠️ **Work in progress.** This is a personal/experimental IoT project. It controls real vehicle functions (ignition, locks, windows), so use it at your own risk and only on a vehicle you own and can test safely.

Easy Car v1 turns an ordinary car into a connected, smarter vehicle using a low-cost **ESP8266 (NodeMCU)** microcontroller. It lets you control the car remotely or locally over Wi-Fi, from a companion Android app, with the goal of adding modern-vehicle features to older cars using minimal electronics.

## Goals

- **Remote & local control** of the vehicle (ignition, locks, windows, lights) over Wi-Fi.
- **Improved safety** — by connecting the NodeMCU to the network, vehicles can eventually exchange data with each other to help avoid accidents.
- **Automation** of vehicle functions using IoT.
- **Low cost** — built with a minimal set of electronic components.

## Features

- Wi-Fi web server running on the ESP8266.
- **Manual mode:** text and voice commands (English and French) sent from the Android app.
- **Automatic mode:** starts or stops the car based on Wi-Fi signal strength (RSSI proximity).
- Control of ignition / accessory, engine start/stop, central lock/unlock, windows and lights.
- **OTA (over-the-air)** firmware updates after the first USB upload.
- Companion Android app built with MIT App Inventor.
- Custom control board designed in KiCad.

## Hardware

| Component | Purpose |
| --- | --- |
| NodeMCU ESP8266 | Wi-Fi microcontroller (the brain) |
| ULN2003 | Driver array for the relays |
| 5V relays | Switch the car's electrical circuits |
| Diodes | Protection against voltage spikes |
| Buck converter | Steps the car's 12V down to 5V |

## Repository structure

```
easy_car_v1/
├── Firmware.ino        # Arduino (ESP8266) firmware
├── Android App/        # MIT App Inventor project (easycar.aia)
├── PCB/                # KiCad PCB design (easycar.kicad_pcb)
├── IMG/                # PCB images
└── README.md
```

## Getting started

### 1. Flash the firmware
- Open `Firmware.ino` in the **Arduino IDE** (with ESP8266 board support installed), or upload the pre-compiled `easycar.bin`.
- Before flashing, set your own values at the top of the firmware:
  - Wi-Fi `ssid` and `password`
  - OTA update username and password (**do not keep the defaults**)
- Connect the NodeMCU via USB and upload.

### 2. Build the Android app
- Go to **https://ai2.appinventor.mit.edu/** and import `Android App/easycar.aia` to open, edit or compile the app.

### 3. OTA updates (after the first USB upload)
Once the firmware is running, you can update it over the air from your browser:
```
http://<YOUR_DEVICE_IP>:81/update
```
You'll be prompted for the OTA username and password you set in the firmware.

### 4. PCB
The control board can be fabricated from the KiCad design in `PCB/easycar.kicad_pcb`.

![Easy Car PCB](/IMG/easycar.png)
![Easy Car PCB](/IMG/easycar1.png)

## Electronics / Schematic

The system follows a simple chain: the **NodeMCU (ESP8266)** sends 3.3V control signals to a **ULN2003** Darlington driver, which switches **5V relays**, and each relay drives one car circuit. A **5V buck converter** steps the car's 12V down to power the NodeMCU, and **flyback diodes** protect the circuit from the relay coils.

![Easy Car v1 wiring schematic](/IMG/schematic.png)

### GPIO pin mapping (from `Firmware.ino`)

| Function | NodeMCU pin | GPIO | ULN2003 input |
| --- | --- | --- | --- |
| Ignition (engine start) | D6 | GPIO12 | IN1 |
| ACC / dashboard | D8 | GPIO15 | IN2 |
| Lights | D7 | GPIO13 | IN3 |
| Unlock | D5 | GPIO14 | IN4 |
| Lock | D3 | GPIO0 | IN5 |
| Windows | D4 | GPIO2 | IN6 |
| Start button (input) | D2 | GPIO4 | — |

**Power & common:** ULN2003 pin 8 = GND, pin 9 (COM) = +12V for the relay coils; NodeMCU, ULN2003, buck converter and the car chassis all share a **common ground**.

## Security note

Earlier versions of this project shipped with default credentials (`admin/admin`) and a hardcoded Wi-Fi password. **Always set your own Wi-Fi and OTA credentials** before flashing, and never commit real passwords to the repository.

## Roadmap

- [ ] Vehicle-to-vehicle data exchange for collision avoidance
- [ ] More automation and sensor inputs
- [ ] Hardened authentication and security

## Author

**Aziz Ayoujil** — ayoujil.aziz0@gmail.com
