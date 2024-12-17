# IRIS2-firmware-flight
Firmware for the IRIS2 CPU, that will fly as an instrument on the mission Sunrise III

## Prerequisites
You should install Code Composer Studio (Version 10 or higher preffered) https://www.ti.com/tool/CCSTUDIO
> :warning: **DEBUG_MODE**: In the configuration.h file, line 10 comment or uncomment to be in DEBUG mode. Always fly with Debug mode disabled!
```c
#define DEBUG_MODE
```

## Programming the MCU
You should have a programmer for MSP430, a launchpad is enough.
In the docs/ folder you will find all the documents with detailed description and datasheets of the hardware involved.

In order to program the MCU you must connect the following lines:
* GND
* SBWTDIO
* SBWTCK

The following lines are not always needed:
* 3V3, Only if you are not providing external power supply
* RDX, Only if you need access to Front Plate (Command line) UART 
* TXD, Only if you need access to Front Plate (Command line) UART

For easy access :)
<div align="center">
<img src="docs/JTAG_connection.png" alt="JTAG connection with Launchpad" width="70%"/>
</div>

## Global Overview of the Board
Some of the files that you MUST read before trying to program the MCU:
* Pinouts in detail: [docs/IRIS2_pinouts.xlsx](docs/IRIS2_pinouts.xlsx)
* Schematics of the board: [docs/IRIS2_PCB_CPU_v1.3.pdf](docs/IRIS2_PCB_CPU_v1.3.pdf)

<div align="center">
<img src="docs/IRIS2_CPU.png" alt="IRIS2 CPU Block Diagram" width="70%"/>
</div>

## Command line Interface
In order to interface with IRIS, a TTY interface is provided. 

### Command line Configuration
To access to the TTY from Windows we recommend using the program PuTTY that can be downloaded from here https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html
The configuration parameters should be as follows:
* Connection type: `Serial`
* Serial line: `COMx` (where "x" is the COM number in your computer that you can find in the "device manager")
* Speed: `115200`

After opening the TTY session, press 'Enter' and you should see the terminal command sign.

After a reboot, you should see the following message:
```
IRIS2 is booting up...
IRIS 2 (Image Recorder Instrument for Sunrise) terminal.
IRIS 2 Firmware version is '12', compiled on Apr 23 2022 at 15:38:03.
Made with passion and hard work by:
  * Aitor Conde <aitorconde@gmail.com>. Senior Engineer. Electronics. Software.
  * Ramon Garcia <ramongarciaalarcia@gmail.com>. Project Management. Software.
  * David Mayo <mayo@sondasespaciales.com>. Electronics.
  * Miguel Angel Gomez <haplo@sondasespaciales.com>. Structure.
IRIS 2 last booted 0 seconds ago.
Reboot counter is 4.
Last Reboot reason was 0x18, 'WDTPW password violation (PUC)'.
# 
```

### Command line commands
These are the currently implemented commands:

|Command      | Comment     |
|-------------|-------------|
|`help`         |It returns informations on commands and its use|
|`status`       |It returns all the status and telemetry of IRIS2|
|`reboot`       |It performs a PUC reboot of the MCU|
|`conf`         |It returns all the available configuration parameters|
|`conf set [parameter] [value]` |It sets a value to a configuration parameter|
|`uptime`       |It returns the up time in seconds|
|`unixtime`     |It returns the current unixtime  |
|`date`         |It returns the current system date|
|`date YYYY/MM/DD HH:mm:ss` |It sets the system date|
|`i2c rtc`      |It returns the current RTC date on the external RTC|
|`i2c baro`     |It returns the current barometric pressure and calculated Altitude|
|`i2c ina`      |It returns the current Voltage and Current input power values|
|`i2c acc`      |It returns the current Accelerometer values|
|`camera [x] pic`  |It makes automatically a picture with the [x] camera. It returns Error -4 if battery below 6.75V. It returns Error -1,-2,-3 if it was busy.|
|`camera [x] vid [sec]`  |It makes automatically a video with the [x] camera with a duration of [sec] seconds. It returns Error -4 if battery below 6.75V. It returns Error -1,-2,-3 if it was busy. It returns Error 1 if it was already doing video and duration is updated with new passed duration.|
|`camera [x] interrupt`  |It ends the video inmediately|
|`camera [x] on`  |It switches on the [x] camera|
|`camera [x] format`| :warning: It formats the SDCard of the [x] camera!!|
|`camera [x] picture_mode`|It sets the [x] camera to picture mode|
|`camera [x] video_mode`  |It sets the [x] camera to video mode|
|`camera [x] pic_raw` |It takes a picture with the [x] camera|
|`camera [x] video_start` |It starts a video on the [x] camera|
|`camera [x] video_end`   |It stops a video on the [x] camera|
|`camera [x] send_cmd y`   |Sends command y (do not include line feed at the end!) to camera x|
|`camera [x] off` |It switches off the [x] camera|
|`p [x]` |It presses the power button of the [x] camera|
|`tm nor`       |It returns current Telemetry Line to be saved in NOR memory|
|`tm fram`      |It returns current Telemetry Line to be saved in FRAM memory|
|`memory status` |It returns the current status of the memories|
|`memory dump [nor/fram] [start] [end]` |It dumps the contents of the NOR/FRAM memories of the CPU|
|`memory read [nor/fram] [events/tlm] [start] [end]` |It reads the contents of the NOR/FRAM memories and shows them in CSV|
|`memory erase [nor/fram] bulk` |It erases the NOR/FRAM memory of the CPU|
|`uartdebug [x]` |All characters received of the [x] UART will be dumped on the console. If 5 is selected then it shows verbosity in everything. 0 is the default and does not put anything on console.|
|`u [data]` |[data] will be dumped to the uart selected as debug|

### Configuration Parameters
Sending the command `conf` will print all the configuration parameters and its assigned values:

```
# conf
sim_enabled = 0
sim_pressure = 0
sim_sunriseSignal = 0
flightState = 1
flightSubState = 0
fram_tlmSavePeriod = 600
nor_deviceSelected = 0
nor_tlmSavePeriod = 10
baro_readPeriod = 1000
ina_readPeriod = 100
acc_readPeriod = 100
temp_readPeriod = 1000
leds = 1
gopro_model[0] = 0
gopro_model[1] = 0
gopro_model[2] = 0
gopro_model[3] = 0
gopro_beeps = 2
gopro_leds = 0
gopro_pictureSleep = 2500
launch_heightThreshold = 3000
launch_climbThreshold = 2
launch_videoDurationLong = 7200
launch_videoDurationShort = 3600
launch_camerasLong = 0x03
launch_camerasShort = 0x0C
launch_timeClimbMaximum = 7200
flight_timelapse_period = 120
flight_camerasFirstLeg = 0x0F
flight_camerasSecondLeg = 0x0B
flight_timeSecondLeg = 86400
landing_heightThreshold = 25000
landing_heightSecurityThreshold = 32000
landing_speedThreshold = -15
landing_videoDurationLong = 3600
landing_videoDurationShort = 900
landing_camerasLong = 0x02
landing_camerasShort = 0x09
landing_camerasHighSpeed = 0x04
landing_heightShortStart = 3000
recovery_videoDuration = 120
```
In order to change a configuration you should use the following command as example:
```
# conf set flightState 6
Selected parameter 'flightState' has been set to '6'.
```
List of configuration parameters:

|Command      | Default Value| Units        | Comment     |
|-------------|-------------:|:-------------|:------------|
| `sim_enabled` | 0 | | :warning: DEBUG Mode, enables simulation values for pressure and sunrise signal. 1 = Enabled, 0 = Disabled|
| `sim_pressure` | 0 | mbar / 100 | Simulated pressure, only takes effect if sim_enabled is = 1. A value of 101301 equals to 1013.01mbar| 
| `sim_sunriseSignal` | 0 | | :warning: DEBUG Mode, 0 = Sunrise signal is not simulated, 1 = Sunrise signal is simulated as HIGH, 2 = Sunrise signal is simulated LOW | 
| `flightState` | 1 | | :warning: 0 = Standby, 1 = Waiting for launch, 2 = Making launch video, 3 = Timelapse cruising, 4 = Making landing video, 5 = Timelapse waiting for recovery team, 6 = 2 min video of the team and timelapse post recovery|
| `flightSubState` | 0 | | Only useful on landing video, just sub-states|
| `fram_tlmSavePeriod` | 600 | s | Periodicity to save telemetry on the FRAM |
| `nor_deviceSelected` | 0 | | Selected NOR memory to work with, because we have two! |
| `nor_tlmSavePeriod` | 10 | s | Periodicity to save telemetry to on the NOR Flash |
| `baro_readPeriod` | 1000 | ms | Periodicity to read the barometer |
| `ina_readPeriod` | 100 | ms | Periodicity to read the Voltage and Current levels of the battery |
| `acc_readPeriod` | 100 | ms | Periodicity to read the 3 axis Accelerometer |
| `temp_readPeriod` | 1000 | ms | Periodicity to read the temperature sensors |
| `leds` | 1 | | 1 = LEDs are On and showing activity, 0 = All leds are off. Only affects the CPU and the Front plate but never the GoPros |
| `gopro_model[x]` | 0 | | 0 = Gopro Black (IRIS2), 1 = Gopro White (TouchScreen) |
| `gopro_beeps` | 2 | | 0 = 100% Volume, 1 = 70% Volume, 2 = off. Be aware that this is configured on next boot for the camera, however it takes effect only after a second reboot |
| `gopro_leds` | 1 | | 0 = Off, 1 = 2 blinks, 2 = 4 blinks |
| `gopro_pictureSleep` | 2500 | ms | Sleep time between the camera configures until it makes picture. 1s for small SDCards, 1.5s for at least 32GB SDCards, 2.5s for 64GB SDCards |
| `launch_heightThreshold` | 3000 | m | If reached this height and on state 1, IRIS will jump to State 2 (start making launch video) |
| `launch_climbThreshold` | 2 | m/s | If reached this speed and on state 1, IRIS will jump to State 2 (Start making launch video) |
| `launch_videoDurationLong` | 7200 | s | Duration of the video for the cameras to make long launch videos |
| `launch_videoDurationShort` | 3600 | s | Duration of the video for the cameras to make short launch videos |
| `launch_camerasLong` | 0x03 | hex | Selected cameras for making long videos. 0x03 means Cameras 1 and 2. 0x0F means all cameras |
| `launch_camerasShort` | 0x0C | hex | Selected cameras for making short videos. 0x0C means Cameras 3 and 4. 0x0F means all cameras |
| `launch_timeClimbMaximum` | 7200 | s | Safe time in which IRIS cannot jump to state 4 because it is too soon. This is to prevent triggering landing when the climb is very slow. This was reduced to 7200 because it caused conflict on first flight of IRIS2 due to the launch being aborted |
| `flight_timelapse_period` | 120 | s | Periodicity between pictures of every timelapse |
| `flight_camerasFirstLeg` | 0x0F | hex | Selected cameras for making timelapse during the first part of the cruise phase. 0x0F means all cameras |
| `flight_camerasSecondLeg` | 0x0B | hex | Selected cameras for making timelapse during the second part of hte cruise phase. 0x0B means Cameras 2, 3 and 4.
| `flight_timeSecondLeg` | 86400 | s | Duration of the first part of the cruise phase for timelapse. |
| `landing_heightThreshold` | 25000 | m | If reached this height or lower and on state 3, IRIS will jump to State 4 (start making landing video) |
| `landing_heightSecurityThreshold` | 32000 | m | Above this height, the measurements of the barometer are not considered safe to calculate vertical speeds and they are ignored |
| `landing_speedThreshold` | -15 | m/s | If reached this vertical speed or lower and on state 3, IRIS will jump to State 4 (start making landing video) |
| `landing_videoDurationLong` | 3600 | s | Duration of the long landing videos |
| `landing_videoDurationShort` | 900 | s | Duration of the short landing videos |
| `landing_camerasLong` | 0x02 | hex | Selected cameras for making long videos. 0x02 means that only Camera 2 will make video. 0x0F means all cameras |
| `landing_camerasShort` | 0x09 | hex | Selected cameras for making short videos. 0x09 means Cameras 1 and 4. 0x0F means all cameras |
| `landing_camerasHighSpeed` | 0x04 | hex | Selected cameras for making high speed video. 0x04 means Cameras 3. 0x0F means all cameras |
| `landing_heightShortStart` | 3000 | m | When reached this height or lower and on state 4, IRIS will start the second phase of the landing switching on all the cameras to make video of the impact |
| `recovery_videoDuration` | 120 | s | Duration of the video of the recovery team. It will activate on vibration detection |
| `rtcDriftFlag` | 1 |  | 0 = no drift corrections are applied, 1 or 2 = it activates the automatic correction of the drift RTC . 1 = RTC is slower than reality, 2 = RTC is faster than reality|
| `rtcDrift` | 2426 | s | This is the time in seconds that takes the RTC to have a 1 second error. The value 2426 s is the measured drift of the flight hardware but each board will be different, i.e. Aitor's flatsat has a value of 22302 s! |

### Testing the cameras and the Sunrise Signal
This are the recommended commands for testing the camera:
|Command      | Comment     |
|-------------|-------------|
|`status`     | It shows the current status |
|`memory status`| It shows the current status of the memories|
|`conf set gopro_leds 1` | It sets the LEDs of the cameras to be on while using them to help debugging|
|`conf`       | It shows the current configuration paramters|
|`conf set flightState 0` | It sets the instrument in standby status, so it will not make videos or pictures unless manually instructed|
|`conf set flightSubState 0` | Just in case|
|`uartdebug 5` | It will show on terminal more information than usual, especially when commanding cameras or with Sunrise digital signal|

After this secuence you are ready for fully testing the cameras, just copy paste the following commands:
```console
status
memory status
conf
conf set flightState 0
conf set flightSubState 0
conf set gopro_leds 1
uartdebug 5

camera 1 on
camera 1 format
camera 1 off

camera 2 on
camera 2 format
camera 2 off

camera 3 on
camera 3 format
camera 3 off

camera 4 on
camera 4 format
camera 4 off

camera 1 pic
camera 2 pic
camera 3 pic
camera 4 pic

camera 1 pic
camera 2 pic
camera 3 pic
camera 4 pic

camera 1 vid 30
camera 2 vid 30
camera 3 vid 30
camera 4 vid 30

camera 1 pic
camera 2 pic
camera 3 pic
camera 4 pic
```
After this, every SDCard should show 3 pictures and 1 video of 30s.
> :warning: Never apply a NOR memory bulk erase, as we want every possible telemetry to be saved forever.

### Setting IRIS for launch
This are the recommended commands for testing the camera:
|Command      | Comment     |
|-------------|-------------|
|`conf set flightState 0` | It sets the instrument in standby status, so it will not make videos or pictures unless manually instructed|
|`conf set gopro_leds 0` | It sets the LEDs of the cameras to be off while using them|
|`status`     | It shows the current status |
|`memory status`| It shows the current status of the memories|
|`memory read nor events 0 [x]` | It dumps all the recorded events. [x] is the value of the last recorded event |
|`memory read nor tlm 0 [x]` | It dumps all the recorded telemetry. [x] is the value of the last recorded telemetry. This can take several minutes to dump! |
|`conf`       | It shows the current configuration paramters|
|`conf set flightState 1` | It sets the instrument in waiting for launch status|
|`conf set flightSubState 0` | Just in case|
|`uartdebug 0` | It will disable the debug messages on console |
|`conf set sim_enabled 0` | Just to make sure that simulation is disabled |
|`conf set sim_sunriseSignal 0` | Just to make sure that simulation of the Sunrise Singal is disabled |
|`memory erase fram bulk` | Clean FRAM memory before launch |
|`memory status` | Print memory status for eternity |
|`status` | General status for eternity |
|`conf` | General configuration for eternity |

Just for easier access, here you have the commands, but be careful, change the [x] with the read value on the memory status!!

```console
conf set flightState 0
conf set gopro_leds 0
status
memory status
memory read nor events 0 [x]
memory read nor tlm 0 [x]
conf
conf set flightState 1
conf set flightSubState 0
uartdebug 0
conf set sim_enabled 0
conf set sim_sunriseSignal 0
memory erase fram bulk
memory status
status
conf
```
Now you can safely switch off IRIS to wait for the launch.

## Telemetry
On the folder [telemetry](telemetry) you find the needed scripts to visualize the telemetry in a Grafana dashboard.
<div align="center">
<img src="docs/IRIS2_telemetry.png" alt="Vertical Speed of IRIS2 in grafana"/>
</div>

## Videos of the Flight
Here you can find a set of videos taken by the instrument, including the timelapses:
* [Main video covering the full mission](https://youtu.be/CKWAjiNBPxo)
* [The playlist with all the timelapses](https://www.youtube.com/watch?v=dQEWQV9Ofiw&list=PLlCuSjIHDIh0sF5nbLt4HJ0wI6yLDL5Xi)

## Authors
* Aitor Conde <aitorconde@gmail.com>
* Ramón García <ramongarciaalarcia@gmail.com>

