# Smart Irrigation (LoRa + ESP32 + Web UI + Azure)

A long-range soil monitoring and irrigation controller using **two TTGO LoRa32 v1.6.1** boards and an **Arduino Nano ESP32 (S3)** at the field for PWM drive. The **gateway** computes irrigation decisions (with weather), then sends **PWM setpoints back to the field** over LoRa. The **field** forwards those PWM values over **UART** to the Nano ESP32 which drives **2× DRV8833** (4 pumps).

- **Field unit (Transmitter + Actuation):**  
  TTGO LoRa32 reads 10 soil channels (+ DHT11, INA219), sends telemetry to the gateway, and **relays pump PWM commands to a Nano ESP32** over UART. The **Nano ESP32** outputs PWM to **2× DRV8833** → **4 pumps**.

- **Gateway (Receiver + Logic + Cloud):**  
  TTGO LoRa32 hosts the **local web UI**, fetches **OpenWeatherMap** weather, applies **protections & thresholds**, and (optionally) publishes to **Azure IoT Hub**. It returns **PWM setpoints** to the field via LoRa.

---

## 🧭 Topology

```mermaid
flowchart LR
  subgraph Field [Field Unit — Transmitter + Actuation]
    TX[TTGO LoRa32 (Field TX)]
    MUX[CD74HC4067 — 10 soil]
    DHT[DHT11]
    INA[INA219 — bus V/I]
    Nano[Nano ESP32 (PWM 0–255)]
    DRV[2× DRV8833]
    PUMPS[4× Pumps]

    TX --> MUX
    TX --> DHT
    TX --> INA
    TX <-->|UART| Nano
    Nano --> DRV
    DRV --> PUMPS
  end

  TX <-->|LoRa| RX

  subgraph Gateway [Gateway — Receiver + Logic + Cloud]
    RX[TTGO LoRa32 (Gateway RX)]
    Web[Local Web UI]
    Weather[OpenWeatherMap API]
    Azure[Azure IoT Hub]
    Email[Email Alerts]

    RX --> Web
    RX --> Weather
    RX --> Azure
    Azure --> Email
  end
```

---

## ⚡ Power Flow (Field Unit)

```mermaid
flowchart TD
  LiPo[2S LiPo Battery (7.4 V nominal)] --> Fuse[Polyfuse / Protection]
  Fuse --> Reg[R-78B5.0-1.5 → 5V]
  Reg --> Bulk[Bulk + Decoupling Capacitors]

  Bulk --> TX[TTGO LoRa32 TX]
  Bulk --> NanoESP[Nano ESP32]
  Bulk --> MUX[CD74HC4067 + DHT11 + INA219]
  Bulk --> DRV[2× DRV8833]
  Bulk --> SIM[SIM7000G (optional)]
  DRV --> PUMPS[Water Pumps]
```

Notes:
- 3× 100 µF + 1× 470–1000 µF bulk caps recommended.  
- 0.1 µF ceramic decouplers at each IC.  
- SIM7000G requires peak current buffering (≥ 1000 µF).

---

## 💧 Pump Control Logic

- Field **does not compute irrigation** — it only reads sensors and relays data.  
- Gateway applies logic:  
  - Skip/reduce irrigation if rain is forecast.  
  - Enforce **per-pump soil cutoff %**.  
  - Apply **10 min lockout** after pump turns OFF (prevents chattering).  
  - Auto-return from **manual override** after timeout.  
- Gateway sends final **P,<d0>,<d1>,<d2>,<d3>** (0–255) over LoRa → Field → Nano ESP32 PWM.

---

## 📊 Data Flow

1. **Field** collects soil, temp/hum, battery, current → uplinks CSV via LoRa.  
2. **Gateway** merges telemetry with weather forecast.  
3. **Gateway** computes PWM setpoints → LoRa downlink.  
4. **Nano ESP32** generates PWM → DRV8833 → pumps.  
5. **Gateway** syncs to **Azure IoT Hub** and local Web UI.  

---

## 🔧 Features

- 10 soil channels via CD74HC4067 multiplexer.  
- Real-time telemetry on OLED + Web UI.  
- Secure Web API (HTTP basic auth).  
- Azure IoT Hub twin sync + telemetry.  
- Lockout safety & manual override support.  
- Configurable pump cutoffs, durations, temperature limits.  

---

## 📦 Future Improvements

- NB-IoT/LTE fallback (SIM7000G at field).  
- OTA firmware updates for ESP32 nodes.  
- Solar charging + BMS integration.  
- Capacitive soil sensors for longer lifespan.  

---

## 📜 License

MIT License – free to use and modify.

---

## 🙌 Acknowledgments

- Espressif ESP32  
- TTGO LoRa32  
- Recom R-78B5.0-1.5 regulator  
- Azure IoT Hub  
- OpenWeatherMap API  
- SIM7000G LTE/NB-IoT module  
