import sys
import subprocess
import os
import shutil

if "--help" in sys.argv:
    print "usage: deepstate-reduce binary input-test output-test [string] [--which test]"
    print
    print "Reduces input-test by trying to delete OneOf blocks and lower byte values."
    print "Writes reduced test to output-test."
    print "Optional string gives an output reduction criteria (string to look for in output)."
    print "If no string is provided, looks for Failure or Crash."
    print
    print "--which test allows control over which DeepState test is executed, if"
    print "none is provided, defaults to last test defined."
    sys.exit(0)

try:
    which = sys.argv.index("--which")
    whichTest = sys.argv[which+1]
    sys.argv = sys.argv[:which] + sys.argv[which + 2]
    print sys.argv
except:
    pass
    
deepstate = sys.argv[1]
test = sys.argv[2]
out = sys.argv[3]
if len(sys.argv) > 4:
    checkString = sys.argv[4]
else:
    checkString = None

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
        if checkString:
            if checkString in line:
                return True
        else:
            if "ERROR: Failed:" in line:
                return True
            if "ERROR: Crashed" in line:
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
                currentOneOf[-1] = lastRead
        elif "FINISHED OneOf CALL" in line:
            OneOfs.append((currentOneOf[-1], lastRead))
            currentOneOf = currentOneOf[:-1]
    return (OneOfs, lastRead)

initial = runCandidate(test)
assert checks(initial)

with open(test, 'rb') as test:
    currentTest = bytearray(test.read())

print "ORIGINAL TEST HAS", len(currentTest), "BYTES"
    
s = structure(initial)
print "LAST BYTE READ IS", s[1]

if s[1] < len(currentTest):
    print "SHRINKING TO IGNORE UNREAD BYTES"
    currentTest = currentTest[:s[1]+1]

changed = True
while changed:
    changed = False
    cuts = s[0]
    for c in cuts:
        newTest = currentTest[:c[0]] + currentTest[c[1]+1:]
        with open(".candidate.test", 'wb') as outf:
            outf.write(newTest)
        r = runCandidate(".candidate.test")
        if checks(r):
            print "ONEOF REMOVAL SUCCEEDED:", len(newTest)
            s = structure(r)
            changed = True
            currentTest = newTest
            break
    for b in range(0, len(currentTest)):
        for v in range(0, currentTest[b]):
            newTest = bytearray(currentTest)
            newTest[b] = v
            with open(".candidate.test", 'wb') as outf:
                outf.write(newTest)
            r = runCandidate(".candidate.test")
            if checks(r):
                print "BYTE REDUCTION SUCCEEDED:", len(newTest)
                s = structure(r)
                changed = True
                currentTest = newTest
                break            
    if not changed:
        print "NO REDUCTIONS FOUND"

if (s[1] + 1) > len(currentTest):
    print "PADDING TEST WITH", (s[1] + 1) - len(currentTest), "ZEROS"
    padding = bytearray('\x00' * ((s[1] + 1) - len(currentTest)))
    currentTest = currentTest + padding
        
with open(out, 'wb') as outf:
    outf.write(currentTest)
