# Dory

![Dory project logo](images/ChatGPT%20Image%20May%2029,%202026,%2011_48_04%20PM.png)

A custom control and power distribution board for NCSSM Morganton Robotics.

![NCSSM Morganton Robotics](images/ncssm_robotics_logo.png)

## Why this exists

This board started as a wiring problem.

On the robot, too many things were being connected with loose jumper wires, splitters, adapters, and one-off harnesses. It worked, but it was annoying to build, annoying to trace, and even more annoying to debug when something came loose. Every extra wire was another failure point, and every repair meant spending time figuring out which connection had moved instead of working on the robot.

Float PCB is meant to clean that up. It brings the common controller, power, sensor, light, and debug connections onto one board so the robot is easier to assemble, easier to service, and less dependent on fragile wiring.

## Board evolution

The design went through several revisions while the board moved from a simple rectangular breakout toward the current round layout with cleaner connector placement, clearer labels, and more intentional branding.

![Top-side PCB revision collage](images/pcb_snapshots/top_collage.png)

![Bottom-side PCB revision collage](images/pcb_snapshots/bottom_collage.png)

## Features

- **ESP32-S3 controller** using an ESP32-S3-WROOM-1U module for onboard control and wireless-capable development.
- **External antenna support** through the ESP32-S3-WROOM-1U module for more flexible radio placement.
- **USB-C interface** for programming, debugging, and convenient bench access.
- **XT30 battery input** for a more robust main power connector than loose leads.
- **64 mm round board outline** for a compact layout that fits cleanly into the robot.
- **4-layer PCB stackup** with front/back signal layers and two internal power layers.
- **5V and 3.3V regulation** using TPSM863252 power modules.
- **ESP-controlled 5V enable** so software can turn the 5V rail on or off when needed.
- **Onboard buzzer** for audible alerts, even when the board is mounted inside the robot.
- **External light output** for driving a Blue Robotics-style indicator or robot status light.
- **Dual I2C ports** on JST-XH connectors for cleaner sensor wiring.
- **Robot I/O connectors** for switch, light, servo, buzzer, and miscellaneous expansion signals.
- **Battery sensing** through a resistor divider and dedicated `BATT_ADC` test point.
- **Status LEDs** for battery, 5V, and 3.3V rails.
- **USB ESD protection** on the USB data lines.
- **Debug/test access** with labeled test points for power rails, boot, enable, I2C, and ground.
- **Mechanical mounting** with three M2 mounting holes.

## Hardware overview

The board is built around an ESP32-S3-WROOM-1U module and breaks out the useful robot-facing signals to keyed or serviceable connectors. The PCB is a 64 mm diameter, 4-layer circular board with front and back signal routing plus two internal power layers. Power enters through an XT30 connector, then the onboard regulators generate the rails needed by the controller and peripherals.

The intent is not to replace every subsystem on the robot. The goal is to provide a cleaner central board for the connections that otherwise turn into messy wiring: power, I2C devices, lights, switches, a buzzer, servo control, battery monitoring, and spare expansion lines.

## Work in progress

The current plan is to use the onboard ESP32 as the robot-side controller and pair it with a separate ESP32-based base station. The base station will communicate with the ESP32 on the Float PCB, giving the robot a cleaner wireless control/debug path without needing to plug directly into the board every time.

An accompanying web interface is also planned. The site will be built with Svelte and use Web Serial so a laptop can talk to the ESP32 base station directly from the browser. The goal is to make setup, monitoring, and debugging accessible without requiring a custom desktop app.

## Repository contents

- `float_pcb.kicad_pro` — KiCad project file.
- `float_pcb.kicad_sch` — schematic source.
- `float_pcb.kicad_pcb` — PCB layout source.
- `jlcpcb/production_files/` — generated BOM, CPL, and Gerber ZIP for JLCPCB.
- `jlcpcb/gerber/` — expanded Gerber and drill outputs.
- `float_pcb.csv` — project BOM/export data.
- `digikey/order.csv` — DigiKey ordering file.
- `images/` — README and board/team branding assets.

## Manufacturing

The repository includes production outputs under `jlcpcb/production_files/`:

- `GERBER-float_pcb.zip`
- `BOM-float_pcb.csv`
- `CPL-float_pcb.csv`

Before ordering, re-open the project in KiCad, refill zones, run DRC/ERC, and regenerate production files from the latest schematic and PCB layout. The checked-in files are useful for handoff, but the KiCad sources should be treated as the source of truth.

For component ordering, use `digikey/order.csv` as the DigiKey order file.

## Known issues

- One of the I2C ports is about 0.025 mm off in the Z direction. This is only for people with OCD like me; it should not matter for normal assembly.
- The fancy QFN buck converter packages are hard to solder by hand. If you want to build this board, make sure you have solder paste, reflow, and patience, or redesign that power stage around something friendlier like a SOT-23 package.

## Development notes

- Designed in KiCad.
- Uses mostly SMD passives with JST-XH, XT30, USB-C, and pin-header connections.
- Includes test points for bring-up and debugging.
- Intended for robotics use where reliability and serviceability matter more than minimizing every connector or square millimeter.

## Team

Built for NCSSM Morganton Robotics as a practical fix for too much robot wiring.
