#include "concolicExecutor.h"
#include "utils.h"
#include <string.h>
#include <set>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include "tracerExecutionStrategy.h"
#include "tracerExecutionStrategyExternal.h"
#include "tracerExecutionStrategyIPC.h"
#include "tracerExecutionStrategyMPI.h"

////////
// Some global definitions & configs

// If this is not commented, it will output all possible solutions in the given file
#define OUTPUT_ALL_GENERATED_TESTS 

// If below is enabled, we just take the textual output saved in the local folder and run over it.
// Usefull to debug things
//#define SIMULATE_TRACER_EXECUTION

// Enable this to append (assert X)  before any let instruction read from output file
#define MANUALLY_ADD_ASSERT_PREFIX

ConcolicExecutor::ConcolicExecutor(const ExecutionOptions& execOptions)
{
	m_execOptions = execOptions;

	switch (m_execOptions.m_execType)
	{
	case ExecutionOptions::EXEC_SERIAL:
		m_tracerExecutionStrategy = new TracerExecutionStrategyExternal(m_execOptions);
		break;
	case ExecutionOptions::EXEC_DISTRIBUTED_IPC:
		m_tracerExecutionStrategy = new TracerExecutionStrategyIPC(m_execOptions);
		break;
	case ExecutionOptions::EXEC_DISTRIBUTED_MPI:
		m_tracerExecutionStrategy = new TracerExecutionStrategyMPI(m_execOptions);
		break;	
	default:
		break;
	}

	m_tracerExecutionStrategy->init();

	// Get the state created by the strategy
	m_execState = m_tracerExecutionStrategy->getExecutionState();
}

ConcolicExecutor::~ConcolicExecutor()
{
	delete m_tracerExecutionStrategy;
}

// Run the input symbolically, negate the constraints one by one and get new inputs by solving them with the SMT
// Returns in the out variable the input childs of the base input parameter
void ConcolicExecutor::ExpandExecution(const InputPayload& input, std::vector<InputPayload>& outGeneratedInputChildren) 
{
	//execute and fill the path constraints for this input
	PathConstraint pathConstraint;
	m_tracerExecutionStrategy->executeTracerSymbolically(input, pathConstraint);

	const int numConstraints = pathConstraint.constraints.size();
	outGeneratedInputChildren.clear();
	outGeneratedInputChildren.reserve(numConstraints);
	// Initialize a path constraint solver then use it to inverse and solve inputs for all its children according to the algorithm
	PathConstraintZ3Solver pcsolver;
	pcsolver.init(&pathConstraint);
	for(int j = input.bound + 1; j < numConstraints; j++) 
	{
		// Step 1: generate first an input with j'th condition inverted
		//-------------
		pcsolver.pushState(); // Store state to revert later to it
		// Invert the test (i.e. if it was taken then go to not taken, and converse)
		pathConstraint.constraints[j].setInverted(true);
		// add to the interval solver the inverted j'th constraint
		pcsolver.addConstraint(j);
		// Get new input if conditions can be satisfied
		InputPayload newInputPayload;
		newInputPayload.input = input.input;  // Copy the original input and modify only the affected bytes
		if (pcsolver.solve(newInputPayload))
		{
			newInputPayload.bound = j;
			outGeneratedInputChildren.emplace_back(newInputPayload);
		}
		// Step 2: remove the inverted condition and add to the solver the normal condition before going to next children
		//-------------
		pcsolver.popState(); // Basically here we removed the last inverted condition
		// Put the flag back for consistency
		pathConstraint.constraints[j].setInverted(false);
		pcsolver.addConstraint(j);	// And add the normal condition here. before going to the next
	}
}

void ConcolicExecutor::searchSolutions(const ArrayOfUnsignedChars& startInput, const bool outputAllCoverageInputs, const char* fileToOutputCoverageInputs) 
{
	// Set the first input received and add it to the worklist
	InputPayload initialInput;
	initialInput.input = startInput;
	initialInput.bound = -1;
	m_workList.push(std::move(initialInput));

	std::vector<std::vector<unsigned char>> coverageInputs; 

	// Perform a search inside the existing tree of tasks
	while(!m_workList.empty())  
	{
		// Take the top scored path in the worklist - note that this is not executed yet and its constraints are not valid 
		const InputPayload inputPicked(std::move(m_workList.top()));
		m_workList.pop();

        if (outputAllCoverageInputs)
        {
		    coverageInputs.push_back(inputPicked.input);
        }

		// Execute tracer and library using this input and produce the children input
		// Expand the input by negating branch test along the path
		m_outGeneratedInputChildren.clear();
		ExpandExecution(inputPicked, m_outGeneratedInputChildren);
		// Iterate over all children input, run & check them, score then add to the worklist
		for (InputPayload& payloadChildren : m_outGeneratedInputChildren)
		{
			// TODO: get results and report potential problems somewhere
			m_tracerExecutionStrategy->executeTracerTracking(payloadChildren);
			
			m_workList.push(payloadChildren);
		}
	}
    
    if (outputAllCoverageInputs)
    {
        std::ofstream newfile(fileToOutputCoverageInputs);
        for (const std::vector<unsigned char> &s : coverageInputs) 
        {
			for (unsigned char chr : s)
            	newfile << (int)chr <<" ";
			newfile << std::endl;
        }
    }
}
