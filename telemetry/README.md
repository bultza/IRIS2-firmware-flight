# IRIS2 telemetry scripts
You will need these scrips to show and understand the telemetry and events of the outputs produced by the IRIS 2 instrument.

## How to install
You need python3 to load the raw telemetry CSV file into an influxDB database. Grafana is used to display the telemetry. Instructions for Linux are provided.

### Commands in Linux
Install python:
```console
sudo apt install python3
```
Install pip3:
```console
sudo apt install python3-pip
```
Install influxDB python interface
```console
pip3 install influxdb
pip3 install -r requirements.txt
```
Install influxDB
```console
sudo apt-get install influxdb
sudo service influxdb start
```
Install Grafana
```console
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
sudo add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
sudo apt update
sudo apt install grafana
sudo systemctl start grafana-server
sudo systemctl status grafana-server
sudo systemctl enable grafana-server
```
Create database:
```console
influx
CREATE DATABASE db0
```

## Usage
There are 3 different scripts:
 * To load telemetry on the database.
 * To show graphs of the telemetry without database
 * To convert the events files into human readable events

### Loading telemetry to grafana
Load the CSV file into the database
```console
python3 loadFile.py 20211120_tlm.csv
```
Connect to grafana at http://localhost:3000/ Click on the "+" button and Import from JSON, use the IRIS2_dashboard.json file

Probably you need to configure the database access from the left configuration arrow, datasources, usually the default values are enough.

<div align="center">
<img src="../docs/IRIS2_telemetry.png" alt="Vertical Speed of IRIS2 in grafana" />
</div>

### Converting the event log file to human readable
After dumping the event log into a CSV file, run the following command:
```console
python3 translateEvents.py 20220220_AfterTVAC_putty_events_nor.csv
```
A display as follows will appear:
```
Reading file '20220220_AfterTVAC_putty_events_nor.csv'
2022/02/19 16:54:14.599 (0 days 00:03:14.599): FLIGHTSTATE_WAITFORLAUNCH   , [0], Power off detected? Current was -6mA, voltage 8.76V
2022/02/19 16:54:19.016 (0 days 00:03:18.016): FLIGHTSTATE_WAITFORLAUNCH   , [0], Power off detected? Current was -5mA, voltage 8.76V
================================================================================
2022/02/19 16:54:21.246 (0 days 00:00:00.246): FLIGHTSTATE_DEBUG           , [0], Booting up with firmware version '7', reason was 'WDTPW password violation (PUC)'
2022/02/19 16:54:22.777 (0 days 00:00:01.777): FLIGHTSTATE_WAITFORLAUNCH   , [0], Power off detected? Current was -2mA, voltage 8.75V
2022/02/19 16:54:27.065 (0 days 00:00:06.065): FLIGHTSTATE_WAITFORLAUNCH   , [0], Power off detected? Current was -1mA, voltage 8.76V
================================================================================
2022/02/19 16:50:58.011 (0 days 00:00:00.011): FLIGHTSTATE_DEBUG           , [0], Booting up with firmware version '7', reason was 'Brownout (BOR)'
2022/02/19 16:51:00.724 (0 days 00:00:03.724): FLIGHTSTATE_WAITFORLAUNCH   , [0], Power off detected? Current was -1mA, voltage 8.26V
================================================================================
2022/02/19 18:04:44.011 (0 days 00:00:00.011): FLIGHTSTATE_DEBUG           , [0], Booting up with firmware version '7', reason was 'Brownout (BOR)'
2022/02/19 18:24:15.763 (0 days 00:19:31.763): FLIGHTSTATE_WAITFORLAUNCH   , [0], Power off detected? Current was -1mA, voltage 8.18V
================================================================================
2022/02/19 18:30:09.011 (0 days 00:00:00.011): FLIGHTSTATE_DEBUG           , [0], Booting up with firmware version '7', reason was 'Brownout (BOR)'
2022/02/19 19:06:08.532 (0 days 00:35:60.532): FLIGHTSTATE_LAUNCH          , [0], Flight sequence changed to 'FLIGHTSTATE_LAUNCH' due to altitude over the height threshold.
2022/02/19 19:06:23.563 (0 days 00:36:15.563): FLIGHTSTATE_LAUNCH          , [1], Camera '1' started recording video with a duration of 1s in 'HighSpeed 960p/Wide' mode.
2022/02/19 19:07:24.760 (0 days 00:37:16.760): FLIGHTSTATE_LAUNCH          , [2], Camera '1' ended recording video.
2022/02/19 19:08:21.452 (0 days 00:38:12.452): FLIGHTSTATE_LAUNCH          , [3], Camera '0' ended recording video.
2022/02/19 19:08:26.665 (0 days 00:38:18.665): FLIGHTSTATE_TIMELAPSE       , [0], Flight sequence changed to 'FLIGHTSTATE_TIMELAPSE'
2022/02/19 19:08:41.699 (0 days 00:38:33.699): FLIGHTSTATE_LANDING         , [1], Camera '2' started recording video with a duration of 1s in 'HighSpeed 960p/Wide' mode.
2022/02/19 19:09:29.864 (0 days 00:39:21.864): FLIGHTSTATE_LANDING         , [2], Camera '0' ended recording video.
2022/02/19 19:36:00.593 (0 days 01:05:52.593): FLIGHTSTATE_LANDING         , [3], IRIS2 is below landing threshold! Measured 2999.82m.
```

### Displaying graphs without grafana
> :warning: With the following two scripts you have to modify the code to display other telemetry values than those shown by default.

You can also display different graphs using mathplot and not installing grafana. After dumping the telemetry of IRIS into a CSV file do, run the following command:
```console
python3 showGraph.py 20220214_baroTest_tlm.csv
```
There is an alternative using pandas:
```console
python3 showGraphTelemetry.py 20220214_baroTest_tlm.csv
```

## Docker Alternative
I tried to install grafana and influx with docker-compose unsuccessfully, access permisions need to be fixed between dockers to make them work
