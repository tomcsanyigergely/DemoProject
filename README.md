# DemoProject

## Video
YouTube URL: https://www.youtube.com/watch?v=igeaqjTsYtA

## Linux dedicated server build
Download: https://github.com/tomcsanyigergely/DemoProject/releases/download/v1.0.0/LinuxServer.zip

How to run:
```
unzip LinuxServer.zip
cd LinuxServer/FastPacedShooter/Binaries/Linux
chmod +x FastPacedShooterServer
./FastPacedShooterServer
```

## Windows client build
Download: https://github.com/tomcsanyigergely/DemoProject/releases/download/v1.0.0/WindowsClient.zip

Unzip, then run the `FastPacedShooterClient.exe` executable using the following command (from PowerShell):
```
./FastPacedShooterClient.exe -ServerAddress="<SERVER_IP_ADDRESS>" -Nickname=<YOUR_NICKNAME>
```
where:
- `<SERVER_IP_ADDRESS>` is the IP address of the server
- `<YOUR_NICKNAME>` is a custom player name (use a different nickname for each client)

## Controls
* WASD - move
* Space - jump
* Left Mouse Button - shoot
* Right Mouse Button - aim down sights
* Mouse wheel up/down - switch weapon
* Left Shift - sprint
* R - reload
* TAB - show/hide scoreboard
* I - enable/disable hitbox debugging
* K - enable/disable aimbot
* H - show/hide statistics
* ö - open console:
  * clear hitboxes: `FlushPersistentDebugLines`
  * show/hide FPS: `stat FPS`
  * limit FPS: `t.MaxFPS 60` or `t.MaxFPS 144` or choose any other frame rate

## Credits
https://sketchfab.com/3d-models/ak47-831519a097d84e079fd8bc4b15e5b57d<br>
https://sketchfab.com/3d-models/rifle-awp-weapon-model-cs2-23ad3d7fb46b40e59cab7937654e2691
