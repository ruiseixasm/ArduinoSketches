# INSTALL

## Method A: Install with pip (recommended)
pip3 install platformio

## OR Method B: Use install script
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py

## Add to PATH (if not auto-added)
echo 'export PATH=$PATH:~/.local/bin' >> ~/.bashrc
source ~/.bashrc


# CHECK INSTALLATION
which platformio
which pio
pip list | grep platformio


# CLEAN CACHE

## Clean PlatformIO Cache
~/.platformio/penv/bin/pio system prune
~/.platformio/penv/bin/pio system cleanup

## Clear Package Cache
### Remove downloaded package cache (This can free 500MB-1GB)
rm -rf ~/.platformio/.cache


# UNINSTALL

## Remove apt version
sudo apt remove platformio

## Remove pip versions
pip uninstall platformio
pip3 uninstall platformio

## Remove user installation
rm -rf ~/.platformio


# UPDATE PIP
python3.13 -m pip install --upgrade pip


