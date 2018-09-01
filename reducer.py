import sys
import subprocess
import os
import shutil

deepstate = sys.argv[1]
test = sys.argv[2]
out = sys.argv[3]
checkString = sys.argv[4]

def runCandidate(candidate):
    with open(".reducer.out", 'w') as outf:
        subprocess.call([deepstate, "--input_test_file", candidate,
                             "--verbose_reads"], stdout=outf, stderr=outf)
    result = []
    with open(".reducer.out", 'r') as inf:
        for line in inf:
            result.append(inf)
    return result

def checks(result):
    for line in result:
        if checkString in line:
            return True
    return False

initial = runCandidate(test)
assert checks(result)

        
