# LINUX

## Update package list
sudo apt update

## Install required dependencies
sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

## NavO dito partido "anti-sistema"...igate to your extra drive
cd /mnt/extra

## Create esp directory
mkdir -p esp
cd esp

## Clone ESP-IDF (using latest stable version)
git clone -b v5.1.4 --recursive https://github.com/espressif/esp-idf.git

> This will take some time to download...

## Move to the installation folder
cd esp-idf

## Install ALL tools to /mnt/extra
#### 1. Create the directory on extra drive
mkdir -p /mnt/extra/esp/.espressif

#### 2. Create the symlink
ln -s /mnt/extra/esp/.espressif ~/.espressif

#### 3. Now run installer
cd /mnt/extra/esp/esp-idf
./install.sh

#### Or use this command (fills local drive though)
./install.sh --install-prefix /mnt/extra/esp/.espressif


## Install ESP-IDF tools (Python packages, compilers, etc.)
./install.sh

> The installer will download tools to ~/.espressif by default

## If you want tools on /mnt/extra too:
./install.sh --install-prefix /mnt/extra/esp/.espressif


## Set up environment
echo "alias get_esp='. /mnt/extra/esp/esp-idf/export.sh'" >> ~/.bashrc
source ~/.bashrc
get_esp


## Set up environment
. ./export.sh






## Check tools location
echo $IDF_TOOLS_PATH

## Build a test project
cd /mnt/extra
mkdir -p test_project/main
cd test_project

## Create minimal project
cat > main/main.c << 'EOF'
##include <stdio.h>
void app_main(void) { printf("Test!\n"); }
EOF

## Create CMakeLists.txt
echo "idf_component_register(SRCS \"main.c\")" > main/CMakeLists.txt
echo -e "cmake_minimum_required(VERSION 3.16)\ninclude(\$ENV{IDF_PATH}/tools/cmake/project.cmake)\nproject(test)" > CMakeLists.txt


## Build
idf.py set-target esp32
idf.py build




# WINDOWS

## Install Prerequisites:

Install Python 3.8+ from python.org (check "Add to PATH")
Install Git from git-scm.com

## Open Power Shell as Administrator and run:

python -m pip install --upgrade pip
python -m pip install virtualenv

## Open Power Shell as User and run:

cd ~
mkdir esp
cd esp
git clone -b v5.1.3 --recursive https://github.com/espressif/esp-idf.git

## Open Command Prompt as User and do:

cd esp\esp-idf
.\install.bat
.\export.bat


## Add this to your system PATH:

Press Win + X → System → Advanced system settings
Environment Variables → System variables → Path → Edit

Add: C:\Users\rui\esp\esp-idf\tools
Also add: C:\Users\rui\.espressif\tools\xtensa-esp32-elf\esp-2021r2-patch5-8.4.0\xtensa-esp32-elf\bin
(The exact path might be different - check your .espressif folder)

> But the simplest is just to remember: Always run export.bat first in each new Command Prompt session!










## Create the batch file that sets up environment
echo @echo off > esp-idf-cmd.bat
echo echo Setting up ESP-IDF environment... >> esp-idf-cmd.bat
echo call "%~dp0export.bat" >> esp-idf-cmd.bat
echo cd /d %%USERPROFILE%%\esp >> esp-idf-cmd.bat
echo echo ESP-IDF Command Prompt ^(version 5.1^) >> esp-idf-cmd.bat
echo echo. >> esp-idf-cmd.bat
echo cmd /k >> esp-idf-cmd.bat

## Create Start Menu shortcut (run as Administrator)
mkdir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\ESP-IDF"
mklink "%APPDATA%\Microsoft\Windows\Start Menu\Programs\ESP-IDF\ESP-IDF 5.1 CMD.lnk" "C:\Users\rui\esp\esp-idf\esp-idf-cmd.bat"

