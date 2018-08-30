import subprocess
import os
import sys
import time

prefix = sys.argv[1]
total_time = sys.argv[2]
lfoptions = sys.argv[3:]

runs = 0
total_execs = 0

with open(os.devnull,'w') as dnull:
    subprocess.call(["rm -rf " + prefix + ".corpus"], shell=True, stdout=dnull, stderr=dnull)

os.mkdir(prefix + ".corpus")

with open(prefix + ".libfuzzer.data",'w') as outf:
    outf.write("time,coverage,fitness,total_execs,dictionary\n")

start = time.time()

while (time.time() - start) < total_time:
    runs += 1
    with open(prefix + "." + str(runs + 1) + ".libfuzzer.out",'w') as outf:    
        P = subprocess.Popen(cmd, shell=True, stdout=outf, stderr=outf)
    runs += 1
    print prefix, "RUN #" + str(runs)
    while 
    with open(prefix + ".libfuzzer.out",'r') as inf:
        coverage = None
        execs = None
        fit = None
        for line in inf:
            if "cov:" in line:
                coverage = int(line.split()[2])
            if "ft:" in line:
                fit = int(line.split()[2])                
            if "number_of_executed_units" in line:
                execs = int(line.split()[2])
            if BUILD_DICT and dict_started:
                if "EXTERNAL:" in line:
                    if "Uses" not in line:
                        current_entry += line.split("EXTERNAL: ")[1][:-1]
                    else:
                        dictionary.append(current_entry)
                        current_entry = ""
            if BUILD_DICT and ("dictionary" in line):
                dict_started = True
                current_entry = ""
    print "COVERAGE:", coverage
    print "FITNESS:", fit    
    print "EXECS:", execs
    total_execs += execs
    print "TOTAL EXECS:", total_execs
    print "TOTAL RUNTIME:", runs * timeout,"seconds"
    with open(prefix + ".libfuzzer.data",'a') as outf:
        outf.write(str(runs * timeout) + "," + str(coverage) + "," + str(fit) + "," +
                   str(total_execs) + "," + str(len(dictionary)) + "\n")
    if BUILD_DICT:
        print "DICTIONARY LENGTH:", len(dictionary)
        with open(prefix + ".dict.txt",'w') as outf:
            for entry in dictionary:
                outf.write(entry + "\n")

    with open(os.devnull,'w') as dnull:
        subprocess.call(["rm oom-*"], shell=True, stdout=dnull, stderr=dnull)
