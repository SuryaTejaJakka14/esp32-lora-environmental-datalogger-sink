# ESP32 LoRa Environmental Datalogger — Sink Node

Custom ESP-IDF firmware and KiCad-designed PCB for a low-power LoRa
environmental sensor network. This repository contains the **sink node**
firmware — the receiver/base station that collects telemetry transmitted
by distributed sensor nodes over LoRa.

![PCB Top View](hardware/Images/PCB_Top.jpg)

## Overview

[2–3 sentences describing what this node does in the network and why a
sink/receiver architecture was chosen.]

## Key features

- Custom LoRa driver written in C (`LoRa/LoRa.c`, `LoRa/LoRa.h`) — no
  off-the-shelf library dependency
- Built on ESP-IDF (not Arduino) for direct hardware control
- Reproducible dev environment via `.devcontainer`
- Custom PCB schematic designed in KiCad

## Hardware

- **MCU:** ESP32 [exact module — e.g., ESP32-WROOM-32]
- **Radio:** [LoRa module model, e.g., SX1276/RFM95]
- **PCB:** Custom schematic designed in KiCad — see [`/hardware/schematic`](hardware/schematic)

| PCB — Top | PCB — Bottom |
|---|---|
| ![PCB Top](hardware/Images/PCB_Top.jpg) | ![PCB Bottom](hardware/Images/PCB_Bottom.jpg) |

KiCad schematic and project files: [`hardware/schematic`](hardware/schematic)

> **Note:** This repository currently includes the KiCad schematic
> (`PCB.kicad_sch`) and project file (`PCB.pro`). The PCB layout file
> will be added once finalized.

## Repository structure

```text
esp32-lora-environmental-datalogger-sink/
├── main/
│   ├── main.c              # Application entry point and sink node logic
│   └── CMakeLists.txt
├── LoRa/
│   ├── LoRa.c                # Custom LoRa driver implementation
│   └── LoRa.h
├── hardware/
│   ├── schematic/
│   │   ├── PCB.kicad_sch      # KiCad schematic
│   │   └── PCB.pro            # KiCad project file
│   └── Images/
│       ├── PCB_Top.jpg
│       └── PCB_Bottom.jpg
├── .devcontainer/            # Reproducible dev environment config
├── .vscode/                  # Editor configuration
├── CMakeLists.txt            # Project-level ESP-IDF build config
└── sdkconfig                 # ESP-IDF project configuration
```

## Build instructions

This project uses [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/).

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Clone the project
git clone https://github.com/SuryaTejaJakka14/esp32-lora-environmental-datalogger-sink.git
cd esp32-lora-environmental-datalogger-sink

# Configure, build, and flash
idf.py set-target esp32
idf.py build
idf.py -p [PORT] flash monitor
```

## PCB design (KiCad)

The schematic was designed from scratch in KiCad to [reason — e.g.,
"integrate the LoRa module and power regulation for the sink node"].

- Schematic and project files: [`hardware/schematic`](hardware/schematic)
- Board photos: [`hardware/Images`](hardware/Images)
- Full build log: [Hackaday.io project](your-hackaday-link-here)

## What I'd improve next

- Finalize and add the KiCad PCB layout file (`.kicad_pcb`)
- [TLS/secure comms between nodes]
- [Cloud sync or database integration for the sink node]
- [Power optimization / sleep mode tuning]

## Related projects

- [Job Intelligence Data Pipeline](https://github.com/SuryaTejaJakka14/job-intelligence-data-pipeline) — data engineering and automation pipeline

## Author

**Surya Teja Jakka**  
Data & IoT Engineer | Embedded Systems | Data Pipelines

- GitHub: [@SuryaTejaJakka14](https://github.com/SuryaTejaJakka14)
- LinkedIn: [linkedin.com/in/teja-j14](https://www.linkedin.com/in/teja-j14/)
- Email: sj888@nau.edu
