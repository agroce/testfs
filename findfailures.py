import subprocess
import sys
import glob

corpus = sys.argv[1]

cmd = "./Tests --input_test_file "

crashes = {}
fatals = {}

for f in glob.glob(corpus + "/*"):
    with open("test.out", 'w') as outf:
        subprocess.call([cmd + f], shell=True, stdout=outf, stderr=outf)
    crashline = None
    fatalline = None
    stepcount = 0
    with open("test.out", 'r') as inf:
        for line in inf:
            if "Crash" in line:
                crashline = line
            if "FATAL" in line:
                fatalline = line
            if "STEP" in line:
                stepcount += 1
    if crashline is not None:
        if crashline not in crashes:
            crashes[crashline] = (stepcount, f)
        else:
            if stepcount < crashes[crashline][0]:
                crashes[crashline] = (stepcount, f)
    if fatalline is not None:
        if fatalline not in fatals:
            fatals[fatalline] = (stepcount, f)
        else:
            if stepcount < fatals[fatalline][0]:
                fatals[fatalline] = (stepcount, f)
for fatal in fatals:
    print fatal, fatals[fatal]
for crash in crashes:
    print crash, crashes[crash]    
        
