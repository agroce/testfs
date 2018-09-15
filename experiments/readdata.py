import scipy
import scipy.stats
import csv
import glob
import math
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

names = map(lambda x:x.split(".")[0], glob.glob("*.0.libfuzzer.data"))

allValues = {}

for n in names:
    allValues[n] = {}
    for f in glob.glob(n + "*.libfuzzer.data"):
        e = int(f.split(".")[1])
        allValues[n][e] = {}
        with open(f, 'r') as data:
            reader = csv.DictReader(data)
            for row in reader:
                allValues[n][e][int(row['time'])] = row

for n in names:
    t3600 = []
    for e in allValues[n]:
        t3600.append(int(allValues[n][e][3600]['coverage']))
    print n, t3600, scipy.mean(t3600), scipy.median(t3600)

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

f1 = plt.figure()

plt.xlim(0,3600)
plt.ylim(1650,1800)

colors = {}
colors['dictvalue'] = 'black'
colors['nodictvalue'] = 'blue'
colors['dictnovalue'] = 'green'
colors['nodictnovalue'] = 'red'

for n in names:
    for t in sorted(allValues[n][0]):
        values = []
        for e in allValues[n]:
            values.append(int(allValues[n][e][t]['coverage']))
        conf = scipy.stats.norm.interval(0.95,loc=scipy.mean(values),scale=scipy.std(values)/math.sqrt(len(values)))
        #print n,t,conf, values
        plt.errorbar(t, scipy.mean(values), yerr=conf[1]-scipy.mean(values), color=colors[n])

f1.savefig("libfuzzer.png")

