#!/usr/bin/python3

import sys
import matplotlib.pyplot as plt
import numpy as np
import pylab as pl
import csv
import math
from statistics import mean
from itertools import cycle

def main(args):
    lines = ['k-o', 'k-.'] #, 'co--', 'mo--', 'ro--', 'yo--', 'gv--', 'bv--', 'cv--', 'mv--', 'rv--', 'yv--', 'g*--', 'b*--', 'c*--', 'm*--', 'r*--', 'y*--']    
    linecycler = cycle(lines)
    
    hostname = ["haswell-72"] #, "arm-tx-96", "ibm-power9"]
    w = len(hostname)
    h = 30
    #x = [[0 for x in range(w)] for y in range(h)] 
    x = []
    #y = [[0 for x in range(w)] for y in range(h)] 
    y = []
    yerr = []
    #num_threads=["72","144","216","288","576","1152","2592","5184","10368"]
    num_threads=["72","144","288","576","1152","2592","5184","10368","20736"] #,"41472","72000"]
    index = np.arange(len(num_threads))
    print(index)
    test_name=["abt-ves", "abt-no-ves"]
    testname_print=["abt-ves", "abt-no-ves"]
    test_type=sys.argv[1]
    xcol = int(sys.argv[2])
    bar_width = 0.35
    yname = "Execution Time (Seconds)"
    #x1 = int(sys.argv[2]);
    #y1 = int(sys.argv[3]);
    i = 0
    step = 10
    for host in hostname:
        for test in test_name:
            filename = "out/" + host + "-" + test_type + "-" + test + ".dat"        
            print(filename)
            p = []
            q = []
            plotx = []
            ploty = []
            plotyerr = []
            with open(filename) as csvfile:
                plots = csv.reader(csvfile, delimiter=' ')
                for row in plots:
                    p.append(int(row[xcol]))
                    q.append(float(row[2]))
            i += 1;
            for j in range(0,len(p), 10):
                plotx.append(p[j])
                ploty.append(float(mean(q[j:j+step])))
                plotyerr.append(float(np.std(q[j:j+step])))
            x.append(plotx)
            y.append(ploty)
            yerr.append(plotyerr)
        plotname = test_type
        print(x)
        print(y)
        print(yerr)
        fig, ax = plt.subplots()
        change = -1 * bar_width/2
        for j in range(0,i):
            plt.bar(index + change, y[j], yerr = yerr[j], width=0.35, label=test_type + "-" + testname_print[j])
            change = -1 * change
            #plt.errorbar(x[j], y[j], yerr=yerr[j], width=width, label=test_type + "-" + testname_print[j])

        plt.xlabel("#ULTs (#ES = 72)")
        plt.ylabel(yname)
        #plt.xscale('log')
        if test_type == "strassen":
            plt.xticks(np.arange(min(x[0]), max(x[0])+1, 5000))
        else:
            plt.xticks(index, labels=num_threads)
            #plt.xticks(np.arange(min(x[0]), max(x[0])+1, 72))
        plt.grid(True)
        fig.tight_layout()
        plt.legend()
        plt.savefig(plotname, dpi=300)
        i = 0;
        x = []
        y = []
        plt.close()


if __name__ == "__main__":
    main(sys.argv)

