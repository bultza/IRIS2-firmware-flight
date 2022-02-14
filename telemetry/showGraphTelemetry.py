import csv
import matplotlib.pyplot as plt
import sys
import pandas as pd

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
    data = pd.read_csv(sys.argv[1])
    print(data)
    newplot = plt.figure(1)
    data["pressure"].plot(x='date')
    newplot = plt.figure(2)
    data["altitude"].plot(x='date')
    plt.show()


if __name__ == "__main__":
    main()