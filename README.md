# ESP32 LoRa Environmental Datalogger — Sink Node

Custom ESP-IDF firmware and KiCad-designed PCB for a low-power LoRa
environmental sensor network. This repository contains the **sink node**
firmware — the receiver/base station that collects telemetry transmitted
by distributed sensor nodes over LoRa.

![PCB Top View](hardware/photos/board-top.jpg)

## Overview

[2–3 sentences: what this node does in the network, why a sink/receiver
architecture was chosen, what problem it solves. Example:]

This node receives environmental telemetry from distributed field sensor
nodes over a LoRa mesh and aggregates it for storage and downstream
processing. It's part of a larger low-power environmental monitoring
system designed for long-term, unattended field deployment.

## Key features

- Custom LoRa driver written in C (`LoRa.c` / `LoRa.h`) — no off-the-shelf
  library dependency
- Built on ESP-IDF (not Arduino) for direct hardware control and
  lower-level power management
- Reproducible build environment via `.devcontainer`
- Custom PCB designed in KiCad (schematic, layout, and BOM included)

## Hardware

- **MCU:** ESP32 [exact module/dev board — e.g., ESP32-WROOM-32]
- **Radio:** [LoRa module model, e.g., SX1276/RFM95]
- **PCB:** Custom-designed in KiCad — see [`/hardware`](hardware/)
- **Power:** [battery/solar setup if applicable]

| Schematic | PCB Layout |
|---|---|
| ![Schematic](hardware/exports/schematic.png) | ![Layout](hardware/exports/pcb-layout-top.png) |

Full KiCad project files: [`hardware/kicad-project`](hardware/kicad-project)  
Bill of materials: [`hardware/exports/bill-of-materials.csv`](hardware/exports/bill-of-materials.csv)

## Repository structure

```text
esp32-lora-environmental-datalogger-sink/
├── main/
│   ├── main.c              # Application entry point and sink node logic
│   └── CMakeLists.txt
├── LoRa/
│   ├── LoRa.c               # Custom LoRa driver implementation
│   └── LoRa.h
├── hardware/
│   ├── kicad-project/        # KiCad schematic + PCB layout source files
│   ├── exports/              # Rendered schematic/layout images, BOM
│   └── photos/                # Physical board photos
├── .devcontainer/            # Reproducible dev environment config
├── .vscode/                  # Editor configuration
├── CMakeLists.txt            # Project-level ESP-IDF build config
└── sdkconfig                 # ESP-IDF project configuration
```

## Build instructions

This project uses [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/).

```bash
# Set up ESP-IDF environment (v[X.X], if you want to specify)
. $HOME/esp/esp-idf/export.sh

# Clone and enter the project
git clone https://github.com/SuryaTejaJakka14/esp32-lora-environmental-datalogger-sink.git
cd esp32-lora-environmental-datalogger-sink

# Configure, build, and flash
idf.py set-target esp32
idf.py build
idf.py -p [PORT] flash monitor
```

## PCB design (KiCad)

The board was designed from scratch in KiCad to [reason — e.g., "fit a
compact enclosure for field deployment" / "integrate the LoRa module and
power regulation on a single board"].

- Schematic and layout: [`hardware/kicad-project`](hardware/kicad-project)
- Rendered exports: [`hardware/exports`](hardware/exports)
- Full build photos and log: [Hackaday.io project](your-hackaday-link-here)

## What I'd improve next

- [TLS/secure comms between nodes]
- [Cloud sync or database integration for the sink node]
- [Enclosure/weatherproofing improvements]
- [Power optimization / sleep mode tuning]

## Related projects

- [Job Intelligence Data Pipeline](https://github.com/SuryaTejaJakka14/job-intelligence-data-pipeline) — data engineering and automation pipeline

## Author

**Surya Teja Jakka**  
Data & IoT Engineer | Embedded Systems | Data Pipelines

- GitHub: [@SuryaTejaJakka14](https://github.com/SuryaTejaJakka14)
- LinkedIn: [linkedin.com/in/teja-j14](https://www.linkedin.com/in/teja-j14/)
- Hackaday.io: [your-hackaday-profile-link]
- Email: sj888@nau.edu
