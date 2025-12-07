# WINDOWS

## Important note
Always prefer the Command Prompt to the Power Shell

## Always run
C:\Users\rui\esp\esp-idf\export.bat

## Navigate to the project root folder
cd ESP_IDF\HelloWorld

## Compile with
idf.py build

## Upload the build
idf.py -p COM4 flash

## Serial monitor (harder to stop, use other tool)
idf.py -p COM4 monitor

## To exit from the monitor, press
Ctrl+]


# Linux

## Source the export script (adjust path if different)
source $HOME/esp/esp-idf/export.sh

## Compile with
idf.py build

## Upload the build
idf.py flash
idf.py -p /dev/ttyUSB0 flash

## Monitor while flashing
idf.py flash monitor


