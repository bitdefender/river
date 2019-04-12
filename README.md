# River 

- The instalation process is for Ubuntu 16.04.5 LTS xenial version
- Also, note that the process is described for all of our tools not just RIVER.

## Steps setup 
1. Save the installRiverTools.sh file to home directory ("~/")
2. Make sure you don't have anything important in folder "~/testtools"
3. run ~/installRiverTools.sh from command line

## Test tracer functionality

Test simpletracer on the basic fmi experiments lib:
```
python -c 'print "B" * 100' | river.tracer -p libfmi.so
``` 

More details about the parameters:
```
$ ./bin/river.tracer --payload <target-library> [--annotated] [--z3] < <input_test_case>
```
**--annotated** adds tainted index data in traces


**--payload \<target-library\>** specifies the shared object that you want to trace. This must export the payload that is the tested sw entry point.

**--annotated** adds tainted index data in traces

**--z3** specified together with `--annotated` adds z3 data in traces

**\<input_test_case\>** is corpus file that represents a test case.



## Experiments 
If you want to see live how logs react to your changes:
1. Go to /home/YOURUSERNAME/testtools/river/benchmark-payloads/fmi and change the code inside fmi.c 
2. Run: ./test_build.sh to build and install the lib, then ./test_inference.sh to test on a given payload input 
(edit the input payload inside the script if you want)

