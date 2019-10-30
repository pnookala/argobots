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
    lines = ['go-', 'b*--', 'cv-.', 'md:', 'rs-', 'y+-', 'g+--', 'b+:', 'c+-.', 'm*-', 'r*:', 'y.-', 'g.:']
    linecycler = cycle(lines)
    
    hostname = ["haswell-72"] #, "arm-tx-96", "ibm-power9"]
    w = len(hostname)
    h = 30
    #x = [[0 for x in range(w)] for y in range(h)] 
    x = []
    #y = [[0 for x in range(w)] for y in range(h)] 
    y = []
    yerr = []
    num_ess=[1,2,4,8,16,24,32,48,64,72]
    test_name=["abt-ves"] #["abt-ves", "switch10-abt-ves", "switch50-abt-ves", "switch50-with1ult-abt-ves"] #, "abt-no-ves"]
    testname_print=["1 vES", "2 vESs","4 vESs", "8 vESs", "16 vESs", "24 vESs", "32 vESs", "48 vESs", "64 vESs", "72 vESs"] #["switch-1", "switch-10", "switch-50", "switch-50-with1ult"] #, "abt-no-ves"]
    test_type=sys.argv[1]
    step=10
    yname = "Execution Time (Seconds)"

    #x1 = int(sys.argv[2]);
    #y1 = int(sys.argv[3]);
    i = 0;
    for host in hostname:
        for test in test_name:
            for ess in num_ess:
                filename = "out/" + host + "-" + test_type + "-" + str(ess) + "-"  + test + ".dat"        
                print(filename)
                p = []
                q = []
                plotx = []
                ploty = []
                plotyerr = []
                with open(filename) as csvfile:
                    plots = csv.reader(csvfile, delimiter=' ')
                    for row in plots:
                            p.append(int(row[0]))
                            q.append(float(row[3]))
                i += 1;
                x.append(p)
                y.append(q)
            plotname = test_type
            print(x)
            print(y)
        for j in range(0,i):
            plt.plot(x[j], y[j], next(linecycler), label="strassen-" + testname_print[j])

        plt.xlabel("Total #vESs (recursive strassen + matrix mul at leaves)")
        plt.ylabel(yname)
        plt.xscale('log')
        plt.yscale('log')
        plt.grid(True)
        plt.legend(fontsize="small")
        plt.savefig(plotname, dpi=300)
        i = 0;
        x = []
        y = []
        plt.close()


if __name__ == "__main__":
    main(sys.argv)

