The bash scripts were polling the data from the https://www.csbf.nasa.gov/sweden/payloads.html website in realtime. Crontab was used in order to pool the data every 1 minute or 15minutes depending. In theory only the points are needed, but the tracks was a nice addition in case you miss recording since the very beginning.

The crontab for the scripts to log the GPS data in a folder was:

```crontab
*/1 * * * * /path_to_script/bul_pointSnapshot.sh 2>&1
*/15 * * * * /path_to_script/bul_trackSnapshot.sh 2>&1
```

The python scripts are used to get the data from the downloaded files into the influx database.
