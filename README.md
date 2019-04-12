# River 

- The instalation process is for Ubuntu 16.04.5 LTS xenial version
- Also, note that the process is described for all of our tools not just RIVER.
- You can download Ubuntu 16.04.5 LTS from here (take 64-bit) : http://releases.ubuntu.com/16.04/

## Steps setup 
1. Save the installRiverTools.sh file to your home directory ("~/")
2. Make sure you don't have anything important in folder "~/testtools" because that folder, if exists, will be removed.
3. From command line run:  
```
~/installRiverTools.sh clean bashrc
```
("clean" is for a clean build while bashrc is used to write the env variables in ~/.bashrc)

Now you have to close the terminal and open it again (to activate the env variables) to see if it works !

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

## Install Visual Studio Code
```
  a. sudo apt-get install curl
	b. curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > microsoft.gpg
	c. sudo mv microsoft.gpg /etc/apt/trusted.gpg.d/microsoft.gpg
	d. sudo sh -c 'echo "deb [arch=amd64] https://packages.microsoft.com/repos/vscode stable main" > /etc/apt/sources.list.d/vscode.list'
	e. sudo apt update
	f. sudo apt install code
  if you want to remove vscode : sudo apt remove code && sudo apt autoremove
```
## Debug using Visual Studio Code
``` 
  a. Open vs-code as administrator: sudo code --user-data-dir="~/.vscode-root"
  b. Install C/C++ extentions (there are 2) C/C++ 0.21.0 and C++ Intellisense 0.2.2
  c. File->Open folder simpletracer
  d. set breakPoint in river.tracer/rivertracer.cpp
  e. set the arguments in launch.json
	  "program": "${workspaceRoot}/river.tracer/river.tracer",
            "args": ["-p", "libfmi.so", "--annotated", "--z3"]
  f. Open libtracer/utils.cpp, and edit method ReadFromFile(...), comment the while loop inside and add the following lines:
     strcpy((char*) buf, "BBBBBB");
	    read = 6;
  g. Compile your changes, go to ~/testtools/river then ~/testtools/simpletracer and type the following:
   g.1 cmake CMakeLists
   g.2 make
   g.3 sudo make install

  h. close vscode, and open it normally
  i. start debugging
``` 

## Experiments 
If you want to see live how logs react to your changes:
1. Go to ~/testtools/river/benchmark-payloads/fmi and change the code inside fmi.c 
2. Run: ./test_build.sh to build and install the lib
3. Run ./test_inference.sh to test on a given payload input (edit the input payload inside the script if you want)
  Notice the file libfmi_dissassembly.txt created for you to understand the ASM code used by River. You must follow only the one in test_simple function ("search for the string in that file).


