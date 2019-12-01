import sys
import numpy as np
import matplotlib.pyplot as plt
import pylab as pl
import csv
import math
import statsmodels.api as sm # recommended import according to the docs

data = []
test_type = sys.argv[1]
test_name = ["abt-ves","abt-no-ves"]
host = "haswell-72"

for test in test_name:
    filename = "out/" + host + "-" + test_type + "-" + test + ".dat"

    with open(filename,'r') as csvfile:
        plots = csv.reader(csvfile, delimiter=' ')
        for row in plots:
            data.append(float(row[1]))

    old_vals = data[0::100]
    ecdf = sm.distributions.ECDF(old_vals)

    dsum = np.sum(old_vals);
    normalized_data = old_vals/dsum;

    x = np.linspace(min(old_vals), max(old_vals),100)
    print(x)
    y = ecdf(x)
    plt.step(x, y, label=test_type + "-" + test)

plt.legend()
plt.savefig(test_type, dpi=300)
plt.grid(True)
