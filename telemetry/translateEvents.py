import csv
import matplotlib.pyplot as plt
import sys
import pandas as pd



def read_data(filename):
    """
    It reads the file and loads it in an array. It ignores the first line that
    they probably have the headers
    """
    print("Reading file '" + filename + "'")
    with open(filename) as f:
        #Dont read the first line (headers)
        lines = f.readlines()[1:]
    return lines

def prntime(ms):
    """
    Convert milliseconds to human readable string
    """
    s=ms/1000
    m,s=divmod(s,60)
    h,m=divmod(m,60)
    d,h=divmod(h,24)
    return str(int(d)) + " days " + '{:0>2.0f}'.format(h) + ":" \
                       + '{:0>2.0f}'.format(m) + ":" + '{:0>2.0f}'.format(s) \
                       + "." + '{:0>3.0f}'.format(ms%1000)

def translateFlightSequence(code, tab):
    """ 
    It translates the flight status
    """
    if tab == 1:
        if code == "0":
            return "FLIGHTSTATE_DEBUG           "
        elif code == "1":
            return "FLIGHTSTATE_WAITFORLAUNCH   "
        elif code == "2":
            return "FLIGHTSTATE_LAUNCH          "
        elif code == "3":
            return "FLIGHTSTATE_TIMELAPSE       "
        elif code == "4":
            return "FLIGHTSTATE_LANDING         "
        elif code == "5":
            return "FLIGHTSTATE_TIMELAPSE_LAND  "
        elif code == "6":
            return "FLIGHTSTATE_RECOVERY        "
        else:
            return "FLIGHTSTATE_UNKNOWN(" + code + ")  "
    else:
        if code == "0":
            return "FLIGHTSTATE_DEBUG"
        elif code == "1":
            return "FLIGHTSTATE_WAITFORLAUNCH"
        elif code == "2":
            return "FLIGHTSTATE_LAUNCH"
        elif code == "3":
            return "FLIGHTSTATE_TIMELAPSE"
        elif code == "4":
            return "FLIGHTSTATE_LANDING"
        elif code == "5":
            return "FLIGHTSTATE_TIMELAPSE_LAND"
        elif code == "6":
            return "FLIGHTSTATE_RECOVERY"
        else:
            return "FLIGHTSTATE_UNKNOWN(" + code + ")"

def translateFlightSubSequence(code):
    """ 
    It translates the flight Sub status
    """
    return "[" + code + "]"

def translateBootReason(code1, code2):
    """ 
    It translates the reboot reasons for the MSP430
    """
    code = int(code1) + int(code2) * 255
    if code == 0x00:
        return "Clean Start"
    elif code ==  0x02:
        return "Brownout (BOR)"
    elif code ==  0x04:
        return "RSTIFG RST/NMI (BOR)"
    elif code ==  0x06:
        return "PMMSWBOR software BOR (BOR)"
    elif code ==  0x0a:
        return "Security violation (BOR)"
    elif code ==  0x0e:
        return "SVSHIFG SVSH event (BOR)"
    elif code ==  0x14:
        return "PMMSWPOR software POR (POR)"
    elif code ==  0x16:
        return "WDTIFG WDT timeout (PUC)"
    elif code ==  0x18:
        return "WDTPW password violation (PUC)"
    elif code ==  0x1a:
        return "FRCTLPW password violation (PUC)"
    elif code ==  0x1c:
        return "Uncorrectable FRAM bit error detection (PUC)"
    elif code ==  0x1e:
        return "Peripheral area fetch (PUC)"
    elif code ==  0x20:
        return "PMMPW PMM password violation (PUC)"
    elif code ==  0x22:
        return "MPUPW MPU password violation (PUC)"
    elif code ==  0x24:
        return "CSPW CS password violation (PUC)"
    elif code ==  0x26:
        return "MPUSEGIPIFG encapsulated IP memory segment violation (PUC)"
    elif code ==  0x28:
        return "MPUSEGIIFG information memory segment violation (PUC)"
    elif code ==  0x2a:
        return "MPUSEG1IFG segment 1 memory violation (PUC)"
    elif code ==  0x2c:
        return "MPUSEG2IFG segment 2 memory violation (PUC)"
    elif code ==  0x2e:
        return "MPUSEG3IFG segment 3 memory violation (PUC)"
    else:
        return "Unknown"


def translateEvent(code, payload1, payload2, payload3, payload4, payload5):
    """ 
    It translates the event code and its payloads
    """
    payload5 = payload5.split('\n')
    if code == "69":
        reason = translateBootReason(payload1, payload2)
        return "Booting up with firmware version '" + payload5[0] \
             + "', reason was '" + reason + "'"
    elif code == "1":
        parameter = chr(int(payload1)) \
                  + chr(int(payload2)) \
                  + chr(int(payload3)) \
                  + chr(int(payload4)) 
        return "Changing configuration '" + parameter + "' with value '" + payload5[0] + "'"  
    elif code == "2":
        return "EVENT_NOR_CLEAN"
    elif code == "10":
        return "Camera '" + payload1 + "' was manually switched on."
    elif code == "11":
        return "Camera '" + payload1 + "' took a picture manually."
    elif code == "12":
        duration = int(payload4) + int(payload5[0]) * 255
        if payload2 == "1":
            mode = "Normal 2.7k/4:3"
        elif payload2 == "2":
            mode = "HighSpeed 960p/Wide"
        else:
            mode = "Picture"
        return "Camera '" + payload1 + "' started recording video with a "\
             + "duration of " + str(duration) + "s in '" + mode + "' mode."
    elif code == "13":
        return "Camera '" + payload1 + "' ended recording video."
    elif code == "14":
        return "Camera '" + payload1 + "' was manually switched off."
    elif code == "15":
        return "Camera '" + payload1 + "' took a picture (in timelapse mode)."
    elif code == "16":
        return "Camera '" + payload1 + "' configuration changed to video mode manually."
    elif code == "17":
        return "Camera '" + payload1 + "' configuration changed to picture mode manually."
    elif code == "18":
        return "Camera '" + payload1 + "' video was interrupted."
    elif code == "20":
        event = translateFlightSequence(payload1 , 0)
        #TODO Event 
        """
            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_LAUNCH;
            payload[1] = getAltitude() > (confRegister_.launch_heightThreshold * 100);
            payload[2] = getVerticalSpeed() > (confRegister_.launch_climbThreshold * 100);
            payload[3] = sunriseGpioSignal;
            saveEventSimple(EVENT_STATE_CHANGED, payload);

            uint8_t payload[5] = {0};
            payload[0] = FLIGHTSTATE_LANDING;
            payload[1] = getAltitude() < (confRegister_.landing_heightThreshold * 100);
            payload[2] = verticalSpeed < (confRegister_.landing_speedThreshold * 100);
            payload[3] = sunriseGpioSignal;
            saveEventSimple(EVENT_STATE_CHANGED, payload);

        """


        return "Flight sequence changed to '" + event + "'"
    elif code == "30":
        altitude = int(payload1) \
                 + int(payload2) * 255 \
                 + int(payload3) * 65535 \
                 + int(payload4) * 16777215
        return "IRIS2 is below landing threshold! Measured " + "{:.2f}".format(altitude/100) + "m."
    elif code == "40":
        return "Movement detected by Accelerometer!"
    elif code == "99":
        return "EVENT_I2C_ERROR_RESET"
    elif code == "100":
        return "SUNRISE GPIO Changed!"
    elif code == "101":
        return "SUNSIRE Signal activation detected!"
    elif code == "200":
        current = int(payload1) + int(payload2) * 255
        voltage = int(payload3) + int(payload4) * 255
        if current > 32767:
            current = current - 65536 + 255
        return "Power off detected? Current was " + str(current) + "mA, voltage " + "{:.2f}".format(voltage/100) + "V"
    else:
        return "EVENT_UNKNOWN (" + code + ")"

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print ('ERROR, you should add a file as argument, python3 loadFile input.csv.')
        exit()

    filename = sys.argv[1]
    
    lines = read_data(filename)
    counter = 0
    for rawline in lines:
        #print (rawline)
        line=rawline.split(",")
        #print (line)
        #Processed line:
        uptime_ms = int(line[3])
        if(line[6] == "69"):
            print("================================================================================")
        finalLine = line[1] + "." + '{:0>3}'.format(uptime_ms%1000) \
                    + " (" + prntime(uptime_ms) + "): " \
                    + translateFlightSequence(line[4], 1) + ", " \
                    + translateFlightSubSequence(line[5]) + ", " \
                    + translateEvent(line[6], \
                                    line[7], \
                                    line[8], \
                                    line[9], \
                                    line[10], \
                                    line[11])

        print(finalLine)



        #if counter == 5:
        #    exit()
        
        counter = counter + 1
    
    print("Process ended with " + str(counter) + " events")





