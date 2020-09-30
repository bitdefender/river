from __future__ import print_function
from triton     import TritonContext, ARCH, Instruction, MemoryAccess, CPUSIZE, MODE
import  sys
import argparse

import logging
logging.basicConfig(level=logging.DEBUG) # filename='example.log', # Set DEBUG or INFO if you want to see more

ENTRY = None
ENTRY_FUNC_NAME = "check" # "check"
INPUT_BUFFER_ADDRESS = 0x10000000
TARGET_TO_REACH = None

def parseArgs():
    # Construct the argument parser
    ap = argparse.ArgumentParser()

    # Add the arguments to the parser
    ap.add_argument("-bp", "--binaryPath", required=True,
                    help="where is the binary located")
    ap.add_argument("-arch", "--architecture", required=True,
                    help="Architecture of the executable, one of: ARM32, ARM64, X86, X64")
    ap.add_argument("-min", "--minLen", required=True,
                    help="min size of input len", type=int)
    ap.add_argument("-max", "--maxLen", required=True,
                    help="max size of input len", type=int)
    ap.add_argument("-targetAddress", "--targetAddress", required=False, default=None,
                    help="the target that your program is trying to reach !", type=str)
    args = ap.parse_args()

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

# Returns true if the optional target address was reached
def emulate(ctx, pc):
    targetFound = False
    astCtxt = ctx.getAstContext()
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
        pc = ctx.getConcreteRegisterValue(ctx.registers.rip)

        if TARGET_TO_REACH is not None and pc == TARGET_TO_REACH:
            targetFound = True

    logging.info('[+] Emulation done.')
    return targetFound

# This function initializes the context memory.
def initContext(ctx, seed):
    # Clean symbolic state
    ctx.concretizeAllRegister()
    ctx.concretizeAllMemory()

    # Symbolize the inputs in the seed.
    # Todo: do the proper buffer version
    seed = seed.copy()
    # Put the last bytes as fake sentinel inputs to promote some usages detection outside buffer
    SENTINEL_SIZE = 4
    inputLen = max(seed.keys()) + 1
    for sentinelByteIndex in range(inputLen, inputLen + SENTINEL_SIZE):
        seed[sentinelByteIndex] = 0

    inputLen = max(seed.keys()) + 1
    for byteIndex, value in seed.items():
        byteAddr = INPUT_BUFFER_ADDRESS + byteIndex
        ctx.setConcreteMemoryValue(byteAddr, value)
        ctx.symbolizeMemory(MemoryAccess(byteAddr, CPUSIZE.BYTE))

    # The commented version is the generic one
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



# This function returns a set of new inputs based on the last trace.
def getNewInput(ctx):
    # Set of new inputs
    inputs = list()

    # Get path constraints from the last execution
    pco = ctx.getPathConstraints()

    # Get the astContext
    astCtxt = ctx.getAstContext()

    # We start with any input. T (Top)
    previousConstraints = astCtxt.equal(astCtxt.bvtrue(), astCtxt.bvtrue())

    # Go through the path constraints
    for pc in pco:
        # If there is a condition
        if pc.isMultipleBranches():
            # Get all branches
            branches = pc.getBranchConstraints()
            for branch in branches:
                # Get the constraint of the branch which has been not taken
                if branch['isTaken'] == False:
                    # Ask for a model
                    #print(branch['constraint'])
                    #expr = astCtxt.unroll(branch['constraint'])
                    #expr = ctx.simplify(expr)
                    models = ctx.getModel(astCtxt.land([previousConstraints, branch['constraint']]))
                    seed   = dict()
                    for k, v in list(models.items()):
                        # Get the symbolic variable assigned to the model
                        symVar = ctx.getSymbolicVariable(k)
                        # Save the new input as seed.
                        byteAddrAccessed = symVar.getOrigin()
                        byteAddrAccessed_relativeToInputBuffer = byteAddrAccessed - INPUT_BUFFER_ADDRESS
                        seed.update({byteAddrAccessed_relativeToInputBuffer : v.getValue()})
                    if seed:
                        inputs.append(seed)

        # Update the previous constraints with true branch to keep a good path.
        previousConstraints = astCtxt.land([previousConstraints, pc.getTakenPredicate()])

    # Clear the path constraints to be clean at the next execution.
    ctx.clearPathConstraints()

    return inputs


# Load segments into triton.
def loadBinary(ctx, path):
    global ENTRY

    logging.info(f"Loading the binary at path {path}..")
    import lief
    binary = lief.parse(path)
    phdrs  = binary.segments
    for phdr in phdrs:
        size   = phdr.physical_size
        vaddr  = phdr.virtual_address
        logging.info('[+] Loading 0x%06x - 0x%06x' %(vaddr, vaddr+size))
        ctx.setConcreteMemoryAreaValue(vaddr, phdr.content)

    logging.info(f"Findind the exported function of interest {path}..")
    res = binary.exported_functions
    for function in res:
        if ENTRY_FUNC_NAME in function.name:
            ENTRY = function.address
            logging.info(f"Function of interest found at address {ENTRY}")
            break

    assert ENTRY != None, "Exported function wasn't found"

if __name__ == '__main__':

    args = parseArgs()

    ctx = TritonContext(args.architecture)

    # Define symbolic optimizations
    ctx.setMode(MODE.ALIGNED_MEMORY, True)
    ctx.setMode(MODE.ONLY_ON_SYMBOLIZED, True) # ????

    # Load the binary
    loadBinary(ctx, args.binaryPath)

    # This is a list of inputs tested before.. # TODO: remove this after impl SAGE or other techiques...
    inputsAlreadyTried = list()

    # TODO: currently random input first with random size between allowed regions
    # use corpus and dictionary
    # Now we exhaustively search between different input sizes...
    for CURRENT_MAX_INPUT_SIZE in range(args.minLen, args.maxLen+1):
        # Set value 1 for byte index 0- thus,
        worklist  = list([{key : 0 for key in range(CURRENT_MAX_INPUT_SIZE)}]) # Input given as dictionary until we put the plain buffer in, this helps us identify duplicated inputs...

        while worklist:
            # Take the first seed
            seed = worklist[0]

            print('Seed injected:', seed)

            # Init context memory
            initContext(ctx, seed)

            # Emulate
            targetFound = emulate(ctx, ENTRY)

            if targetFound:
                seed_asStr = {k : chr(value) for k,value in sorted(seed.items())}
                print(f"The solution to get to the target address is input {seed} or in string format: {seed_asStr}!")
                exit(0)

            inputsAlreadyTried += [dict(seed)]
            del worklist[0]

            newInputs = getNewInput(ctx)
            for inputs in newInputs:
                if inputs not in inputsAlreadyTried and inputs not in worklist:
                    worklist += [dict(inputs)]

    sys.exit(0)

