from __future__ import print_function
import subprocess
import sys
import os

os.remove('default.profraw')

subprocess.call(["./TestsCov --verbose_reads --input_test_file " + sys.argv[1]], shell=True)
subprocess.call(["llvm-profdata-6.0 merge -o testscov.profdata default.profraw"], shell=True)
with open("new.covout", 'w') as newcovf:
    subprocess.call(["llvm-cov-6.0 show ./TestsCov -instr-profile=testscov.profdata *.c Tests.cpp"], shell=True, stdout=newcovf,
                        stderr=newcovf)

coverage_original = []
with open("covout", 'r') as covf:
    for line in covf:
        ls = line.split("|")
        try:
            count = int(ls[1])
            if count > 1:
                count = 1
        except:
            count = -1
        coverage_original.append(count)

pos = 0
with open("new.covout", 'r') as covf:
    for line in covf:
        ls = line.split("|")
        try:
            count = int(ls[1])
            if count > 1:
                count = 1
        except:
            count = -1
        if coverage_original[pos] > count:
            print("COVERAGE DOES NOT MATCH!")
            sys.exit(1)
        pos += 1

print("COVERAGE MATCHES!")
sys.exit(0)

