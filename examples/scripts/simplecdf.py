import sys
import numpy as np
import matplotlib.pyplot as plt
import pylab as pl
import csv
import math
import statsmodels.api as sm # recommended import according to the docs

data = []
test_type = sys.argv[1]
test_name = ["abt-no-ves","abt-ves"]
host = "haswell-72"

sorted_vals = []
data = []
old_vals = []

for test in test_name:
    filename = "out/" + host + "-" + test_type + "-" + test + ".dat"
    print(filename)
    with open(filename,'r') as csvfile:
        plots = csv.reader(csvfile, delimiter=' ')
        for row in plots:
            data.append(float(row[2]))

    old_vals = data
    sorted_vals = np.sort(old_vals)
    print(sorted_vals)
    ecdf = sm.distributions.ECDF(sorted_vals)

    #dsum = np.sum(sorted_vals);
    #normalized_data = sorted_vals/dsum;

    x = np.linspace(min(sorted_vals), max(sorted_vals),1000)
    y = ecdf(x)
    plt.step(x, y, label=test_type + "-" + test)

plt.legend()
#plt.yticks(np.arange(min(sorted_vals), max(sorted_vals), step=0.0001))
#plt.yscale('log')
plt.ylabel('Frequency')
plt.xlabel('Fork-join Time Per ES')
plt.grid(True, which='both')
plt.savefig(test_type, dpi=300)
