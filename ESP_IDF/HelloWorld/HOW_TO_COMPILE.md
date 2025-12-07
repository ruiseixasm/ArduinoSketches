# WINDOWS

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

