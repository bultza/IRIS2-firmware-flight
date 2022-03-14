# How to install
You need python3 to load the raw telemetry CSV file into an influxDB database. Grafana is used to display the telemetry. Instructions for Linux are provided.

## Commands in Linux
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
Load the CSV file into the database
```console
python3 loadFile.py 20211120_tlm.csv
```
Connect to grafana at http://localhost:3000/ Click on the "+" button and Import from JSON, use the IRIS2_dashboard.json file

Probably you need to configure the database access from the left configuration arrow, datasources, usually the default values are enough

## Docker Alternative
I tried to install grafana and influx with docker-compose unsuccessfully, access permisions need to be fixed between dockers to make them work
