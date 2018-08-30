import subprocess
import os
import sys

timeout = 60
prefix = sys.argv[1]
total_time = int(sys.argv[2])

BUILD_DICT = "--no_dict" not in sys.argv
VALUE_PROFILE = "--no_value_profile" not in sys.argv

if "--no_checkpoint" in sys.argv:
    timeout = total_time

if VALUE_PROFILE:
    val_prof = " -use_value_profile=1 "
else:
    val_prof = " "

runs = 0
total_execs = 0

with open(os.devnull,'w') as dnull:
    subprocess.call(["rm -rf " + prefix + ".corpus"], shell=True, stdout=dnull, stderr=dnull)

os.mkdir(prefix + ".corpus")

cmd0 = ["./TestsLF -rss_limit_mv=4096" + val_prof + "-print_final_stats=1 -max_total_time=" +
        str(timeout) + " " + prefix + ".corpus"]
if BUILD_DICT:
    cmd1 = ["./TestsLF -rss_limit_mb=4096" + val_prof + "-print_final_stats=1 -max_total_time=" +
            str(timeout) + " -dict=" + prefix + ".dict.txt " + prefix + ".corpus"]
else:
    cmd1 = cmd0

with open(prefix + ".libfuzzer.data",'w') as outf:
    outf.write("time,coverage,fitness,total_execs,dictionary\n")

dictionary = []
    
while (runs * timeout) < total_time:
    runs += 1
    with open(prefix + "." + str(runs) + ".libfuzzer.out",'w') as outf:
        if len(dictionary) == 0:
            subprocess.call(cmd0, shell=True, stdout=outf, stderr=outf)
        else:
            subprocess.call(cmd1, shell=True, stdout=outf, stderr=outf)            
    runs += 1
    print prefix, "RUN #" + str(runs)
    if BUILD_DICT:
        dictionary = []
        dict_started = False
    with open(prefix + str(runs) + ".libfuzzer.out",'r') as inf:
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
