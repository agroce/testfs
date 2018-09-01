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
        subprocess.call([deepstate + " --input_test_file " +
                         candidate + " --verbose_reads"],
                        shell=True, stdout=outf, stderr=outf)
    result = []
    with open(".reducer.out", 'r') as inf:
        for line in inf:
            result.append(line)
    return result

def checks(result):
    for line in result:
        if checkString in line:
            return True
    return False

def structure(result):
    OneOfs = []
    currentOneOf = []
    for line in result:
        if "STARTING OneOf CALL" in line:
            currentOneOf.append(-1)
        elif "Reading byte at" in line:
            lastRead = int(line.split()[-1])
            if currentOneOf[-1] == -1:
                currentOneOf[-1] == lastRead
        elif "FINISHED OneOf CALL" in line:
            OneOfs.append((currentOneOf[-1], lastRead))
            currentOneOf = currentOneOf[:-1]
    return (OneOfs, lastRead)

initial = runCandidate(test)
assert checks(initial)

with open(test, 'rb') as data:
    data = data.read()

original = bytes(data)
print type(original)
print type(data)

print structure(initial)
        
