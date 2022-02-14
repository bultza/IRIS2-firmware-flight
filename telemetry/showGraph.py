import csv
import matplotlib.pyplot as plt
import sys

def showGraph(index, header, x, y, axisName, colorLine):
    """Creates a general graph based on the number of parameters"""
    newplot = plt.figure(index)
    plt.plot(x, y, color = colorLine, linestyle = 'dashed',
             marker = 'o',label = header)
    plt.xticks(rotation = 25)
    plt.xlabel('Date')
    plt.ylabel(axisName)
    plt.title(header)
    plt.grid()
    plt.legend()


def main():

    print("Reading file '" + (sys.argv[1]) + "'")
    x = []
    pressure = []
    temperatures = []
    headers = []
    altitude = []
      
    with open(sys.argv[1],'r') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        #Read header
        headers = next(reader, None)
        print(headers)
        #Read the data:
        for row in reader:
            x.append(int(row[2]))
            pressure.append(int(row[4])/100.0)
            altitude.append(int(row[5])/10.0)
            temperatures.append(int(row[10])/10.0)

      
    # plot1 = plt.figure(1)

    # plt.plot(x, y, color = 'g', linestyle = 'dashed',
    #          marker = 'o',label = "Pressure")

    # plt.xticks(rotation = 25)
    # plt.xlabel('Date')
    # plt.ylabel('mbar')
    # plt.title('Pressure')
    # plt.grid()
    # plt.legend()

    # plot2 = plt.figure(2)
    # plt.plot(x, temperatures, color = 'r', linestyle = 'dashed',
    #          marker = 'o',label = "Temperature")
      
    showGraph(1, headers[4], x, pressure, headers[4], 'g')
    showGraph(2, headers[5], x, altitude, headers[5], 'b')
    showGraph(3, headers[10], x, temperatures, headers[4], 'r')

    plt.show()

if __name__ == "__main__":
    main()