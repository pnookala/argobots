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
    lines = ['k-', 'k-.'] #, 'co--', 'mo--', 'ro--', 'yo--', 'gv--', 'bv--', 'cv--', 'mv--', 'rv--', 'yv--', 'g*--', 'b*--', 'c*--', 'm*--', 'r*--', 'y*--']    
    linecycler = cycle(lines)
    
    hostname = ["haswell-72"] #, "arm-tx-96", "ibm-power9"]
    w = len(hostname)
    h = 30
    #x = [[0 for x in range(w)] for y in range(h)] 
    x = []
    #y = [[0 for x in range(w)] for y in range(h)] 
    y = []
    num_threads=[72,144,216,288]
    test_name=["abt-ves", "abt-no-ves"]
    testname_print=["abt-ves", "abt-no-ves"]
    test_type=sys.argv[1]

    yname = "Execution Time (Seconds)"

    #x1 = int(sys.argv[2]);
    #y1 = int(sys.argv[3]);
    i = 0;
    for host in hostname:
        for test in test_name:
            filename = "out/" + host + "-" + test_type + "-" + test + ".dat"        
            print(filename)
            p = []
            q = []
            plotx = []
            ploty = []
            with open(filename) as csvfile:
                plots = csv.reader(csvfile, delimiter=' ')
                for row in plots:
                    if test_type == "strassen":
                        p.append(int(row[1]))
                    else:
                        p.append(int(row[0]))
                    q.append(int(row[1]))
            i += 1;
            #for j in range(0,len(p), 10):
            #    plotx.append(p[j])
            #    ploty.append(mean(q[j:j+10]))
            x.append(p)
            y.append(q)
        plotname = test_type
        for j in range(0,i):
            plt.plot(x[j], y[j], next(linecycler), label=test_type + "-" + testname_print[j])

        plt.xlabel("#ESs")
        plt.ylabel(yname)
        if test_type == "strassen":
            plt.xticks(np.arange(min(x[0]), max(x[0])+1, 5000))
        else:
            plt.xticks(np.arange(min(x[0]), max(x[0])+1, 72))
        plt.grid(True)
        plt.legend()
        plt.savefig(plotname, dpi=300)
        i = 0;
        x = []
        y = []
        plt.close()


if __name__ == "__main__":
    main(sys.argv)

