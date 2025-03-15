# Roslund Key Notes
This is a repo specifcially tailored to our robot roslund which is inteded to be controlled via a pi-picow like microcontroller. We use serial ctl to serialize controller data over bluetooth communications (Not intended for X-Bee). 

## Devlopment Log
The below log is for keeping track of changes made to this project

- 2024-09-25 : Created this readme - Cole, Natorion, Lucas
	- `packet.h` is the file that defines the data packets that are sent between the client and robot. It is a C struct.
	- Purpose: 
		- The changes made to serialctl was in regards to getting roslund working for niu stemfest 2024: the goal of this is to have mechannum drive to work with bluethooth serial ctl. maybe even with a ps5 or ps4 controller.
- 2025-03-08
	- Code woked for DevUp event! Yayyy!
- 2025-03-09 : Read me edited by Natorion and Lucas.
	- This repository includes both robot code and serial ctl client.


## File Structure(for robot code):
- README.md
- src
	- src/base64.c
	- src/base64.h
	- src/crc16.c
	- src/crc16.h
	- src/globals.h
	- src/main.cpp
	- src/MCP3XXX.h
	- src/packet.h
	- src/ribtest.code-workspace
	- src/zserio.cpp
	- src/zserio.h
### File Structure(Client code)
- serialctl-client
	- base64.c
	- base64.h
	- crc16.c
	- joystick.c
	- joystick.h
	- main.c
	- Makefile
	- packet.h 
	- serio.c
	- serio.h

# Building
## Compiling for Linux
```sh
git clone https://github.com/illinoistechrobotics/roslund-serialctl-2025.git
cd roslund-serialctl-2025/serialctl-client
make
```

## Compiling for Windows  
- Install Cygwin https://www.cygwin.com/setup-x86_64.exe  
- Add packages when prompted: `libSDL2-devel libSDL2_2.0_0 make git gcc-core`  
- Use latest stable package versions.  


Open Cygwin64 Terminal
- `git clone https://github.com/illinoistechrobotics/roslund-serialctl-2025.git`
- `cd serialctl`
- make windows
- the folder `release` now contains a portable windows build with the necessary DLLs. Run from commandline like on Linux

# Running
## Running on Linux

- Install the bluez utility packages required (on Arch, `bluez-utils` and `bluez-deprecated-utils`; you need the `rfcomm` and `bluetoothctl` commands)
- Pair your robot, it will disconnect after pairing but this is expected.
- Run `serialctl-client/bluetooth-helper.sh` as superuser, and select the correct device.

## Running on Windows
### Connecting to Roslund

- Open Bluetooth and connect to roslund
	- (it will disconnect dont worry)
- go to settings > bluetooth and devices > view more devices > more bluetooth settings
- Goto COM PORTS

From there you should be able to see what com ports are used for communicating with roslund. 

### Start Controller

- Open the `release` folder and run your terminal from there
- To start driving the robot run this command
./serialctl.exe /dev/com??? 0 

note for the specific COM port, test both the incomming and outgoing ports listeed in the bluetooth COM port settings 

### Drive Modes
```
#define ARCADE_DRIVE 0
#define TANK_DRIVE 1
#define MECANUM_DRIVE 2
```


