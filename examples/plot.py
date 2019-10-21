#!/usr/bin/python3

import sys
import matplotlib.pyplot as plt
import numpy as np
import pylab as pl
import csv
import math
import sys
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
            filename = host + "-" + test_type + "-" + test + ".dat"        
            print(filename)
            p = []
            q = []
            with open(filename) as csvfile:
                plots = csv.reader(csvfile, delimiter=' ')
                for row in plots:
                    p.append(float(row[0]))
                    q.append(float(row[2]))
            i += 1;
            for j in range(0,len(p)):
                x.append(mean(p[j:j+10]))
                y.append(q[j])
                j = j+10;
        plotname = test + "-" + test_type
        for j in range(0,i):
            xax = np.arange(len(x[j]))
            plt.plot(x[j], y[j], next(linecycler), label=testname_print[j])

        plt.xlabel("latency (cycles)")
        plt.ylabel(yname)
        plt.xscale('log')
        #plt.xticks(np.arange(len(num_threads)), labels=num_threads)
        plt.grid(True)
        plt.legend()
        plt.savefig(plotname, dpi=300)
        i = 0;
        x = []
        y = []
        plt.close()


if __name__ == "__main__":
    main(sys.argv)

