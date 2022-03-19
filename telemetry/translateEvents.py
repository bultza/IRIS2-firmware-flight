import csv
import matplotlib.pyplot as plt
import sys
import pandas as pd



def read_data(filename):
    print("Reading file '" + filename + "'")
    with open(filename) as f:
        #Dont read the first line (headers)
        lines = f.readlines()[1:]
    return lines

def prntime(ms):
    s=ms/1000
    m,s=divmod(s,60)
    h,m=divmod(m,60)
    d,h=divmod(h,24)
    return str(int(d)) + " days " + str(int(h)) + ":" + str(int(m)) + ":" + str(int(s)) + "." + str(int(ms%1000))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print ('ERROR, you should add a file as argument, python3 loadFile input.csv.')
        exit()

    filename = sys.argv[1]
    
    lines = read_data(filename)
    counter = 0
    for rawline in lines:
        print (rawline)
        line=rawline.split(",")
        #print (line)
        #Processed line:
        uptime_ms = int(line[3])
        finalLine = line[1] + "." + '{:0>2}'.format(3.456) + ": " + prntime(uptime_ms)
        print(finalLine)



        if counter == 5:
            exit()
        
        counter = counter + 1
    
    print("Process ended, added " + str(counter) + " lines to the database")





