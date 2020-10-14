# This is the implementation based on the SAGE paper
from __future__ import print_function
from triton     import TritonContext, ARCH, Instruction, MemoryAccess, CPUSIZE, MODE
import  sys
import argparse
import concolic_utils as CU
from typing import List, Dict, Set
import copy
import time

import logging

ENTRY_FUNC_ADDR = None
INPUT_BUFFER_ADDRESS = 0x10000000 # Where the input buffer will reside in the emulated program
TARGET_TO_REACH = False  # The target instruction to reach - None by default ,check documentation

# TODO Bogdan, replace this with the graphical interface
SECONDS_BETWEEN_STATS = 10 # The interval in seconds to show stats from the running execution


# The set of basic blocks found so far. This is used to evaluate how valuate are the inputs according to the heuristic
# Be carefully on how you use it in RL or distributed environments
AllBlocksFound: Set[int] = set()

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
                    help="where is the binary located")
    ap.add_argument("-entryfuncName", "--entryfuncName", required=False, default="RIVERTestOneInput",
                    help="the name of the entry func you want to start the test from. By default it is a 'RIVERTestOneInput' name function!", type=str)
    ap.add_argument("-arch", "--architecture", required=True,
                    help="Architecture of the executable, one of: ARM32, ARM64, X86, X64")
    ap.add_argument("-max", "--maxLen", required=True,
                    help="max size of input len", type=int)
    ap.add_argument("-targetAddress", "--targetAddress", required=False, default=None,
                    help="the target that your program is trying to reach !", type=str)
    ap.add_argument("-logLevel", "--logLevel", required=False, default='CRITICAL',
                    help="the log level you want, see the logging module and put the corresponding string. Put it DEBUG to see verything", type=str)
    ap.add_argument("-secondsBetweenStats", "--secondsBetweenStats", required=False, default='CRITICAL',
                    help="How many seconds to wait before showing new stats, this is only used if you don't want to use the graphical interface", type=int)
    args = ap.parse_args()

    loggingLevel = logging._nameToLevel[args.logLevel]
    logging.basicConfig(level=loggingLevel)  # filename='example.log', # Set DEBUG or INFO if you want to see more

    SECONDS_BETWEEN_STATS = args.secondsBetweenStats

    global TARGET_TO_REACH
    TARGET_TO_REACH = None if args.targetAddress is None else int(args.targetAddress, 16)

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


# Given a context where to emulate the binary already setup in memory with its input, and the PC address to emulate from, plus a few parameters...
# Returns a tuple (true if the optional target address was reached, num new basic blocks found - if countBBlocks is True)
def emulate(ctx, pc : int, countBBlocks : bool):
    targetAddressFound = False
    currentBBlockAddr = pc # The basic block address that we started to analyze currently
    numNewBasicBlocks = 0 # The number of new basic blocks found by this function (only if countBBlocks was activated)
    newBasicBlocksFound = set()

    def onNewBasicBlockEntryFound(addr):
        nonlocal numNewBasicBlocks
        nonlocal newBasicBlocksFound
        if addr not in AllBlocksFound:
            numNewBasicBlocks += 1
            newBasicBlocksFound.add(addr)
            AllBlocksFound.add(addr)

    onNewBasicBlockEntryFound(currentBBlockAddr)

    logging.info('[+] Starting emulation.')
    while pc:
        # Fetch opcode
        opcode = ctx.getConcreteMemoryAreaValue(pc, 16)

        # Create the ctx instruction
        instruction = Instruction()
        instruction.setOpcode(opcode)
        instruction.setAddress(pc)

        # Process
        ctx.processing(instruction)
        logging.info(instruction)

        # Next
        prevpc = pc
        pc = ctx.getConcreteRegisterValue(ctx.registers.rip)

        if instruction.isControlFlow():
            currentBBlockAddr = pc
            onNewBasicBlockEntryFound(currentBBlockAddr)
            logging.info(f"Instruction is control flow of type {instruction.getType()}. Addr of the new Basic block {hex(currentBBlockAddr)}")

        if TARGET_TO_REACH is not None and pc == TARGET_TO_REACH:
            targetAddressFound = True

    logging.info('[+] Emulation done.')
    if countBBlocks:
        logging.info(f'===== New basic blocks found: {[hex(intBlock) for intBlock in newBasicBlocksFound]}')

    return targetAddressFound, numNewBasicBlocks

# This function initializes the context memory.
def initContext(ctx : TritonContext, inputToTry : CU.Input, symbolized : bool):
    assert (ctx.isSymbolicEngineEnabled() == symbolized), "Making sure that context has exactly the matching requirements for the call, nothing more, nothing less"

    # Clean symbolic state
    if symbolized:
        ctx.concretizeAllRegister()
        ctx.concretizeAllMemory()

    def symbolizeAndConcretizeByteIndex(byteIndex, value, symbolized):
        byteAddr = INPUT_BUFFER_ADDRESS + byteIndex
        ctx.setConcreteMemoryValue(byteAddr, value)

        if symbolized:
            ctx.symbolizeMemory(MemoryAccess(byteAddr, CPUSIZE.BYTE))

    # Symbolize the input bytes in the input seed.
    # Put all the inputs in the buffer in the emulated program memory
    inputLen = max(inputToTry.buffer.keys())
    for byteIndex, value in inputToTry.buffer.items():
        symbolizeAndConcretizeByteIndex(byteIndex, value, symbolized)

    if symbolized:
        # Put the last bytes as fake sentinel inputs to promote some usages detection outside buffer
        SENTINEL_SIZE = 4
        for sentinelByteIndex in range(inputLen, inputLen + SENTINEL_SIZE):
            symbolizeAndConcretizeByteIndex(sentinelByteIndex, 0, symbolized)

    # The commented version is the generic one if using a plain buffer and no dict
    """
    for index in range(30):
        ctx.symbolizeMemory(MemoryAccess(0x10000000+index, CPUSIZE.BYTE))
    """

    # Point RDI on our buffer. The address of our buffer is arbitrary. We just need
    # to point the RDI register on it as first argument of our targeted function.
    ctx.setConcreteRegisterValue(ctx.registers.rdi, INPUT_BUFFER_ADDRESS)
    ctx.setConcreteRegisterValue(ctx.registers.rsi, inputLen)


    # Setup fake stack on an abitrary address.
    ctx.setConcreteRegisterValue(ctx.registers.rsp, 0x7fffffff)
    ctx.setConcreteRegisterValue(ctx.registers.rbp, 0x7fffffff)
    return


# Load the binary segments into the given set of contexts
def loadBinary(ctxList, binaryPath, entryfuncName):
    global ENTRY_FUNC_ADDR

    for ctxIndex, ctx in enumerate(ctxList):
        logging.info(f"Loading the binary at path {binaryPath}..")
        import lief
        binary = lief.parse(binaryPath)
        if binary is None:
            assert False, f"Path to binary not found {binaryPath}"
            exit(0)

        phdrs  = binary.segments
        for phdr in phdrs:
            size   = phdr.physical_size
            vaddr  = phdr.virtual_address
            logging.info('[+] Loading 0x%06x - 0x%06x' %(vaddr, vaddr+size))
            ctxList[ctxIndex].setConcreteMemoryAreaValue(vaddr, phdr.content)

        if ENTRY_FUNC_ADDR is None:
            logging.info(f"Findind the exported function of interest {binaryPath}..")
            res = binary.exported_functions
            for function in res:
                if entryfuncName in function.name:
                    ENTRY_FUNC_ADDR = function.address
                    logging.info(f"Function of interest found at address {ENTRY_FUNC_ADDR}")
                    break

        assert ENTRY_FUNC_ADDR != None, "Exported function wasn't found"

# This function returns a set of new inputs based on the last trace.
def Expand(ctx, inputToTry):
    logging.info(f"Seed injected:, {inputToTry}")

    # Init context memory
    initContext(ctx, inputToTry, symbolized=True)

    # Emulate the binary with the setup memory
    emulate(ctx, ENTRY_FUNC_ADDR, countBBlocks=False)

    # Set of new inputs
    inputs : List[CU.Input] = []

    # Get path constraints from the last execution
    PathConstraints = ctx.getPathConstraints()

    # Get the astContext
    astCtxt = ctx.getAstContext()

    # This represents the current path constraint
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
            for branch in branches:
                # Get the constraint of the branch which has been not taken
                if branch['isTaken'] == False:
                    #print(branch['constraint'])
                    #expr = astCtxt.unroll(branch['constraint'])
                    #expr = ctx.simplify(expr)

                    # Ask for a model and put all the changed bytes in a dictionary
                    model = ctx.getModel(astCtxt.land([currentPathConstraint, branch['constraint']]))
                    changes   = dict()  # A dictionary  from byte index (relative to input buffer beginning) to the value it has in he model
                    for k, v in list(model.items()):
                        # Get the symbolic variable assigned to the model
                        symVar = ctx.getSymbolicVariable(k)
                        # Save the new input as seed.
                        byteAddrAccessed = symVar.getOrigin()
                        byteAddrAccessed_relativeToInputBuffer = byteAddrAccessed - INPUT_BUFFER_ADDRESS
                        changes.update({byteAddrAccessed_relativeToInputBuffer : v.getValue()})

                    # Then, if a possible change was detected => create a new input entry and add it to the output list
                    if changes:
                        newInput = copy.deepcopy(inputToTry)
                        newInput.applyChanges(changes)
                        newInput.bound = pcIndex + 1
                        inputs.append(newInput)

        # Update the previous constraints with taken(true) branch to keep the same path initially taken
        currentPathConstraint = astCtxt.land([currentPathConstraint, pc.getTakenPredicate()])

    # Clear the path constraints to be clean at the next execution.
    ctx.clearPathConstraints()

    return inputs

# Updates on screen feedback
def UpdateOutputStats(startTime, currTime, forceOutput = False):
    # TODO Bogdan: disable this output if user requested the graphical interface !
    newTime = time.time()
    secondsSinceLastUpdate = (newTime - currTime)
    secondsSinceStart = (newTime - startTime)
    if secondsSinceLastUpdate >= SECONDS_BETWEEN_STATS or forceOutput == True:
        currTime = newTime
        logging.critical(f"After {secondsSinceStart:.2f}s Found {len(AllBlocksFound)} blocks '\n' at addresses {AllBlocksFound}")

    return currTime

# This function starts with a given seed dictionary and does concolic execution starting from it.
def SearchInputs(ctx, initialSeedDict):
    # Init the worklist with the initial seed dict
    worklist  = CU.InputsWorklist()
    forceFinish = False

    # Put all the the inputs in the seed dictionary to the worklist
    for initialSeed in initialSeedDict:
        inp = CU.Input()
        inp.buffer = {k: v for k, v in enumerate(initialSeed)}
        inp.bound = 0
        inp.priority = 0
        worklist.addInput(inp)

    startTime = currTime = time.time()
    while worklist:
        # Take the first seed
        inputSeed : CU.Input = worklist.extractInput()
        newInputs = Expand(ctx, inputSeed)

        for newInp in newInputs:
            # Execute the input to detect real issues with it
            ExecuteInputToDetectIssues(newInp)

            # Assign this input a priority, and check if the hacked target address was found or not
            targetFound, newInp.priority = ScoreInput(newInp)

            if targetFound:
                logging.critical(f"The solution to get to the target address is input {newInp}")
                # TODO: Bogdan put this in output somewhere , folder, visualization etc.
                forceFinish = True # No reason to continue...
                break

            # Then put it in the worklist
            worklist.addInput(newInp)

        currTime = UpdateOutputStats(startTime, currTime)

        if forceFinish:
            break

    currTime = UpdateOutputStats(startTime, currTime, forceOutput=True)


def ExecuteInputToDetectIssues(input : CU.Input):
    # TODO Bogdan, output somehow if the input has issues - folders, visual report etc...
    pass

def ScoreInput(newInp : CU.Input):
    logging.info(f"--Scoring input {newInp}")
    initContext(tracingContext, newInp, symbolized=False)
    targetFound, numNewBlocks = emulate(tracingContext, ENTRY_FUNC_ADDR, countBBlocks=True)

    return targetFound, numNewBlocks # as default, return the bound...

if __name__ == '__main__':

    args = parseArgs()

    # Create two contexts : one symbolic used for detecting path constraints etc, and another one less heavy used only for tracing
    symbolicContext = TritonContext(args.architecture)
    tracingContext = TritonContext(args.architecture)

    tracingContext.enableSymbolicEngine(False)
    logging.info(f"Tracing context has enabled symboling engine ? {tracingContext.isSymbolicEngineEnabled()}")
    logging.info(f"Symbolic context has enabled symboling engine ?{symbolicContext.isSymbolicEngineEnabled()}")
    assert tracingContext.isSymbolicEngineEnabled() == False
    assert symbolicContext.isSymbolicEngineEnabled() == True

    # Define symbolic optimizations
    symbolicContext.setMode(MODE.ALIGNED_MEMORY, True)
    symbolicContext.setMode(MODE.ONLY_ON_SYMBOLIZED, True) # ????
    #symbolicContext.setMode(MODE.AST_OPTIMIZATIONS, True)
    #symbolicContext.setMode(MODE.CONSTANT_FOLDING, True)


    # Load the binary into the given set of contexts
    loadBinary([symbolicContext, tracingContext], args.binaryPath, args.entryfuncName)

    # TODO Bogdan: Implement the corpus strategies as defined in https://llvm.org/docs/LibFuzzer.html#corpus, or Random if not given
    initialSeedDict = ["good"]
    CU.processSeedDict(initialSeedDict) # Transform the initial seed dict to bytes instead of chars if needed

    SearchInputs(symbolicContext, initialSeedDict)

    if RECONSTRUCT_BB_GRAPH:
        print(f"Reconstructed graph is: {BlocksGraph}")
    sys.exit(0)

