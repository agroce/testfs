from __future__ import print_function
import subprocess
import sys
import os

os.remove('default.profraw')

subprocess.call(["./TestsCov --input_test_file " + sys.argv[1]], shell=True)
subprocess.call(["llvm-profdata-6.0 merge -o testscov.profdata default.profraw"], shell=True)
subprocess.call(["llvm-cov-6.0 show ./TestsCov -instr-profile=testscov.profdata *.c Tests.cpp  >& new.covout"], shell=True)

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

coverage_new = []
with open("new.covout", 'r') as covf:
    for line in covf:
        ls = line.split("|")
        try:
            count = int(ls[1])
            if count > 1:
                count = 1
        except:
            count = -1
        coverage_new.append(count)

if coverage_original == coverage_new:
    print("COVERAGE MATCHES!")
    sys.exit(0)
else:
    print("COVERAGE DOES NOT MATCH!")
    sys.exit(1)
