
$env:Path += ";$env:USERPROFILE\.platformio\penv\Scripts"
pio project init --board esp32dev --project-dir "Dummy"


echo $env:USERPROFILE

A. Open System Environment Variables:
Press Win + R, type sysdm.cpl, press Enter

Click Advanced → Environment Variables

Under User variables, find/select Path, click Edit

Click New, add: C:\Users\rui\.platformio\penv\Scripts

Click OK, OK, OK



# Check current user PATH
[Environment]::GetEnvironmentVariable("Path", "User")

# It should now include PlatformIO

