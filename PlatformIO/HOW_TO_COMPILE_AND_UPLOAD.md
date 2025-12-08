# COMPILE
pio run --project-dir .
pio run -e master
# UPLOAD
pio run -t upload --project-dir .
pio run -e master --target upload
pio run -e master --target upload --upload-port COM4
# MONITOR
pio device monitor --project-dir .
pio device monitor
pio device monitor --port COM4 --baud 115200
# CLEAN ENVIRONMENT
pio run -t clean
# CHECK FOLDERS
dir src /B
