"""
Autor: Aitor Conde <aitorconde@gmail.com>
It loads an SUNRISE GPS position from the website into an influx database.
"""

import sys
import datetime
import time
import pytz
import os
from influxdb import client as influxdb

def read_file(filename):
    """
    It reads the file and loads it in an array. It ignores the first line that
    they probably have the headers
    """
    
    print ("Reading file " + filename)
    
    trkpts = []
    with open(filename, errors='replace') as f:
        for line in f:
            if line.startswith('<trkpt'):
                parts = line.split('"')
                date_str = parts[11].strip()
                # Parse the date and time
                dt = datetime.datetime.strptime(date_str, "%m/%d/%y %H:%M:%S")
                dt = pytz.utc.localize(dt)
                # Convert to Unix time
                unix_time = int(dt.timestamp())
                print (unix_time, date_str)
                
                trkpt_data = [unix_time, float(parts[1]), float(parts[3]), float(parts[5]) * 0.3048, float(parts[7]) * 1.852, float(parts[9])]
                print (trkpt_data)
                trkpts.append(trkpt_data)
    return trkpts

if __name__ == '__main__':
    db = influxdb.InfluxDBClient("localhost", 8086, "admin", "daedalusdaedalus", "db0")

    if len(sys.argv) != 2:
        print ('ERROR, you should add a folder as argument, python3 loadGPSTrack.py ~/points/')
        exit()
    print ('Argument List:', str(sys.argv))
    filename = sys.argv[1]
    all_data = read_file(filename)
    counter = 0
    linenumber = 0
    for data in all_data:
        
        json_body = [
        {
            "measurement": "position",
            "time": data[0]*1000000000,
            "fields": 
            {
                "latitude": data[1],
                "longitude": data[2],
                "height": data[3],
                "speed": data[4],
                "heading": data[5],
            }
        }
        ]
        print(json_body)
        db.write_points(json_body)
        counter = counter + 1
    print("Process ended, added " + str(counter) + " lines to the database.")



