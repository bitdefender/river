# River 3.0 
# Short intro to River 3.0 changes
- We are going to have two backends: 
 0. LLVM based tools and Libfuzzer 
 1. Binary execution using Triton and LIEF
 
 Currently, we have 1 (binary execution) functional, while 0 (LLVM) is work in progress.
 - It is **cross-platform running on all common OSs and support architectures: x86,x64,ARM32, ARM64**
 - River 3.0 will be a collection of tools that runs over the backend mentioned above. 

# Installation and testing

Step 1: Clone this repo with --recursive option, since we have some external submodules.
Step 2: Build Triton from ExternalTools/Triton with Python bindings as documented in the README.md file in the submodule, or use their latest info on https://github.com/JonathanSalwan/Triton 
Step 3: Install LIEF either from ExternalTools or with ```pip install lief```, or as according to their documentation: https://github.com/lief-project/LIEF 
Note that by taking the versions inside submodules, it is guaranteed to compile correctly with the current River 3.0 API. 

To check if everything is working correctly, open you Python interface (tested on Python 3.7) and check if the following commands work correctly:
```
import triton
import lief
```

# Testing River 3.0

 Currently we have implemented as a proof of concept a Generic Concolic Executor that can be found in River3/ subfolder. 
 You can test it against the crackme_xor program inside River3/TestPrograms
 
 How to build your program for testing with our tools ? 
```
gcc -g -O0 -o crackme_xor ./crackme_xor.c
(you can use without -g or -O0)
```

How to use or concolic tool ? Currently in concolicGeneric.py you can set a sample of parameters like below.


**--annotated ../../samples/crackmes/crackme_xor**
**--architecture x64**
**--minLen 1**
**--maxLen 1**
**--targetAddress 0x11ce**

The targetAddress is optional, it is for capture the flag like kind of things, where you want to get to a certain address
in the binary code.
If you have the source code, you can use: the following command to get the address of interest. The execution will stop when the target is reached, otherwise it will exhaustively try to search all inputs.

```
 objdump -M intel -S ./crackme_xor
```
## Future work
 - Parallelization
 - We are going to convert the evaluation API to Google FuzzBench and Lava kind of benchmarks
 

# *Part 1: River and tracer tools*

- The instalation process is for Ubuntu 16.04.5 LTS xenial version
- Also, note that the process is described for all of our tools not just RIVER.
- You can download Ubuntu 16.04.5 LTS from here (take 64-bit) : http://releases.ubuntu.com/16.04/

# Steps setup 

## End-user Workflow:
0. Be sure you install the latest cmake version first. The version tested was 3.14.
1. Save the installRiverTools.sh file to your home directory ("~/")
```
wget https://raw.githubusercontent.com/AGAPIA/river/master/installRiverTools.sh
```
2. Download an unzip The prebuilt-Z3 libraries https://fmiunibuc-my.sharepoint.com/:u:/g/personal/ciprian_paduraru_fmi_unibuc_ro/EUBDz3URzcBAjzEslzE8LfYBEoR0uwKubUoIweqTSuJeRg?e=3jmRPT to ~/z3 folder (such that path ~/z3/Debug/lib/libz3.so exists).
3. Make sure you don't have anything important in folder "~/testtools" because that folder, if exists, will be removed.
4. From command line run for example:  
```
sh ~/installRiverTools.sh testtools Debug
```
First parameter is the folder to install all River tool. This command will install all in ~/testtools/ . You can of course change the folder name as you wish
Second parameter is for build type and it can be either Debug or Release

Now you have to close the terminal and open it again (to activate the env variables) to see if it works !


## Test tracer functionality

Test simpletracer on the basic fmi experiments lib:
```
python -c 'print ("B" * 100)' | river.tracer -p libfmi.so
``` 

More details about the parameters:
```
$ river.tracer --payload <target-library> [--annotated] [--z3] < <input_test_case>
```
**--annotated** adds tainted index data in traces


**--payload \<target-library\>** specifies the shared object that you want to trace. This must export the payload that is the tested sw entry point.

**--annotated** adds tainted index data in traces

**--z3** specified together with `--annotated` adds z3 data in traces

**\<input_test_case\>** is corpus file that represents a test case.

## Experiments 
If you want to see live how logs react to your changes:
1. Go to /home/YOURUSERNAME/testtools/river/benchmark-payloads/fmi and change the code inside fmi.c 
2. Run: ./test_build.sh to build and install the lib
3. Run ./test_inference.sh to test on a given payload input (edit the input payload inside the script if you want)
  Notice the file libfmi_dissassembly.txt created for you to understand the ASM code used by River. You must follow only the one in test_simple function ("search for the string in that file).

## Programmers workflow:
Asside from the things defined above, you can incrementally test your build by running:
```
~/testtools/river/a_buildtools.sh (Debug or Release) testtools [clean].
```
The last parameter , clean, is optional, only if you want to rebuild everything.

## Debug using Visual Studio Code

The modern way to debug and build projects from VS code:
```
  a. Install VS code using ~/installVsCode.sh script. 
  b. Open vs-code as administrator: sudo code --user-data-dir="~/.vscode-root"
  c. Install C++ , native debugging and cmake extensions
  d. Open the folder of any solution you want to debug and use Cmake status bar to build/debug.
    NOTE: For simpletracer use a GCC compiler in the Cmake interface toolbar
    NOTE: For debugging it is better to use DEbug->StartDebugging or the green arrow below Debug menu. The launch.json is already configured for you.
```


# *Part 2: Concolic executor*

## Installing and usage 
The source code are in river/riverexp folder. You can build it using Visual Studio code. Just open de folder and build, as specified above in the River tools section 
TODO: we need to make an installer and separate it from RIVER with a different repository since its code is totally independent.

The options for executing the executable are the following:

```
riverexp -p libfmi.so --numProcs 1 --outformat [binary OR text] [--outfilter] [--manualtracers] [--maxInputSize 1024] [--maxOutputSize 10000000] 
--inputSeedsFolder /youpath/river/riverexp/benchmarks/yourfoldername
--outputFolder /yourpath/river/riverexp/benchmarks/outputs/yourfoldername
```

**-p** specifies the library name to be executed

**--numProcs** how many procs to use ( TODO: using more than 1 process is work in progress)

**--outformat** can be either binary or text. If binary is used, it will write a binary file for each input produced. If text, then it will output all tests in a single file (open subfolder "outputs" to see all files inside).

**--manualtracers** if used, you can manually spawn tracers, such that you can debug in a proper debugging mode if there is a problem with simpletracer process or communication.

**--maxInputSize** maximum size of input generated

**--maxOutputSize** maximum size of output expected

**--inputSeedsFolder** a folder containing input seeds for your application under test
**--outputFolder** a folder containing new inputs generated by concolic execution. This will also contain folders with inputs for different categories of problems such as: SIGSEGV,SIGABRT, etc. 

## Debugging 

Usually we debug this using Visual Studio code (see the above section for River tools debugging). To debug both simpletracer and riverexp at the same time, just use **manualtracers** option, then start riverexp and simpletracer using:
simple tracer -p libfmi.so --annotated --z3 --flow --addrName /home/ciprian/socketriver --exprsimplify
Also, use **TracerExecutionExtern** strategy (see the section below).

***Note that if you use Visual Studio Code and open launch.json file, you can find several pre-made parameter configurations for both riverexp and simpletracer***.

## Architecture

The Concolic executor is actually totally decoupled from River and simpletracer, built on strategy pattern.
Check the ConcolicExecutor class and how it aggregates inside a TracerExecutionStrategy. TracerExecutionStrategy can be currently:

**TracerExecutionStrategyExternal** - for executing in textual format. Basically the communication between riverexp and simpletracer is done with text file generated, such that you can easily debug things not working.

**TracerExecutionStrategyIPC** - communication through sockets. Currently uses only 1 process.

**TracerExecutionStrategyMPI** - communication using MPI. No code. TODO task

A list of working progress task can be found at https://trello.com/b/WmfPJGo6/tech-tasks . If you like something don't hesitate to contribute to our project !
