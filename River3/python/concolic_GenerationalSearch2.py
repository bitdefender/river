# This is the implementation based on the SAGE paper
from __future__ import print_function
from triton import TritonContext, ARCH, Instruction, MemoryAccess, CPUSIZE, MODE
import sys
import argparse
import RiverUtils as RiverUtils
from RiverTracer import RiverTracer
from typing import List, Dict, Set
import copy
import time
from RiverOutputStats import RiverStatsTextual
import logging

# TODO Bogdan, replace this with the graphical interface
# The online reconstructed graph that shows possible connections between basic blocks.
# We don't keep inputs or symbolic conditions because we don;t need them so far..just the graph
BlocksGraph : Dict[int, Set[int]] = {} # From basic block to the list of basic blocks and possible links

RECONSTRUCT_BB_GRAPH = False # Enable this if you need the block graph above

def onEdgeDetected(fromAddr, toAddr):
    if fromAddr not in BlocksGraph:
        BlocksGraph[fromAddr] = set()
    BlocksGraph[fromAddr].add(toAddr)

def parseArgs():
    # Construct the argument parser
    ap = argparse.ArgumentParser()

    # Add the arguments to the parser
    ap.add_argument("-bp", "--binaryPath", required=True,
                    help="the test binary location")
    ap.add_argument("-entryfuncName", "--entryfuncName", required=False, default="RIVERTestOneInput",
                    help="the name of the entry function you want to start the test from. By default the function name is 'RIVERTestOneInput'!", type=str)
    ap.add_argument("-arch", "--architecture", required=True,
                    help="architecture of the executable: ARM32, ARM64, X86, X64 are supported")
    ap.add_argument("-max", "--maxLen", required=True,
                    help="maximum size of input length", type=int)
    ap.add_argument("-targetAddress", "--targetAddress", required=False, default=None,
                    help="the target address that your program is trying to reach", type=str)
    ap.add_argument("-logLevel", "--logLevel", required=False, default='CRITICAL',
                    help="set the log level threshold, see the Python logging module documentation for the list of levels. Set it to DEBUG to see everything!", type=str)
    ap.add_argument("-secondsBetweenStats", "--secondsBetweenStats", required=False, default='10',
                    help="the interval (in seconds) between showing new stats", type=int)
    ap.add_argument("-outputType", "--outputType", required=False, default='textual',
                    help="the output interface type, can be visual or textual", type=str)
    args = ap.parse_args()

    loggingLevel = logging._nameToLevel[args.logLevel]
    logging.basicConfig(level=loggingLevel)  # filename='example.log', # Set DEBUG or INFO if you want to see more

    SECONDS_BETWEEN_STATS = args.secondsBetweenStats

    args.targetAddress = None if args.targetAddress is None else int(args.targetAddress, 16)

    # Set the architecture
    if args.architecture == "ARM32":
        args.architecture = ARCH.ARM32
    elif args.architecture == "ARM64":
        args.achitecture = ARCH.X86_64
    elif args.architecture == "x86":
        args.architecture = ARCH.X86
    elif args.architecture == "x64":
        args.architecture = ARCH.X86_64
    else:
        assert False, "This architecture is not implemented"
        raise NotImplementedError

    return args

# This function returns a set of new inputs based on the last trace.
def Expand(symbolicTracer : RiverTracer, inputToTry):
    logging.critical(f"Seed injected:, {inputToTry}")

    symbolicTracer.runInput(inputToTry, symbolized=True, countBBlocks=False)

    # Set of new inputs
    inputs : List[RiverUtils.Input] = []

    # Get path constraints from the last execution
    PathConstraints = symbolicTracer.getLastRunPathConstraints()

    # Get the astContext
    astCtxt = symbolicTracer.getAstContext()

    # This represents the current path constraint, dummy initialization
    currentPathConstraint = astCtxt.equal(astCtxt.bvtrue(), astCtxt.bvtrue())

    # Go through the path constraints from bound of the input (to prevent backtracking as described in the paper)
    PCLen = len(PathConstraints)
    for pcIndex in range(inputToTry.bound, PCLen):
        pc = PathConstraints[pcIndex]

        # Get all branches
        branches = pc.getBranchConstraints()

        if RECONSTRUCT_BB_GRAPH:
            # Put all detected edges in the graph
            for branch in branches:
                onEdgeDetected(branch['srcAddr'], branch['dstAddr'])

        # If there is a condition on this path (not a direct jump), try to reverse it
        if pc.isMultipleBranches():
            takenAddress = pc.getTakenAddress()
            for branch in branches:
                # Get the constraint of the branch which has been not taken
                if branch['dstAddr'] != takenAddress:
                    #print(branch['constraint'])
                    #expr = astCtxt.unroll(branch['constraint'])
                    #expr = ctx.simplify(expr)

                    # Check if we can change current executed path with the branch changed
                    desiredConstrain = astCtxt.land([currentPathConstraint, branch['constraint']])
                    changes = symbolicTracer.solveInputChangesForPath(desiredConstrain)

                    # Then, if a possible change was detected => create a new input entry and add it to the output list
                    if changes:
                        newInput = copy.deepcopy(inputToTry)
                        newInput.applyChanges(changes)
                        newInput.bound = pcIndex + 1
                        inputs.append(newInput)

        # Update the previous constraints with taken(true) branch to keep the same path initially taken
        currentPathConstraint = astCtxt.land([currentPathConstraint, pc.getTakenPredicate()])

    # Clear the path constraints to be clean at the next execution.
    symbolicTracer.resetLastRunPathConstraints()

    return inputs

# This function starts with a given seed dictionary and does concolic execution starting from it.
def SearchInputs(symbolicTracer, simpleTracer, initialSeedDict, binaryPath):
    # Init the worklist with the initial seed dict
    worklist  = RiverUtils.InputsWorklist()
    forceFinish = False

    # Put all the the inputs in the seed dictionary to the worklist
    for initialSeed in initialSeedDict:
        inp = RiverUtils.Input()
        inp.buffer = {k: v for k, v in enumerate(initialSeed)}
        inp.bound = 0
        inp.priority = 0
        worklist.addInput(inp)

    startTime = currTime = time.time()
    while worklist:
        # Take the first seed
        inputSeed : RiverUtils.Input = worklist.extractInput()
        newInputs = Expand(symbolicTracer, inputSeed)

        for newInp in newInputs:
            # Execute the input to detect real issues with it
            issue = ExecuteInputToDetectIssues(binaryPath, newInp)
            if issue != None:
                # TODO: save the input to a database
                print(f"{binaryPath} has issues: {issue} on input {newInp}")
                pass

            # Assign this input a priority, and check if the hacked target address was found or not
            targetFound, newInp.priority = ScoreInput(newInp, simpleTracer)

            if targetFound:
                logging.critical(f"The solution to get to the target address is input {newInp}")
                # TODO: Bogdan put this in output somewhere , folder, visualization etc.
                forceFinish = True # No reason to continue...
                break

            # Then put it in the worklist
            worklist.addInput(newInp)

        currTime = outputStats.UpdateOutputStats(startTime, currTime, collectorTracers=[simpleTracer])

        if forceFinish:
            break

    currTime = outputStats.UpdateOutputStats(startTime, currTime, collectorTracers=[simpleTracer], forceOutput=True)


def ExecuteInputToDetectIssues(binary_path, input : RiverUtils.Input):
    from bugs_detection.test_inputs import sig_str, test_input
    return sig_str.get(test_input(binary_path, bytes([v for v in input.buffer.values()])))

def ScoreInput(newInp : RiverUtils.Input, simpleTracer : RiverTracer):
    logging.critical(f"--Scoring input {newInp}")
    targetFound, numNewBlocks, allBBsInThisRun = simpleTracer.runInput(newInp, symbolized=False, countBBlocks=True)
    return targetFound, numNewBlocks # as default, return the bound...

if __name__ == '__main__':

    args = parseArgs()

    # Create two tracers : one symbolic used for detecting path constraints etc, and another one less heavy used only for tracing and scoring purpose
    symbolicTracer  = RiverTracer(symbolized=True,  architecture=args.architecture, targetAddressToReach=args.targetAddress)
    simpleTracer    = RiverTracer(symbolized=True, architecture=args.architecture, targetAddressToReach=args.targetAddress)

    # Load the binary info into the given list of tracers. We do this strage API to load only once the binary...
    RiverTracer.loadBinary([symbolicTracer, simpleTracer], args.binaryPath, args.entryfuncName)
    if args.outputType == "textual":
        outputStats = RiverStatsTextual()

    # TODO Bogdan: Implement the corpus strategies as defined in https://llvm.org/docs/LibFuzzer.html#corpus, or Random if not given
    initialSeedDict = ["good"] # ["a<9d"]
    RiverUtils.processSeedDict(initialSeedDict) # Transform the initial seed dict to bytes instead of chars if needed

    SearchInputs(symbolicTracer=symbolicTracer, simpleTracer=simpleTracer, initialSeedDict=initialSeedDict, binaryPath=args.binaryPath)

    if RECONSTRUCT_BB_GRAPH:
        print(f"Reconstructed graph is: {BlocksGraph}")
    sys.exit(0)
