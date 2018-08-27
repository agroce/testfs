import subprocess
import os

timeout = 60

BUILD_DICT = True

runs = 0
total_execs = 0
cmd0 = ["./TestsLF -rss_limit_mb=4096 -use_value_profile=1 -print_final_stats=1 -max_total_time=" +
        str(timeout) + " corpus"]
cmd1 = ["./TestsLF -rss_limit_mb=4096 -use_value_profile=1 -print_final_stats=1 -max_total_time=" +
        str(timeout) + " -dict=dict.txt corpus"]

with open("libfuzzer.data",'w') as outf:
    outf.write("time,coverage,total_execs,dictionary\n")

while True:
    with open("libfuzzer.out",'w') as outf:
        if runs == 0:
            subprocess.call(cmd0, shell=True, stdout=outf, stderr=outf)
        else:
            subprocess.call(cmd1, shell=True, stdout=outf, stderr=outf)            
    runs += 1
    print "RUN #" + str(runs)
    if BUILD_DICT:
        dictionary = []
        dict_started = False
    with open("libfuzzer.out",'r') as inf:
        coverage = None
        execs = None
        for line in inf:
            if "cov:" in line:
                coverage = int(line.split()[2])
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
    print "EXECS:", execs
    total_execs += execs
    print "TOTAL EXECS:", total_execs
    print "TOTAL RUNTIME:", runs * timeout,"seconds"
    with open("libfuzzer.data",'a') as outf:
        outf.write(str(runs * timeout) + "," + str(coverage) + "," + str(total_execs) + "," + str(len(dictionary)) + "\n")
    if BUILD_DICT:
        print "DICTIONARY LENGTH:", len(dictionary)
        with open("dict.txt",'w') as outf:
            for entry in dictionary:
                outf.write(entry + "\n")

    with open(os.devnull,'w') as dnull:
        subprocess.call(["rm oom-*"], shell=True, stdout=dnull, stderr=dnull)
