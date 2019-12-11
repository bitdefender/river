#ifndef CONCOLIC_DEFS_H
#define CONCOLIC_DEFS_H

#include <vector>
#include <stdio.h>
#include <semaphore.h>
#include <unordered_set>

#define TERMINATION_TAG  -1


struct ExecutionOptions
{
public:
	enum ExecutionType : char
	{
		EXEC_SERIAL,
		EXEC_DISTRIBUTED_IPC,
		EXEC_DISTRIBUTED_MPI,
	};

	enum OutputOptionFlags : char
	{
		OPTION_FILTER_NON_INTERESTING = 1 << 0,  // Show only inputs generated that show interest, crashes, asserts, etc.
		OPTION_BINARY = 1 << 1,					 // One file per input, outputed as binary
		OPTION_TEXT = 1 << 2,					// All inputs generated as text (byte numbers, one per each line)
	};

	ExecutionType m_execType = EXEC_SERIAL;		// If true, we use inter process communication between this and tracer. Highly recommended for release versions
	int m_numProcessesToUse = -1;				// The number of processes to use for tracer execution
	bool spawnTracersManually = false; 			// If true, tracers are spawned by hand
    int MAX_TRACER_INPUT_SIZE = -1;				// The max input/ouput size expected from workers
    int MAX_TRACER_OUTPUT_SIZE = -1;
	const char* testedLibrary;					// The library name under test		
		
	bool IsOutputOptionEnabled(OutputOptionFlags flag) { return (outputOptions & ((int)flag)) != 0;}
	int outputOptions = (int)OPTION_TEXT;

	std::string m_outputFolderPath;				// Path containing the new inputs dataset generated
};

class ExecutionState
{
public:
	// This is a set for all blocks (identified by their addresses) touched durin execution		
	std::unordered_set<int> m_blockAddresesTouched; 
};

#endif