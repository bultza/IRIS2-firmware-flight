#!/bin/bash

# Define the URL to download
URL="https://www.csbf.nasa.gov/map/balloon11/Track740N.xml"

# Get the current date and time in the format YYYYMMDD_HH:MM:SS
CURRENT_TIME=$(date +"%Y%m%d_%H%M%S")

# Define the filename with the required format
FILENAME="/home/bultza/www/sunrise/tracks/${CURRENT_TIME}_track.xml"

# Use wget to download the file and save it with the specified filename
wget -O "$FILENAME" "$URL"

# Print a message to confirm the download
echo "Downloaded $URL and saved as $FILENAME"