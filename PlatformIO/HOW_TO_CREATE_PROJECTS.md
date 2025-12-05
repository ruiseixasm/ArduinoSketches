Method 1: Simple Copy (Quick & Dirty)
text
OriginalProject/
├── .pio/           ← DELETE THIS!
├── .vscode/        ← DELETE or MODIFY
├── src/
│   └── main.cpp
└── platformio.ini

Copy to:
NewProject/
├── src/
│   └── main.cpp    ← Edit your code
└── platformio.ini  ← Edit config
What to Delete/Keep:
bash
# After copying, clean up:
rm -rf .pio          # Build artifacts
rm -rf .vscode       # Or edit for new project
# Keep: src/ and platformio.ini
Method 2: Template System (Better)
Create a template folder:
text
PlatformIO_Templates/
├── ESP32_Blink/
│   ├── src/
│   │   └── main.cpp
│   └── platformio.ini
├── ESP32_WiFi/
├── ESP32_Sensor/
└── copy_template.sh
Copy script copy_template.sh:
bash
#!/bin/bash
# Usage: ./copy_template.sh ESP32_Blink MyNewProject

TEMPLATE=$1
NEW_NAME=$2

if [ -z "$TEMPLATE" ] || [ -z "$NEW_NAME" ]; then
    echo "Usage: $0 <template> <new_project_name>"
    exit 1
fi

cp -r "PlatformIO_Templates/$TEMPLATE" "$NEW_NAME"
cd "$NEW_NAME"
rm -rf .pio .vscode

echo "Created $NEW_NAME from template $TEMPLATE"
Method 3: PlatformIO CLI (Best)
bash
# Create new project with same board
pio project init --board esp32dev --project-dir NewProject


# Create PlatformIO project inside:

## Navigate to workspace
cd "C:\Users\rui\Documents\MyElectronicsWorkspace"

## Create the project folder
mkdir Dummy

## Create PlatformIO project in subfolder
pio project init --board esp32dev --project-dir "Dummy"


# Copy only your source files
cp OriginalProject/src/* NewProject/src/
Method 4: Git Template (Professional)
bash
# Create template repository
git clone --bare https://github.com/yourname/esp32-template.git
git clone esp32-template.git NewProject
cd NewProject
# Edit files
git commit -am "New project based on template"
Critical Files to Modify:
1. platformio.ini - MUST CHANGE:
ini
; Original
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

; New project - change board if needed
[env:nodemcu-32s]          # ← Change environment name
platform = espressif32
board = nodemcu-32s        # ← Different board
framework = arduino
monitor_speed = 9600       # ← Different speed
lib_deps =                 # ← Add/remove libraries
    blynkkk/Blynk@^1.2.0
2. .gitignore - Keep or Update:
gitignore
# PlatformIO
.pio/
.pioenvs/
.piolibdeps/

# VSCode
.vscode/
!.vscode/settings.json
!.vscode/tasks.json
3. .vscode/ Folder:
Either:

Delete it (let VSCode recreate)

Keep but update paths

Make it generic for all projects

Safe Copy Commands:
bash
# Copy clean project
rsync -av --exclude='.pio' --exclude='.git' --exclude='.vscode' OriginalProject/ NewProject/

# Or with tar
tar --exclude='.pio' --exclude='.vscode' -cvf - OriginalProject/ | tar -C NewProject/ -xvf -
Project Structure Best Practice:
text
Projects/
├── Template_ESP32/          # Your master template
│   ├── platformio.ini
│   ├── .gitignore
│   └── src/
│       └── main.cpp
├── BlinkBlue/              # Actual project 1
├── SensorNode/             # Actual project 2
└── WeatherStation/         # Actual project 3
Automated Project Creator:
Create new_pio_project.py:

python
#!/usr/bin/env python3
import shutil
import os
import sys

def create_from_template(template, new_name, board=None):
    # Copy template
    shutil.copytree(template, new_name, 
                   ignore=shutil.ignore_patterns('.pio', '.vscode', '.git'))
    
    # Update platformio.ini if new board specified
    if board:
        with open(f'{new_name}/platformio.ini', 'r') as f:
            content = f.read()
        
        # Change board setting
        content = content.replace('board = esp32dev', f'board = {board}')
        
        with open(f'{new_name}/platformio.ini', 'w') as f:
            f.write(content)
    
    print(f'Created {new_name} from {template}')
    print(f'cd {new_name}')
    print('code .')

if __name__ == '__main__':
    create_from_template('Template_ESP32', sys.argv[1], 
                        sys.argv[2] if len(sys.argv) > 2 else None)
Common Pitfalls to Avoid:
Don't copy .pio/ - Contains compiled binaries

Don't copy .vscode/launch.json - May have absolute paths

Check library dependencies - Update lib_deps in new project

Update project-specific settings - Ports, WiFi credentials, etc.

Quick Copy Recipe:
bash
# 1. Create new folder
mkdir NewProject

# 2. Copy only essentials
cp OriginalProject/platformio.ini NewProject/
cp -r OriginalProject/src NewProject/

# 3. Clean up
cd NewProject
rm -rf .pio 2>/dev/null

# 4. Edit files
code platformio.ini  # Change board/settings
code src/main.cpp    # Change code
Yes, copying works, but delete .pio/ and review .vscode/ settings! Better yet, create a clean template system for future projects.
