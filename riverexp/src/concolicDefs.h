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

	ExecutionType m_execType = EXEC_SERIAL;		// If true, we use inter process communication between this and tracer. Highly recommended for release versions
	int m_numProcessesToUse = -1;				// The number of processes to use for tracer execution
	bool spawnTracersManually = false; 			// If true, tracers are spawned by hand
    int MAX_TRACER_INPUT_SIZE = -1;				// The max input/ouput size expected from workers
    int MAX_TRACER_OUTPUT_SIZE = -1;
	const char* testedLibrary;					// The library name under test		
};

class ExecutionState
{
public:
	// This is a set for all blocks (identified by their addresses) touched durin execution		
	std::unordered_set<int> m_blockAddresesTouched; 
};

#endif