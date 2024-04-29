"""
Autor: Aitor Conde <aitorconde@gmail.com>
It loads an IRIS2 telemetry CSV file into an influx database.
"""

import sys
import datetime
import csv
from csv import reader
from influxdb import client as influxdb



def read_data(filename):
    """
    It reads the file and loads it in an array. It ignores the first line that
    they probably have the headers
    """
    print ("Reading file " + filename)
    with open(filename, errors='replace') as f:
        #lines = f.readlines()[1:]
        lines = f.readlines()

    return lines

if __name__ == '__main__':
    db = influxdb.InfluxDBClient("localhost", 8086, "admin", "daedalusdaedalus", "db0")

    if len(sys.argv) != 2:
        print ('ERROR, you should add a file as argument, python3 loadFile input.csv.')
        exit()
    #print ('Argument List:', str(sys.argv))
    filename = sys.argv[1]
    lines = read_data(filename)
    counter = 0
    linenumber = 0
    for rawline in lines:
        #print (rawline)
        line = rawline.split(",")
        linenumber = linenumber + 1
        #print (len(line))
        if len(line) != 31:
            print("Line " + str(linenumber) + " ignored due to incorrect format: '" + rawline.strip() + "'")
            continue
        if line[0] == "address" and line[1] == "date":
            print("Line " + str(linenumber) + " header telemetry detected")
            continue
        if line[2] == "-1" and line[3] == "-1" :
            print("Line " + str(linenumber) + " ignored because it is empty")
            continue
        
        json_body = [
        {
            "measurement": "tlm",
            "time": int(line[2])*1000000000,
            "fields": 
            {
                "uptime": int(line[3]),
                "pressure": int(line[4]),
                "altitude": int(line[5]),
                "verticalSpeedAVG": int(line[6]),
                "verticalSpeedMAX": int(line[7]),
                "verticalSpeedMIN": int(line[8]),
                "temperatures0": int(line[9]),
                "temperatures1": int(line[10]),
                "temperatures2": int(line[11]),
                "accXAxisAVG": int(line[12]),
                "accXAxisMAX": int(line[13]),
                "accXAxisMIN": int(line[14]),
                "accYAxisAVG": int(line[15]),
                "accYAxisMAX": int(line[16]),
                "accYAxisMIN": int(line[17]),
                "accZAxisAVG": int(line[18]),
                "accZAxisMAX": int(line[19]),
                "accZAxisMIN": int(line[20]),
                "voltagesAVG": int(line[21]),
                "voltagesMAX": int(line[22]),
                "voltagesMIN": int(line[23]),
                "currentsAVG": int(line[24]),
                "currentsMAX": int(line[25]),
                "currentsMIN": int(line[26]),
                "state": int(line[27]),
                "sub_state": int(line[28]),
                "switches_status": int(line[29], 16),
                "errors": int(line[30], 16)
            }
        }
        ]
        #print(json_body)
        db.write_points(json_body)
        counter = counter + 1
    print("Process ended, added " + str(counter) + " lines to the database.")



