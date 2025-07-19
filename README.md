# _PHR24G03 "ToFMAP"_
Proyecto propuesto en la asignatura Programación Hardware Reconfigurable de la ETSI de Sistemas Informaticos de la UPM del año 2024 (grupo 3).
## ToFMAP ¿Qué es?
Proyecto de mapeo de espacios en 3D 

## Hardware empleado

Entre otros, principalmente:
- 1x Basys 3
- 1x PMOD ToF
- 2x ESP32

## Características clave

El proyecto consta de las siguientes características clave:
- Control de un vehículo bajo demanda con un mando o desde un PC.
- Visualización de datos de entorno más concretamente las dimensiones de éste.

## Código fuente

- ESP0-Controller 
--- Código fuente C/C++ del ESP32 que se coloca junto al mando
- ESP1-Vehicle 
--- Código fuente C/C++ del ESP32 que se comunica mediante UART a la Basys 3
- ToFMAP 
--- Código fuente VHDL de la FPGA se trata del componente principal del proyecto
- ToFMAP-Viewer
--- Código fuente C/C++ del software de Mapeo
