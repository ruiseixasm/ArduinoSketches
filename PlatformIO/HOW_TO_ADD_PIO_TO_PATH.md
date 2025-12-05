
$env:Path += ";$env:USERPROFILE\.platformio\penv\Scripts"
pio project init --board esp32dev --project-dir "Dummy"


echo $env:USERPROFILE
