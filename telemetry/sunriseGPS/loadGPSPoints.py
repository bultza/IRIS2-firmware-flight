"""
Autor: Aitor Conde <aitorconde@gmail.com>
It loads an SUNRISE GPS position from the website into an influx database.
"""

import sys
import datetime
import time
import pytz
import os
import re
from influxdb import client as influxdb

# Function to convert latitude string to a double number
def parse_latitude(lat_str):
    # Remove the <CENTER> tags and trim whitespace
    lat_str = lat_str.replace('<CENTER>', '').replace('</CENTER>', '').strip()
    # Remove the 'Latitude:' prefix
    lat_str = lat_str.replace('Latitude:', '').strip()
    # Extract the degrees, minutes, and direction
    degree_index = lat_str.index('�')
    degrees = float(lat_str[:degree_index])
    remainder = lat_str[degree_index+1:].strip()  # Skip past the degree symbol
    minutes = float(remainder[:-2].strip())  # Remove the direction (N/S) and strip whitespace
    direction = remainder[-1]  # Last character is the direction (N/S)
    
    # Convert to decimal
    decimal_lat = degrees + minutes / 60
    # Adjust for direction
    if direction == 'S':
        decimal_lat = -decimal_lat
    
    return decimal_lat
    
# Function to convert latitude string to a double number
def parse_longitude(lat_str):
    # Remove the <CENTER> tags and trim whitespace
    lat_str = lat_str.replace('<CENTER>', '').replace('</CENTER>', '').strip()
    # Remove the 'Latitude:' prefix
    lat_str = lat_str.replace('Longitude:', '').strip()
    # Extract the degrees, minutes, and direction
    degree_index = lat_str.index('�')
    degrees = float(lat_str[:degree_index])
    remainder = lat_str[degree_index+1:].strip()  # Skip past the degree symbol
    minutes = float(remainder[:-2].strip())  # Remove the direction (N/S) and strip whitespace
    direction = remainder[-1]  # Last character is the direction (N/S)
    
    # Convert to decimal
    decimal_lat = degrees + minutes / 60
    # Adjust for direction
    if direction == 'W':
        decimal_lat = -decimal_lat
    
    return decimal_lat

# Function to convert altitude string to meters
def parse_altitude(alt_str):
    # Remove the <CENTER> tags and trim whitespace
    alt_str = alt_str.replace('<CENTER>', '').replace('</CENTER>', '').strip()
    # Remove the 'Altitude:' prefix
    alt_str = alt_str.replace('Altitude:', '').replace('Feet', '').strip()
    # Convert feet to meters (1 foot = 0.3048 meters)
    altitude_meters = float(alt_str) * 0.3048
    return altitude_meters
    
# Function to convert speed and heading string
def parse_speed_and_heading(speed_str):
    # Remove the <CENTER> tags and trim whitespace
    speed_str = speed_str.replace('<CENTER>', '').replace('</CENTER>', '').strip()
    # Extract the speed and heading
    parts = speed_str.split('@')
    speed_knots = float(parts[0].replace('Knots', '').strip())
    heading_degrees = float(parts[1].replace('�', '').strip())
    # Convert speed from knots to km/h (1 knot = 1.852 km/h)
    speed_kmph = speed_knots * 1.852
    return speed_kmph, heading_degrees

def read_file(filename):
    """
    It reads the file and loads it in an array. It ignores the first line that
    they probably have the headers
    """
    # print ("Reading file " + filename)
    with open(filename, errors='replace') as f:
        #lines = f.readlines()[1:]
        lines = f.readlines()
        if len(lines) >= 17:
        
            date_str = lines[11].strip()
            # Extract the part of the date string that contains the actual date and time
            date_str = date_str.replace('<CENTER>', '').replace('</CENTER>', '').strip()
            # Parse the date and time
            dt = datetime.datetime.strptime(date_str, "%H:%M:%SZ %m/%d/%y")
            dt = pytz.utc.localize(dt)
            # Convert to Unix time
            unix_time = int(dt.timestamp())
            # print (unix_time, lines[11])
            
            latitude_str = lines[13].strip()
            latitude = parse_latitude(latitude_str)
            # print (latitude, latitude_str)
            
            longitude_str = lines[14].strip()
            longitude = parse_longitude(longitude_str)
            # print (longitude, longitude_str)
            
            altitude_str = lines[15].strip()
            altitude = parse_altitude(altitude_str)
            # print (altitude, altitude_str)
            
            speed_and_heading_str = lines[16].strip()
            speed, heading = parse_speed_and_heading(speed_and_heading_str)
            # print (speed, heading, speed_and_heading_str)
            
            return [unix_time, latitude, longitude, altitude, speed, heading]

    return


    
def read_data(folder):
    """
    It searches all the files and reads the important data of each file.
    """
    print ("Reading folder " + folder)
    files = os.listdir(folder)
    txt_files = [file for file in files if file.endswith('.htm')]
    sorted_txt_files = sorted(txt_files)
    # Print the sorted file names
    all_data_array = []
    last_data = [1,1,1,1,1,1]
    for file in sorted_txt_files:
        #lines += read_file(file)
        data = read_file(folder + "/" + file)
        if data:
            if last_data[1] == data[1] and last_data[2] == data[2] and last_data[3] == data[3] and last_data[4] == data[4] and last_data[5] == data[5]:
                print("Ignoring last duplicated data")
            else:                
                all_data_array.append(data)
                last_data = data
        else:
            print("Incorrect data on file", file)

    return all_data_array

if __name__ == '__main__':
    db = influxdb.InfluxDBClient("localhost", 8086, "admin", "daedalusdaedalus", "db0")

    if len(sys.argv) != 2:
        print ('ERROR, you should add a folder as argument, python3 loadGPSPoint.py ~/points/')
        exit()
    print ('Argument List:', str(sys.argv))
    filename = sys.argv[1]
    all_data = read_data(filename)
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
        #print(json_body)
        db.write_points(json_body)
        counter = counter + 1
    print("Process ended, added " + str(counter) + " lines to the database.")



