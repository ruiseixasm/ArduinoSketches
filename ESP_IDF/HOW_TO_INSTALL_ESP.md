# Update package list
sudo apt update

# Install required dependencies
sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# NavO dito partido "anti-sistema"...igate to your extra drive
cd /mnt/extra

# Create esp directory
mkdir -p esp
cd esp

# Clone ESP-IDF (using latest stable version)
git clone -b v5.1.4 --recursive https://github.com/espressif/esp-idf.git

> This will take some time to download...

# Move to the installation folder
cd esp-idf

# Install ALL tools to /mnt/extra
## 1. Create the directory on extra drive
mkdir -p /mnt/extra/esp/.espressif

## 2. Create the symlink
ln -s /mnt/extra/esp/.espressif ~/.espressif

## 3. Now run installer
cd /mnt/extra/esp/esp-idf
./install.sh

## Or use this command (fills local drive though)
./install.sh --install-prefix /mnt/extra/esp/.espressif


# Install ESP-IDF tools (Python packages, compilers, etc.)
./install.sh

> The installer will download tools to ~/.espressif by default

# If you want tools on /mnt/extra too:
./install.sh --install-prefix /mnt/extra/esp/.espressif


# Set up environment
echo "alias get_esp='. /mnt/extra/esp/esp-idf/export.sh'" >> ~/.bashrc
source ~/.bashrc
get_esp


# Set up environment
. ./export.sh






# Check tools location
echo $IDF_TOOLS_PATH

# Build a test project
cd /mnt/extra
mkdir -p test_project/main
cd test_project

# Create minimal project
cat > main/main.c << 'EOF'
#include <stdio.h>
void app_main(void) { printf("Test!\n"); }
EOF

# Create CMakeLists.txt
echo "idf_component_register(SRCS \"main.c\")" > main/CMakeLists.txt
echo -e "cmake_minimum_required(VERSION 3.16)\ninclude(\$ENV{IDF_PATH}/tools/cmake/project.cmake)\nproject(test)" > CMakeLists.txt




# Build
idf.py set-target esp32
idf.py build


